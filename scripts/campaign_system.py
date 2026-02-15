
import json
import os
import random
import uuid
import urllib.request
from typing import List, Dict, Optional
from enum import Enum
from pydantic import BaseModel

# --- DATA MODELS ---

class QuestType(str, Enum):
    HOSTILE = "Hostile"
    SOCIAL = "Social"
    PUZZLE = "Puzzle"
    HUNT = "Hunt"
    REVENGE = "Revenge"
    EXPLORATION = "Exploration"

class QuestStep(BaseModel):
    step_id: str
    title: str
    description: str
    type: QuestType
    status: str = "active" # active, completed, failed
    target_location_id: Optional[str] = None
    target_npc_id: Optional[str] = None

class PlotPoint(BaseModel):
    id: str
    stage_name: str # e.g., "The Call to Adventure"
    description: str
    is_major: bool = True
    quests: List[QuestStep] = []
    completed: bool = False

class POIType(str, Enum):
    PERSON = "Person"
    CORPSE = "Corpse"
    ITEM = "Item"
    MONSTER = "Hostile Monster"
    LANDMARK = "Minor Landmark"
    DREAM = "Ethereal Dream"

class POI(BaseModel):
    id: str
    type: POIType
    description: str
    location_id: Optional[str] = None
    x: int
    y: int
    discovered: bool = False

class CampaignState(BaseModel):
    campaign_id: str
    hero_name: str
    current_step_index: int = 0
    plot_points: List[PlotPoint]
    world_seeds: List[Dict] = [] # Filler events
    pois: List[POI] = [] # Reactive Points of Interest
    global_meta: Dict = {} # Storage for simulation metrics

# --- BRAIN CLIENT ---

class SagaBrainClient:
    """Handles communication with the SAGA Simulation Brain (server.py)."""
    def __init__(self, base_url: str = "http://localhost:8000"):
        self.base_url = base_url

    def get_context(self, x: int, y: int) -> Optional[Dict]:
        """Queries the brain for local economic and structural context."""
        try:
            url = f"{self.base_url}/context?x={x}&y={y}"
            print(f"[DEBUG] Brain Query: {url}")
            with urllib.request.urlopen(url, timeout=2) as response:
                return json.loads(response.read().decode())
        except Exception as e:
            print(f"[DEBUG] Brain Query Failed: {e}")
            return None

    def get_global_meta(self) -> Dict:
        """Fetch global world stats (wealth, population)."""
        try:
            with urllib.request.urlopen(self.base_url, timeout=2) as response:
                return json.loads(response.read().decode()).get('data', {})
        except Exception:
            return {}

# --- CAMPAIGN GENERATOR ---

