from pydantic import BaseModel, Field
from typing import List, Dict, Optional

# 1. THE 132-POINT CHASSIS
class CoreAttributes(BaseModel):
    """The 6/6 Physical and Mental Split"""
    # Sector I: Physical
    might: int = 0
    endurance: int = 0
    vitality: int = 0
    fortitude: int = 0
    reflexes: int = 0
    finesse: int = 0
    
    # Sector II: Mental
    knowledge: int = 0
    logic: int = 0
    charm: int = 0
    willpower: int = 0
    awareness: int = 0
    intuition: int = 0

# 2. SURVIVAL POOLS & TRAUMA
class VitalsState(BaseModel):
    """Derived directly from the Core Attributes and tracked during gameplay"""
    current_hp: int = 0
    max_hp: int = 0          # Might + Reflexes + Vitality
    
    current_stamina: int = 0
    max_stamina: int = 0     # Endurance + Fortitude + Finesse
    
    current_composure: int = 0
    max_composure: int = 0   # Willpower + Logic + Awareness
    
    current_focus: int = 0
    max_focus: int = 0       # Knowledge + Charm + Intuition
    
    # Dual-Track Injury Slots
    body_injuries: List[str] = []
    mind_injuries: List[str] = []

# 3. WEALTH & ECONOMY
class WealthState(BaseModel):
    """Tracks the stable standard and the chaotic volatile commodity"""
    aetherium_coins: int = 0
    d_dust_grams: float = 0.0
    current_exchange_rate: float = Field(default=1.0, description="1 gram D-Dust = X Aetherium")

# 4. THE PLAYER ENTITY
class PlayerCharacter(BaseModel):
    """The complete representation of the player's biological and software choices"""
    id: str
    name: str
    species: str
    sub_type: str
    
    attributes: CoreAttributes
    vitals: VitalsState
    wealth: WealthState
    
    # Active loadout (Heavy Armor locks Stamina, Robes lock Focus)
    equipped_weapon: Optional[str] = None
    equipped_armor: Optional[str] = None
    
    # Tactical Grit and Skill Progress
    scars_and_stars_pips: Dict[str, int] = {} 
    
    # The 5 Anatomical Part Passives and 4 Skill Track Abilities
    active_passives: List[Dict[str, str]] = [] 

# 5. THE MASTER WORLD STATE
class CampaignState(BaseModel):
    """The Ultimate Source of Truth loaded into memory at runtime"""
    campaign_id: str
    chaos_level: int = 1
    current_resonance: Optional[str] = None
    active_edicts: List[str] = []
    world_tags: Dict[str, List[str]] = {} # e.g. "MAP_HEX_001": ["Obscured", "Vapor"]
    current_time: Dict[str, str] = {"year": "1024", "season": "Deep Winter", "day": "1"}
    
    active_player: PlayerCharacter
    
    # Data Bus / Director Elements
    current_location_node: str = ""
    active_quests: List[Dict] = []
    world_deltas: List[Dict] = [] # Tracks persistent changes like destroyed settlements
    
    # When the Director triggers an Ambush, this locks the Arcade Interface
    ui_locked: bool = False 
