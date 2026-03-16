import arcade
import arcade.gui
from saga_engine.core.state import CampaignState
from saga_engine.core.bus import EventBus

# Screen Dimensions
SCREEN_WIDTH = 1920
SCREEN_HEIGHT = 1080
SCREEN_TITLE = "S.A.G.A. Engine VTT"

class VTTView(arcade.View):
    """The Master 5-Panel Arcade Interface"""
    
    def __init__(self, bus: EventBus, global_state: CampaignState):
        super().__init__()
        self.bus = bus
        self.state = global_state
        
        # 1. ARCADE CAMERAS
        # We use two cameras: One for the zooming hex map, one for the static UI
        self.camera_map = arcade.Camera(SCREEN_WIDTH, SCREEN_HEIGHT)
        self.camera_gui = arcade.Camera(SCREEN_WIDTH, SCREEN_HEIGHT)
        
        # 2. UI MANAGER (The 5-Panel Layout)
        self.ui_manager = arcade.gui.UIManager()
        self.ui_manager.enable()
        
        # --- LEFT PANEL: Director's Log ---
        self.chat_log = arcade.gui.UITextArea(
            x=0, y=200, width=400, height=880, 
            text="[SYSTEM] S.A.G.A. Engine Initialized.\n",
            text_color=arcade.color.WHITE
        )
        self.ui_manager.add(self.chat_log)

        # --- BOTTOM CONSOLE: The Action Deck ---
        # Includes the 'Burn Slider' and Tactical Skill Buttons
        self.assault_button = arcade.gui.UIFlatButton(text="ASSAULT", width=150)
        self.assault_button.on_click = self.on_assault_click # Bind click to Event Bus
        
        bottom_box = arcade.gui.UIBoxLayout(vertical=False)
        bottom_box.add(self.assault_button.with_space_around(right=20))
        
        self.ui_manager.add(
            arcade.gui.UIAnchorWidget(anchor_x="center_x", anchor_y="bottom", child=bottom_box)
        )

        # 3. SUBSCRIBE TO THE SPINE
        # Whenever the Rules Engine changes HP or the Director speaks, the UI instantly redraws
        self.bus.subscribe("STATE_UPDATE", self.handle_state_update)
        
        # 4. GLSL CHAOS SHADER PREP
        # Loads a custom GLSL fragment shader to warp the screen during DEADLOCKS
        # self.chaos_shader = arcade.gl.Program(fragment_shader="ui/shaders/chaos_warp.glsl")

    # --- INPUT BINDINGS (The Frontend-to-Spine Contract) ---

    def on_assault_click(self, event):
        """Fires when the player clicks the Assault button on the Action Deck."""
        if self.state.ui_locked:
            return # The Director is speaking; player cannot act.
            
        # Read the visual UI burn slider (mocked here as 2)
        stamina_burn = 2 
        
        # Publish the event instantly to the Data Bus
        self.bus.publish("CLASH_ATTACK", {
            "type": "ASSAULT", # Consistent with director's handle_action payload check if needed
            "action": "ASSAULT",
            "attacker_stamina_burn": stamina_burn,
            "weapon_damage": 8,
            "attacker_pool": self.state.active_player.attributes.might,
            "defender_pool": 12, # Mock target pool
            "defender_hp": 20
        })

    def on_mouse_press(self, x, y, button, modifiers):
        """Translates screen clicks into grid coordinates for the Director to intercept."""
        if self.state.ui_locked: return
        
        # Translate screen pixel X/Y to map coordinates
        grid_coord = f"[{int(x // 64)}, {int(y // 64)}]"
        
        # The Director is listening for this exact event!
        self.bus.publish("MAP_CLICK", {"type": "MOVE", "target": grid_coord})

    # --- STATE REDRAW (The UI Loop) ---

    def handle_state_update(self, payload: dict):
        """Instantly updates visual UI elements when the math changes in memory."""
        if payload.get("element") == "CHAT_LOG":
            # Append AI narration to the left panel
            current_text = self.chat_log.text
            self.chat_log.text = f"{current_text}\n{payload['text']}"
            
        elif payload.get("action") == "CLASH_RESOLVED":
            # Append mathematical results
            current_text = self.chat_log.text
            self.chat_log.text = f"{current_text}\n[SYSTEM: {payload['margin_result']}! Damage: {payload['damage']}]"

    def on_draw(self):
        """The 60 FPS Render Loop"""
        self.clear()
        
        # 1. DRAW THE BATTLEMAP (Center Stage)
        self.camera_map.use()
        # If Chaos Level > 3, we would render self.map_sprites through self.chaos_shader here
        # self.map_sprites.draw()
        
        # 2. DRAW THE STATIC UI (The 5 Panels)
        self.camera_gui.use()
        self.ui_manager.draw()
        
        # 3. DRAW DYNAMIC RESOURCE ORBS (Stamina & Focus)
        # We read directly from state.py. If stamina drops, the red arc shrinks instantly.
        if self.state.active_player.vitals.max_stamina > 0:
            stamina_percent = self.state.active_player.vitals.current_stamina / self.state.active_player.vitals.max_stamina
        else:
            stamina_percent = 0
            
        arcade.draw_arc_filled(
            center_x=150, center_y=100, 
            width=80, height=80, 
            color=arcade.color.DARK_RED, 
            start_angle=0, end_angle=360 * stamina_percent
        )
