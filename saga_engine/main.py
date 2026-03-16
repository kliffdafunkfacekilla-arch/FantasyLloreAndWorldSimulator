import arcade
import json
import threading

# Import the Spine (Memory & Bus)
from saga_engine.core.state import CampaignState, PlayerCharacter, CoreAttributes, VitalsState, WealthState
from saga_engine.core.bus import EventBus

# Import the Organs (Logic, World, Brain)
# from modules.lore_vault import LoreVaultDB # Placeholder for now
from saga_engine.modules.content_generators import WorldGenerator
from saga_engine.modules.director import DirectorBrain

# Import the Skin (VTT)
from saga_engine.ui.arcade_views import VTTView, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_TITLE

def load_static_databases() -> dict:
    """Bootstraps all JSON rulesets into memory at startup."""
    db = {}
    paths = {
        "weapons": "saga_engine/data/base_weapons.json",
        "charms": "saga_engine/data/hedge_charms.json",
        "hazards": "saga_engine/data/hazard_templates.json",
        "npcs": "saga_engine/data/npc_archetypes.json",
        "world": "saga_engine/data/Master_World.json"
    }
    for key, path in paths.items():
        try:
            with open(path, "r") as f:
                db[key] = json.load(f)
        except FileNotFoundError:
            db[key] = [] if key != "world" else {}
            print(f"[BOOT WARNING] {path} not found. Using empty data.")
    return db

def initialize_new_campaign() -> CampaignState:
    """Builds the 132-point Player Chassis and sets the initial World State."""
    # In a full build, this would be loaded from a save file or character creator
    player = PlayerCharacter(
        id="PC_01",
        name="Kaelen",
        species="MAMMAL",
        sub_type="Predator",
        attributes=CoreAttributes(might=12, endurance=11, finesse=11, reflexes=10, vitality=9, fortitude=9,
                                  knowledge=10, logic=9, charm=10, willpower=11, awareness=12, intuition=11),
        vitals=VitalsState(max_hp=31, current_hp=31, max_stamina=31, current_stamina=31,
                           max_composure=32, current_composure=32, max_focus=31, current_focus=31),
        wealth=WealthState(aetherium_coins=50, d_dust_grams=0.0)
    )
    
    return CampaignState(
        campaign_id="CAMP_001",
        active_player=player,
        current_location_node="", # Starting hex from Master_World.json
        chaos_level=1
    )

def main():
    print("[SYSTEM] Bootstrapping S.A.G.A. Engine...")
    
    # 1. Load Static Data into RAM
    static_db = load_static_databases()
    
    # 2. Forge the Spine
    global_state = initialize_new_campaign()
    bus = EventBus()
    
    # 3. Spin up the Lore Vault as a Background Thread
    # lore_vault = LoreVaultDB()
    # lore_thread = threading.Thread(target=lore_vault.start_daemon, daemon=True)
    # lore_thread.start()
    
    # 4. Attach the Organs to the Bus
    # These classes automatically subscribe to the exact events they need.
    # rules = RulesEngine(bus=bus, global_state=global_state) # Placeholder
    world_gen = WorldGenerator(bus=bus, global_state=global_state, static_db=static_db)
    director = DirectorBrain(bus=bus, global_state=global_state)
    
    # 5. Initialize the Skin (The Arcade Window)
    # The UI receives the bus to publish clicks, and the state to render health bars.
    window = arcade.Window(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_TITLE, resizable=False)
    vtt_view = VTTView(bus=bus, global_state=global_state)
    
    window.show_view(vtt_view)
    
    print("[SYSTEM] S.A.G.A. Engine successfully launched.")
    print("[SYSTEM] Awaiting player input on the Event Bus...")
    
    # 6. Run the 60 FPS Engine Loop
    arcade.run()

if __name__ == "__main__":
    main()
