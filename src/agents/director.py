from .base_agent import SagaAgent

class DirectorAgent(SagaAgent):
    def __init__(self):
        super().__init__("Kubrick", "Director")

    def establish_scene(self, x, y):
        self.log(f"Setting the scene at {x}, {y}...")
        context = self.query_brain("/context", {"x": x, "y": y})
        
        if not context:
            return "Scene unclear. (Connection Lost)"

        location = context.get('location', {})
        territory = context.get('territory', 'Unknown')
        danger = context.get('danger_level', 'Unknown')
        
        scene_desc = f"LOCATION: {territory}\n"
        scene_desc += f"COORDINATES: {location.get('x')}, {location.get('y')}\n"
        scene_desc += f"STATUS: {danger}\n"
        
        landmark = context.get('nearest_landmark')
        if landmark:
            scene_desc += f"NEARBY: {landmark['name']} ({landmark['distance_km']}km away)"
            
        return scene_desc
