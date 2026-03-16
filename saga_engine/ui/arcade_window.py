import arcade
import arcade.gui
from saga_engine.core.bus import EventBus
from saga_engine.core.state import CampaignState, PlayerCharacter, CoreAttributes, VitalsState, WealthState

class SagaWindow(arcade.Window):
    def __init__(self):
        super().__init__(1280, 720, "S.A.G.A. Engine VTT", resizable=True)
        self.bus = EventBus()
        
        # Initialize UI Manager
        self.manager = arcade.gui.UIManager()
        self.manager.enable()
        
        # Initial State Placeholder
        self.state = self._debug_init_state()
        
        # Set background color
        arcade.set_background_color(arcade.color.BLACK)
        
        # Subscribe to state updates
        self.bus.subscribe("STATE_UPDATE", self.on_state_update)

    def _debug_init_state(self) -> CampaignState:
        """Creates a dummy state for initial UI testing"""
        attrs = CoreAttributes(might=5, reflexes=5, vitality=5)
        vitals = VitalsState(current_hp=15, max_hp=15)
        player = PlayerCharacter(
            id="P_001", 
            name="Scavenger", 
            species="HUMAN", 
            sub_type="EXILE",
            attributes=attrs,
            vitals=vitals,
            wealth=WealthState()
        )
        return CampaignState(campaign_id="TEST_001", active_player=player)

    def on_state_update(self, new_state: CampaignState):
        """Redraw UI when state changes"""
        self.state = new_state
        print(f"[UI] State Updated: HP {self.state.active_player.vitals.current_hp}")

    def on_draw(self):
        self.clear()
        # Drawing logic for the 5-panel layout will go here
        arcade.draw_text(
            f"VTT INITIALIZED | PLAYER: {self.state.active_player.name}",
            self.width // 2, self.height - 30,
            arcade.color.AMBER, 16, anchor_x="center"
        )
        
        self.manager.draw()

def main():
    window = SagaWindow()
    arcade.run()

if __name__ == "__main__":
    main()
