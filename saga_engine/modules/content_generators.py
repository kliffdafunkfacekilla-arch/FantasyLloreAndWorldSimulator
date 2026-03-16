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
            self.hazards_db = static_db.get("hazards", [])
            self.npc_db = static_db.get("npcs", [])
        else:
            try:
                with open("saga_engine/data/hazard_templates.json", "r") as f:
                    self.hazards_db = json.load(f)
                with open("saga_engine/data/npc_archetypes.json", "r") as f:
                    self.npc_db = json.load(f)
            except FileNotFoundError:
                self.hazards_db = []
                self.npc_db = []
            
        # Subscribe to the Event Bus
        self.bus.subscribe("USE_ITEM", self.resolve_consumable)
        self.bus.subscribe("GENERATE_LOOT", self.generate_loot_drop)

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
        """Generates a trap strictly utilizing the 36 Tactical Triads."""
        # Example: Overgrown Razor-Vines
        return {
            "category": EncounterCategory.HAZARD,
            "title": "Overgrown Razor-Vines",
            "mechanics": {
                "detection_triad": "Awareness + Intuition", 
                "bypass_triad": "Finesse + Reflexes",
                "failure_consequence": "Apply 1 Minor Body Injury (Laceration)"
            }
        }

    def _build_social_obstacle(self) -> dict:
        """Generates an NPC with a Composure Pool (Social HP)."""
        return {
            "category": EncounterCategory.SOCIAL,
            "title": "Suspicious Border Guard",
            "npc_data": {
                "name": "Krag",
                "disposition": "SUSPICIOUS",
                "composure_pool": 18, # Willpower + Logic + Awareness
                "motives": ["Protect the border", "Wants a bribe"]
            }
        }

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
        """Calculates item math so the UI doesn't have to."""
        item_name = payload["item_name"]
        
        if item_name == "Vigor Salts":
            # Rolls 1d4 healing
            recovered = random.randint(1, 4)
            self.state.active_player.vitals.current_stamina = min(
                self.state.active_player.vitals.current_stamina + recovered,
                self.state.active_player.vitals.max_stamina
            )
            self.bus.publish("STATE_UPDATE", {"element": "VITALS", "msg": f"Healed {recovered} Stamina."})
            
        elif item_name == "Cinder-Pot":
            # Rolls 2d6 Fire damage
            dmg = random.randint(1, 6) + random.randint(1, 6)
            self.bus.publish("STATE_UPDATE", {"element": "COMBAT_LOG", "msg": f"Cinder-Pot deals {dmg} Fire damage (Reflex Half)."})
