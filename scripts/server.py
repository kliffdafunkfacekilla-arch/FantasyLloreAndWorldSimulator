import os
import json
import uvicorn
import math
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from typing import Dict, List, Optional, Any

# --- CONFIGURATION ---
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(CURRENT_DIR)
GITHUB_ROOT = os.path.dirname(PROJECT_ROOT)
DATA_HUB = os.path.join(GITHUB_ROOT, "SAGA_Global_Data")

LORE_PATH = os.path.join(DATA_HUB, "lore.json")
GAMESTATE_PATH = os.path.join(DATA_HUB, "gamestate.json")

print(f"[BOOT] SAGA Brain initializing...")
print(f"[BOOT] Data Hub: {DATA_HUB}")

# --- DATA LOADER ---
class WorldDatabase:
    def __init__(self):
        self.lore = []
        self.gamestate = {}
        self.factions = {}
        self.nodes = []
    
    def load(self):
        # Load Lore (Static)
        try:
            with open(LORE_PATH, 'r', encoding='utf-8') as f:
                self.lore = json.load(f)
            print(f"[DATA] Loaded {len(self.lore)} lore entries.")
        except Exception as e:
            print(f"[ERROR] Failed to load lore.json: {e}")
            self.lore = []

        # Load Gamestate (Dynamic)
        try:
            with open(GAMESTATE_PATH, 'r', encoding='utf-8') as f:
                self.gamestate = json.load(f)
            
            self.factions = {f['id']: f for f in self.gamestate.get('factions', [])}
            self.nodes = self.gamestate.get('nodes', [])
            
            meta = self.gamestate.get('meta', {})
            print(f"[DATA] Loaded Gamestate (Epoch: {meta.get('epoch', 0)})")
            print(f"[DATA] Factions: {len(self.factions)} | Nodes: {len(self.nodes)}")
            
        except Exception as e:
            print(f"[ERROR] Failed to load gamestate.json: {e}")
            self.gamestate = {}

db = WorldDatabase()
db.load()

# --- APP SETUP ---
app = FastAPI(title="SAYA: SAGA Brain API", version="1.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# --- HELPER FUNCTIONS ---
def get_distance(x1, y1, x2, y2):
    return math.sqrt((x2 - x1)**2 + (y2 - y1)**2)

def find_nearest_node(x: int, y: int, max_dist: float = 9999.0):
    nearest = None
    min_dist = max_dist
    
    for node in db.nodes:
        nx = node.get('x', 0)
        ny = node.get('y', 0)
        dist = get_distance(x, y, nx, ny)
        
        if dist < min_dist:
            min_dist = dist
            nearest = node
            
    return nearest, min_dist

# --- ENDPOINTS ---

@app.get("/")
def health_check():
    """Health check and status report."""
    return {
        "status": "online",
        "data": {
            "lore_count": len(db.lore),
            "faction_count": len(db.factions),
            "node_count": len(db.nodes),
            "global_wealth": db.gamestate.get('meta', {}).get('global_wealth', 0.0),
            "flags": db.gamestate.get('meta', {}).get('flags', {})
        }
    }

@app.get("/context")
def get_world_context(x: int, y: int, radius: Optional[float] = 50.0):
    """
    Returns the narrative context for a specific global coordinate.
    Identifies nearest faction territory, emergent settlements, and local wealth.
    """
    nearest_node, dist = find_nearest_node(x, y)
    
    context = {
        "location": {"x": x, "y": y},
        "territory": "Unclaimed Wilds",
        "nearest_landmark": None,
        "danger_level": "Unknown",
        "local_economy": {"wealth": 0.0, "infra": 0.0}
    }
    
    if nearest_node and dist < radius:
        context['nearest_landmark'] = {
            "name": nearest_node.get('name', 'Unknown'),
            "type": nearest_node.get('type', 'LANDMARK'),
            "distance_km": round(dist, 1),
            "is_lore_site": nearest_node.get('is_lore_site', True)
        }
        
        # Pull wealth/infra data if available (from dynamic nodes)
        context['local_economy']['wealth'] = nearest_node.get('wealth', 0.0)
        context['local_economy']['infra'] = nearest_node.get('infra', 0.0)
        
        if nearest_node.get('type') in ['CITY', 'VILLAGE']:
            context['territory'] = nearest_node.get('name')
            context['danger_level'] = "Civilized"
        else:
             context['danger_level'] = "Wilderness"

    return context

@app.get("/factions")
def get_factions():
    """List all known factions and their power."""
    return [
        {
            "id": f['id'],
            "name": f['name'],
            "power": f['power'],
            "age": f.get('year', 0)
        }
        for f in db.factions.values()
    ]

@app.get("/factions/{faction_id}")
def get_faction_details(faction_id: int):
    """Get detailed info for a specific faction."""
    if faction_id not in db.factions:
        raise HTTPException(status_code=404, detail="Faction not found")
    return db.factions[faction_id]

@app.post("/refresh")
def refresh_data():
    """Force reload of JSON data from disk."""
    db.load()
    return {"status": "refreshed"}

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