class CampaignGenerator:
    def __init__(self, save_dir: str = "./saves"):
        self.save_dir = save_dir
        os.makedirs(self.save_dir, exist_ok=True)
        self.save_path = os.path.join(self.save_dir, "campaign_active.json")
        self.current_campaign: Optional[CampaignState] = None
        self.brain = SagaBrainClient()

    def create_new_campaign(self, hero_name: str, theme: str = "Classic High Fantasy") -> CampaignState:
        """Generates a full 12-step Hero's Journey Campaign infused with simulation data."""
        
        campaign_id = str(uuid.uuid4())
        global_meta = self.brain.get_global_meta()
        
        # The 12 Steps of the Hero's Journey (Vogler/Campbell)
        steps = [
            ("The Ordinary World", "Introduce the hero in their normal life."),
            ("The Call to Adventure", "Something disrupts the status quo."),
            ("Refusal of the Call", "The hero hesitates or fears the unknown."),
            ("Meeting the Mentor", "The hero receives guidance or a magical gift."),
            ("Crossing the Threshold", "The hero leaves the ordinary world for the special world."),
            ("Tests, Allies, Enemies", "The hero explores the new world, facing minor challenges."),
            ("Approach to the Inmost Cave", "The hero prepares for the main danger."),
            ("The Ordeal", "The central life-or-death crisis."),
            ("Reward (Seizing the Sword)", "The hero claims the prize of the ordeal."),
            ("The Road Back", "The hero must return home, often chased."),
            ("The Resurrection", "The final test where the hero is reborn."),
            ("Return with the Elixir", "The hero returns home changed, bringing power/knowledge.")
        ]
        
        plot_points = []
        for i, (stage, desc) in enumerate(steps):
            # 1. Select a relevant location from simulation context
            rand_x, rand_y = random.randint(100, 900), random.randint(100, 900)
            context = self.brain.get_context(rand_x, rand_y)
            
            flavor_text = desc
            target_id = None
            
            if context and context.get('nearest_landmark'):
                landmark = context['nearest_landmark']
                economy = context.get('local_economy', {})
                territory = context.get('territory', 'the wildlands')
                
                if economy.get('wealth', 0) > 1000:
                    flavor_text = f"{desc} Inside the prosperous {landmark['type']} of {landmark['name']}, where wealth flows like water."
                elif economy.get('infra', 0) < 0.2:
                    flavor_text = f"{desc} In the rugged, undeveloped territory of {territory}, where roads are few and danger is constant."
                else:
                    flavor_text = f"{desc} Near {landmark['name']}, a significant junction in {territory}."
                
                target_id = landmark.get('name')

            q_type = self._determine_quest_type(stage)
            
            main_quest = QuestStep(
                step_id=f"q_main_{i}",
                title=f"{stage}: {q_type.value} Task",
                description=flavor_text,
                type=q_type,
                target_location_id=target_id
            )
            
            pp = PlotPoint(
                id=f"step_{i}",
                stage_name=stage,
                description=desc,
                quests=[main_quest]
            )
            plot_points.append(pp)

        # 4. Generate Initial POIs (Story Seeds) between campaign scenes
        pois = self._generate_world_pois(count=5)
            
        self.current_campaign = CampaignState(
            campaign_id=campaign_id,
            hero_name=hero_name,
            plot_points=plot_points,
            pois=pois,
            global_meta=global_meta
        )
        
        self.save_campaign()
        return self.current_campaign

    def _generate_world_pois(self, count: int = 20) -> List[POI]:
        """Scatters POIs across the world map."""
        pois = []
        for i in range(count):
            rx, ry = random.randint(50, 950), random.randint(50, 950)
            ctx = self.brain.get_context(rx, ry)
            
            p_type = random.choice(list(POIType))
            desc = f"Something unusual at [{rx}, {ry}]."
            
            if ctx:
                landmark = ctx.get('nearest_landmark')
                if p_type == POIType.MONSTER:
                    desc = f"A dangerous presence reported near {landmark['name'] if landmark else 'the wilds'}."
                elif p_type == POIType.CORPSE:
                    desc = f"A grim discovery on the road leading from {landmark['name'] if landmark else 'unknown lands'}."
                elif p_type == POIType.ITEM:
                    desc = f"A glimmering object caught in the debris of {landmark['type'] if landmark else 'the terrain'}."
                elif p_type == POIType.DREAM:
                    desc = f"A strange atmospheric haze near {rx}, {ry} that feels like a fading memory."
            
            pois.append(POI(
                id=f"poi_{i}",
                type=p_type,
                description=desc,
                x=rx,
                y=ry
            ))
        return pois

    def trigger_side_quest(self, poi_id: str) -> Optional[QuestStep]:
        """Dynamically builds a quest when a player interacts with a POI."""
        if not self.current_campaign:
            return None
            
        poi = next((p for p in self.current_campaign.pois if p.id == poi_id), None)
        if not poi:
            return None

        poi.discovered = True
        ctx = self.brain.get_context(poi.x, poi.y)
        
        # Determine Quest Type based on POI + Logic
        q_type = QuestType.EXPLORATION
        if poi.type == POIType.MONSTER: q_type = QuestType.HUNT
        elif poi.type == POIType.CORPSE: q_type = QuestType.REVENGE
        elif poi.type == POIType.PERSON: q_type = QuestType.SOCIAL
        elif poi.type == POIType.ITEM: q_type = QuestType.PUZZLE

        flavor = f"Following the {poi.type.value}: {poi.description}"
        if ctx:
            wealth = ctx.get('local_economy', {}).get('wealth', 0)
            if wealth > 2000 and q_type == QuestType.REVENGE:
                flavor += " This death in such a prosperous area smells of calculated assassination."

        side_q = QuestStep(
            step_id=f"side_{poi_id}",
            title=f"The {poi.type.value} Secret",
            description=flavor,
            type=q_type,
            target_location_id=poi.location_id
        )
        
        idx = self.current_campaign.current_step_index
        if idx < len(self.current_campaign.plot_points):
            self.current_campaign.plot_points[idx].quests.append(side_q)
            
        self.save_campaign()
        print(f"[REACTIVE] Triggered Side Quest: {side_q.title}")
        return side_q

    def _determine_quest_type(self, stage_name: str) -> QuestType:
        """Maps Hero's Journey stages to Gameplay Quest Types."""
        if "Enemies" in stage_name or "Ordeal" in stage_name or "Resurrection" in stage_name:
            return QuestType.HOSTILE
        elif "Mentor" in stage_name or "Ordinary" in stage_name:
            return QuestType.SOCIAL
        elif "Approach" in stage_name or "Road Back" in stage_name:
            return QuestType.EXPLORATION
        elif "Reward" in stage_name:
            return QuestType.PUZZLE
        else:
            return QuestType.HUNT

    def load_campaign(self) -> Optional[CampaignState]:
        if not os.path.exists(self.save_path):
            return None
        
        with open(self.save_path, "r") as f:
            data = json.load(f)
            self.current_campaign = CampaignState(**data)
        return self.current_campaign

    def save_campaign(self):
        if self.current_campaign:
            with open(self.save_path, "w") as f:
                json.dump(self.current_campaign.dict(), f, indent=4)

    def get_current_objective(self) -> Optional[QuestStep]:
        if not self.current_campaign:
            return None
            
        idx = self.current_campaign.current_step_index
        if idx >= len(self.current_campaign.plot_points):
            return None
            
        pp = self.current_campaign.plot_points[idx]
        for q in pp.quests:
            if q.status == "active":
                return q
        return None

    def advance_plot(self):
        """Moves to the next plot point."""
        if not self.current_campaign:
            return

        idx = self.current_campaign.current_step_index
        if idx < len(self.current_campaign.plot_points):
            self.current_campaign.plot_points[idx].completed = True
            
        self.current_campaign.current_step_index += 1
        self.save_campaign()

if __name__ == "__main__":
    # --- TEST RUN ---
    gen = CampaignGenerator()
    print("[SAGA] Generating new Campaign with Reactive POIs...")
    campaign = gen.create_new_campaign("Alaric the Bold")
    
    print(f"\n==================================================")
    print(f" CAMPAIGN: {campaign.campaign_id}")
    print(f" HERO:     {campaign.hero_name}")
    print(f" POI COUNT: {len(campaign.pois)}")
    print(f"==================================================\n")
    
    # Simulate Discovery of the first POI
    if campaign.pois:
        first_poi = campaign.pois[0]
        print(f"[PLAYER] Discovered POI: {first_poi.type.value} at [{first_poi.x}, {first_poi.y}]")
        print(f"  > {first_poi.description}")
        
        gen.trigger_side_quest(first_poi.id)
        
        # Verify side quest added
        active_q = gen.get_current_objective()
        if active_q:
            print(f"\n[QUEST LOG] Active Objective: {active_q.title}")
            print(f"  > {active_q.description}")
