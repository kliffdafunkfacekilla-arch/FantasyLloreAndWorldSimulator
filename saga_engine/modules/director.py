from typing import TypedDict, Optional, List, Dict
from langgraph.graph import StateGraph, END
from langchain_core.prompts import PromptTemplate
from saga_engine.core.state import CampaignState
from saga_engine.core.bus import EventBus

# 1. THE LANGGRAPH STATE
class GameState(TypedDict):
    """The object passed between LangGraph nodes during a single tick."""
    action_type: str            # e.g., "MAP_CLICK", "INVESTIGATE_SEED"
    action_target: str          # e.g., "", "SEED_099"
    
    # Injected Context
    current_location: str
    active_quests: List[Dict]
    player_vitals: Dict
    
    # Director Commands
    director_override: Optional[str] # Forces the AI to narrate an ambush/event
    
    # Final Output
    ai_narration: str

class DirectorBrain:
    def __init__(self, bus: EventBus, global_state: CampaignState, lore_db=None):
        self.bus = bus
        self.state = global_state
        self.llm = None # Initialize LangChain LLM here (e.g., from langchain_openai)
        self.lore_db = lore_db
        
        # Build the Cyclical State Machine
        workflow = StateGraph(GameState)
        workflow.add_node("context_injector", self.inject_context_node)
        workflow.add_node("director_check", self.director_node)
        workflow.add_node("voice_actor", self.narrator_node)
        
        workflow.set_entry_point("context_injector")
        workflow.add_edge("context_injector", "director_check")
        workflow.add_edge("director_check", "voice_actor")
        workflow.add_edge("voice_actor", END)
        
        self.engine = workflow.compile()
        
        # Subscribe to the Spine
        self.bus.subscribe("MAP_CLICK", self.handle_action)
        self.bus.subscribe("INVESTIGATE_SEED", self.handle_action)

    def handle_action(self, payload: dict):
        """Triggered instantly when the Event Bus fires."""
        initial_state = {
            "action_type": payload.get("type", ""),
            "action_target": payload.get("target", ""),
            "director_override": None
        }
        # Run the LangGraph pipeline
        result = self.engine.invoke(initial_state)
        
        # Push the AI narration to the Arcade VTT Director's Log
        self.bus.publish("STATE_UPDATE", {"element": "CHAT_LOG", "text": result["ai_narration"]})

    # --- LANGGRAPH NODES ---

    def inject_context_node(self, state: GameState):
        """NODE 1: Pulls the absolute truth from memory so the AI cannot hallucinate."""
        return {
            "current_location": self.state.current_location_node,
            "active_quests": self.state.active_quests,
            "player_vitals": self.state.active_player.vitals.model_dump()
        }

    def director_node(self, state: GameState):
        """NODE 2: Checks for traps or Just-In-Time generation triggers."""
        
        # SCENARIO A: The player clicked a Story Seed
        if state["action_type"] == "INVESTIGATE_SEED":
            # 1. Call Weaver logic synchronously to build the 3-step micro-roadmap
            new_quest = self.generate_jit_quest(state["action_target"])
            self.state.active_quests.append(new_quest)
            
            # 2. Lock the UI and force the AI to narrate the hook
            self.state.ui_locked = True
            return {"director_override": f"[DIRECTOR COMMAND: The player investigated {state['action_target']}. They discovered tracks leading to a new quest: {new_quest['title']}. Narrate this discovery.]"}
            
        # SCENARIO B: The player stepped on an Ambush trigger
        elif state["action_type"] == "MAP_CLICK":
            target_hex = state["action_target"]
            for quest in self.state.active_quests:
                if quest.get("trigger_location") == target_hex:
                    self.state.ui_locked = True
                    # In a full build, this would also broadcast SPAWN_ENEMIES to the UI
                    return {"director_override": f"[DIRECTOR COMMAND: The player stepped into Hex {target_hex}, triggering the '{quest['title']}' ambush. Ignore their intended movement. Narrate enemies emerging and rolling for initiative.]"}
                    
        return {"director_override": None}

    def narrator_node(self, state: GameState):
        """NODE 3: The AI Voice Actor writes the script based on strict instructions."""
        prompt = f"""
        You are the gritty, tactical Game Master of the SAGA Engine.
        
        --- HIDDEN WORLD STATE ---
        Location: {state["current_location"]}
        Player Vitals: HP {state["player_vitals"]["current_hp"]}/{state["player_vitals"]["max_hp"]}
        Active Quests: {state["active_quests"]}
        """
        
        if state["director_override"]:
            prompt += f"\n\n{state['director_override']}\nWrite the narration."
        else:
            prompt += f"\n\nPlayer Action: {state['action_type']} on {state['action_target']}.\nWrite 2 sentences of narration detailing their movement."

        # Placeholder response if LLM is not initialized
        if self.llm:
            response = self.llm.invoke(prompt)
            ai_narration = response.content
        else:
            ai_narration = f"[AI NARRATION PLACEHOLDER] The Director observes your action: {state['action_type']} on {state['action_target']}."

        return {"ai_narration": ai_narration}

    def generate_jit_quest(self, seed_target: str):
        """Synchronous Campaign Weaver logic."""
        # Query Local ChromaDB Lore Vault here, then use LangChain structured output to build quest
        return {"title": "The Feathered Thieves", "trigger_location": ""}
