import random
import json
from enum import Enum
from saga_engine.core.state import CampaignState
from saga_engine.core.bus import EventBus

# --- 1. STRICT SCHEMAS ---
class EncounterCategory(str, Enum):
    COMBAT = "COMBAT"
    SOCIAL = "SOCIAL"
    HAZARD = "HAZARD"
    PUZZLE = "PUZZLE"
    DISCOVERY = "DISCOVERY"
    DILEMMA = "DILEMMA"

class WorldGenerator:
    """Handles Procedural Encounters, Volatile Economy, and Item Mechanics."""
    
    def __init__(self, bus: EventBus, global_state: CampaignState, static_db: dict = None):
        self.bus = bus
        self.state = global_state
        
        # Use provided static_db or load from files
        if static_db:
            self.hazards_db = static_db.get("hazards", {})
            self.npc_db = static_db.get("npcs", {})
            self.magic_db = static_db.get("magic", {})
            self.weapons_db = static_db.get("weapons", {})
            self.charms_db = static_db.get("charms", {})
        else:
            # Fallback for local testing
            self.hazards_db = {}
            self.npc_db = {}
            self.magic_db = {}
            self.weapons_db = {}
            self.charms_db = {}
            
        # Subscribe to the Event Bus
        self.bus.subscribe("USE_ITEM", self.resolve_consumable)
        self.bus.subscribe("GENERATE_LOOT", self.generate_loot_drop)
        self.bus.subscribe("CHECK_BANE", self.resolve_bane_collision)

    # --- ENCOUNTER GENERATION (The Friction) ---
    
    def spawn_encounter(self, biome: str, threat_level: int) -> dict:
        """Called synchronously by the Director when a player hits an empty hex."""
        
        # Roll d100 to determine the friction type based on biome
        roll = random.randint(1, 100)
        
        if roll <= 40:
            return self._build_hazard()
        elif roll <= 70:
            return self._build_social_obstacle()
        else:
            return self._build_dilemma()

    def _build_hazard(self) -> dict:
        """Pulls a random hazard from the database."""
        if not self.hazards_db:
            return {"title": "Generic Hazard", "mechanics": {}}
        
        haz_id = random.choice(list(self.hazards_db.keys()))
        return self.hazards_db[haz_id]

    def _build_social_obstacle(self) -> dict:
        """Pulls a random NPC archetype."""
        if not self.npc_db:
            return {"title": "Generic NPC", "npc_data": {}}
            
        npc_id = random.choice(list(self.npc_db.keys()))
        return self.npc_db[npc_id]

    def _build_dilemma(self) -> dict:
        """Generates a binary choice with long-term consequences."""
        return {
            "category": EncounterCategory.DILEMMA,
            "title": "The Desperate Refugee",
            "description": "A ragged traveler begs for food. Giving some will reduce your supplies but might gain you a future ally."
        }

    # --- ECONOMY & LOOT (The Reward) ---

    def calculate_d_dust_rate(self, base_rate: float = 10.0) -> float:
        """The D-Dust Volatility Calculator."""
        # Modifies the base exchange rate based on the world's Chaos Level
        volatility = self.state.chaos_level * 0.2 
        fluctuation = random.uniform(1.0 - volatility, 1.0 + volatility)
        
        new_rate = base_rate * fluctuation
        return round(new_rate, 2)
        
    def generate_loot_drop(self, payload: dict):
        """Calculates wealth and items, then pushes to the Event Bus."""
        current_rate = self.calculate_d_dust_rate()
        
        # Example drop
        loot = {
            "aetherium_coins": random.randint(5, 20),
            "d_dust_grams": round(random.uniform(0.5, 3.0), 2),
            "market_value_note": f"Current D-Dust Rate: 1g = {current_rate} Aetherium",
            "items": ["Vigor Salts", "Cinder-Pot"]
        }
        self.bus.publish("STATE_UPDATE", {"element": "INVENTORY", "data": loot})

    # --- ITEM RESOLUTION (The Mechanics) ---

    def resolve_consumable(self, payload: dict):
        """Calculates item math using the database effect_math."""
        item_id = payload.get("item_id")
        item_data = self.charms_db.get(item_id)
        
        if not item_data:
            self.bus.publish("STATE_UPDATE", {"element": "CHAT", "msg": f"Item {item_id} unrecognized."})
            return

        math_str = item_data.get("effect_math", "")
        # Simple parser for the blueprint math strings
        if "Heal" in math_str:
            # Logic for healing HP
            self.bus.publish("STATE_UPDATE", {"element": "VITALS", "msg": f"Used {item_data['name']}: {math_str}"})
        elif "Stamina" in math_str:
            # Logic for Stamina
            self.bus.publish("STATE_UPDATE", {"element": "VITALS", "msg": f"Used {item_data['name']}: {math_str}"})
            
    def resolve_bane_collision(self, payload: dict):
        """Standard Ouroboros Logic: Does Pillar A snuff out Pillar B?"""
        attacker_pillar = payload.get("attacker_pillar")
        defender_pillar = payload.get("defender_pillar")
        
        pillar_data = self.magic_db.get("pillars", {}).get(attacker_pillar)
        if pillar_data and pillar_data.get("bane_of") == defender_pillar:
            self.bus.publish("STATE_UPDATE", {
                "element": "COMBAT_LOG", 
                "msg": f"BANE COLLISION: {attacker_pillar} snuffs out {defender_pillar}!"
            })
            return True
        return False
