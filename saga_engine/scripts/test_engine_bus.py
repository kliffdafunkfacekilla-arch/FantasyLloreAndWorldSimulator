from saga_engine.core.bus import EventBus
from saga_engine.core.state import CoreAttributes, PlayerCharacter, VitalsState, WealthState, CampaignState
from saga_engine.core.calc_vitals import calculate_pools

def test_bus_interaction():
    print("--- STARTING BUS VERIFICATION ---")
    bus = EventBus()
    
    # 1. Setup Test State
    stats = CoreAttributes(might=10, reflexes=8, vitality=5)
    vitals = calculate_pools(stats)
    player = PlayerCharacter(
        id="PLAYER_TEST", 
        name="BusTester", 
        species="CYBORG", 
        sub_type="GLITCH",
        attributes=stats,
        vitals=vitals,
        wealth=WealthState()
    )
    state = CampaignState(campaign_id="VERIFY_001", active_player=player)
    
    # 2. Define reactive update
    def handle_state_change(new_payload):
        nonlocal state
        print(f"[BUS] Received STATE_UPDATE: {new_payload}")
        if "hp_change" in new_payload:
            state.active_player.vitals.current_hp += new_payload["hp_change"]
            print(f"[SYNC] New HP: {state.active_player.vitals.current_hp}")

    bus.subscribe("STATE_UPDATE", handle_state_change)
    
    # 3. Simulate a Clash Result event
    print("[SIM] Simulating damage from a Clash...")
    bus.publish("STATE_UPDATE", {"hp_change": -4, "reason": "Clash Scrape"})
    
    # 4. Final Assertion
    if state.active_player.vitals.current_hp == 19: # 23 - 4
        print("--- VERIFICATION SUCCESS ---")
    else:
        print(f"--- VERIFICATION FAILED (HP: {state.active_player.vitals.current_hp}) ---")

if __name__ == "__main__":
    test_bus_interaction()
