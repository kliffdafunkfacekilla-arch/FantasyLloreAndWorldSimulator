from .base_agent import SagaAgent

class NarratorAgent(SagaAgent):
    def __init__(self):
        super().__init__("Morgan", "Narrator")

    def narrate(self, scene_context):
        self.log("Weaving the tale...")
        return f"\"The wind howls through the {scene_context.replace(chr(10), ', ')}...\""
