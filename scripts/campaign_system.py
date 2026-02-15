
import json
import os
import random
import uuid
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

class CampaignState(BaseModel):
    campaign_id: str
    hero_name: str
    current_step_index: int = 0
    plot_points: List[PlotPoint]
    world_seeds: List[Dict] = [] # Filler events

# --- CAMPAIGN GENERATOR ---

class CampaignGenerator:
    def __init__(self, save_dir: str = "./saves"):
        self.save_dir = save_dir
        os.makedirs(self.save_dir, exist_ok=True)
        self.save_path = os.path.join(self.save_dir, "campaign_active.json")
        self.current_campaign: Optional[CampaignState] = None

    def create_new_campaign(self, hero_name: str, theme: str = "Classic High Fantasy") -> CampaignState:
        """Generates a full 12-step Hero's Journey Campaign."""
        
        campaign_id = str(uuid.uuid4())
        
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
            
            # Generate Main Quest for this Stage
            # In a real AI system, LLM would flavor this based on 'theme'
            q_type = self._determine_quest_type(stage)
            
            main_quest = QuestStep(
                step_id=f"q_main_{i}",
                title=f"{stage}: Main Objective",
                description=f"Complete the objective to advance: {desc}",
                type=q_type
            )
            
            # Add Filler/Seed Quests between major points (except start/end)
            side_quests = []
            if 0 < i < 11 and random.random() < 0.4: # 40% chance of a side quest
                sq_type = random.choice(list(QuestType))
                side_quests.append(QuestStep(
                    step_id=f"q_side_{i}",
                    title=f"Side Quest: {sq_type.value} Opportunity",
                    description="A local opportunity for adventure.",
                    type=sq_type
                ))

            pp = PlotPoint(
                id=f"step_{i}",
                stage_name=stage,
                description=desc,
                quests=[main_quest] + side_quests
            )
            plot_points.append(pp)
            
        self.current_campaign = CampaignState(
            campaign_id=campaign_id,
            hero_name=hero_name,
            plot_points=plot_points
        )
        
        self.save_campaign()
        return self.current_campaign

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
        # Return first active quest
        for q in pp.quests:
            if q.status == "active":
                return q
        return None

    def advance_plot(self):
        """Moves to the next plot point."""
        if not self.current_campaign:
            return

        # Mark current point complete
        idx = self.current_campaign.current_step_index
        if idx < len(self.current_campaign.plot_points):
            self.current_campaign.plot_points[idx].completed = True
            
        # Advance
        self.current_campaign.current_step_index += 1
        self.save_campaign()

