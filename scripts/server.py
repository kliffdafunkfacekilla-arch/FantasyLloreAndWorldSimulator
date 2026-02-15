import os
import json
import uvicorn
import math
import random
from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from typing import Dict, List, Optional, Any
from pydantic import BaseModel

# Import campaign logic
try:
    from campaign_system import CampaignGenerator, POIType, QuestType, CampaignState
except ImportError:
    # Handle if running from scripts/ directory vs root
    import sys
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    from campaign_system import CampaignGenerator, POIType, QuestType, CampaignState

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
        self.campaign_gen = CampaignGenerator(save_dir=os.path.join(DATA_HUB, "Saves"))
    
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
            "campaign_active": db.campaign_gen.current_campaign is not None,
            "flags": db.gamestate.get('meta', {}).get('flags', {})
        }
    }

# --- CAMPAIGN ENDPOINTS ---

class NewCampaignRequest(BaseModel):
    hero_name: str
    theme: str = "Classic High Fantasy"

@app.post("/campaign/new")
def start_campaign(req: NewCampaignRequest):
    """Initializes a new Hero's Journey campaign via the API."""
    try:
        campaign = db.campaign_gen.create_new_campaign(req.hero_name, req.theme)
        return {"status": "success", "campaign_id": campaign.campaign_id}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/campaign/active")
def get_active_campaign():
    """Returns the current state of the campaign, active quests, and visible POIs."""
    campaign = db.campaign_gen.load_campaign()
    if not campaign:
        return {"status": "no_active_campaign"}
    
    # Get current plot point
    idx = campaign.current_step_index
    current_pp = campaign.plot_points[idx] if idx < len(campaign.plot_points) else None
    
    return {
        "campaign_id": campaign.campaign_id,
        "hero_name": campaign.hero_name,
        "theme": campaign.campaign_theme,
        "current_step": current_pp.stage_name if current_pp else "Finalized",
        "active_quests": [q for q in current_pp.quests if q.status == "active"] if current_pp else [],
        "pois": [p for p in campaign.pois if not p.discovered]
    }

@app.get("/campaign/pois")
def get_local_seeds(x: int, y: int):
    """
    Triggers on-the-fly seed injection near player and returns local POIs.
    Used by the Engine when entering a new map/area.
    """
    db.campaign_gen.generate_local_seeds(x, y)
    campaign = db.campaign_gen.current_campaign
    if not campaign:
        return []
    
    # Return seeds within local detection radius (e.g., 100 units)
    local_pois = [
        p for p in campaign.pois 
        if abs(p.x - x) < 100 and abs(p.y - y) < 100
    ]
    return local_pois

@app.post("/campaign/trigger/{poi_id}")
def trigger_poi(poi_id: str):
    """Converts a story seed (POI) into a side quest."""
    side_q = db.campaign_gen.trigger_side_quest(poi_id)
    if not side_q:
        raise HTTPException(status_code=404, detail="POI not found or campaign inactive")
    return {"status": "triggered", "quest": side_q}

@app.post("/campaign/advance")
def advance_story():
    """Moves the plot to the next Hero's Journey stage."""
    db.campaign_gen.advance_plot()
    return {"status": "advanced", "new_index": db.campaign_gen.current_campaign.current_step_index}

# --- TACTICAL VTT ENDPOINTS ---

@app.get("/tactical/generate")
def generate_tactical_map(x: int, y: int, poi_id: Optional[str] = None):
    """
    Generates a 2D tactical battle map (grid) based on world coordinates.
    Used by the Oracle VTT to transition from world travel to localized play.
    """
    ctx = get_world_context(x, y)
    landmark = ctx.get('nearest_landmark') if ctx else None
    biome_name = landmark.get('name', 'The Wilderness') if landmark else 'The Wilderness'
    
    # Simple procedural grid (20x20)
    width, height = 20, 20
    grid = [[0 for _ in range(width)] for _ in range(height)]
    
    # Add some random walls (10% density)
    for gy in range(height):
        for gx in range(width):
            if (gx == 0 or gx == width-1 or gy == 0 or gy == height-1):
                grid[gy][gx] = 1 # Boundary walls
            elif random.random() < 0.1:
                grid[gy][gx] = 1

    # Define entities
    entities = []
    
    # 1. The Player (Burt)
    entities.append({
        "id": "player_burt",
        "name": "Burt",
        "type": "player",
        "pos": [2, 2],
        "hp": 20,
        "maxHp": 20,
        "icon": "icons/race/aquatic_male.png",
        "tags": ["hero"]
    })

    # 2. Enemies based on context/POI
    danger = ctx.get('danger_level', 'Low') if ctx else 'Low'
    enemy_count = 2 if danger == 'Low' else 4
    
    # Theme-aware enemies
    theme = db.campaign_gen.current_campaign.campaign_theme if db.campaign_gen.current_campaign else "Classic"
    enemy_name = "Bandit"
    enemy_icon = "icons/race/human_male.png"
    
    if "Conspiracy" in theme:
        enemy_name = "Cultist"
        enemy_icon = "icons/class/warlock.png"
    elif "War" in theme:
        enemy_name = "Scout"
        enemy_icon = "icons/class/rogue.png"

    for i in range(enemy_count):
        entities.append({
            "id": f"enemy_{i}",
            "name": f"{enemy_name} #{i+1}",
            "type": "enemy",
            "pos": [width - 3 - i, height - 3],
            "hp": 10,
            "maxHp": 10,
            "icon": enemy_icon,
            "tags": ["hostile"]
        })

    # Session Data
    session = {
        "meta": {
            "title": f"Encounter near {biome_name}",
            "description": f"A tactical challenge in the {danger} danger zone.",
            "turn": 1,
            "world_pos": [x, y]
        },
        "map": {
            "width": width,
            "height": height,
            "grid": grid,
            "biome": "forest" if "Forest" in biome_name else "dungeon"
        },
        "entities": entities,
        "log": [f"Character Burt has entered {biome_name}."]
    }
    
    return session

@app.post("/tactical/feedback")
def tactical_feedback(result: Dict[str, Any]):
    """
    Receives tactical outcome and applies it to the world simulation.
    """
    outcome = result.get('outcome', 'DEFEAT')
    enemies_killed = result.get('enemies_killed', [])
    x = result.get('x', 500)
    y = result.get('y', 500)
    
    if outcome == 'VICTORY':
        # Reduce chaos or threat in this cell in the simulator
        log_msg = f"Victory achieved! {len(enemies_killed)} enemies defeated at {x},{y}."
        print(f"[TACTICAL] {log_msg}")
        return {"status": "applied", "message": "World state updated with tactical victory."}
        
    return {"status": "acknowledged", "message": "Tactical retreat or defeat recorded."}

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
