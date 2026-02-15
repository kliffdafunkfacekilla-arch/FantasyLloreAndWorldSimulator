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
        economy = context.get('local_economy', {})
        
        scene_desc = f"LOCATION: {territory}\n"
        scene_desc += f"COORDINATES: {location.get('x')}, {location.get('y')}\n"
        scene_desc += f"STATUS: {danger}\n"
        scene_desc += f"ECONOMY: Wealth {economy.get('wealth', 0.0):.1f} | Infra {economy.get('infra', 0.0):.2f}\n"
        
        landmark = context.get('nearest_landmark')
        if landmark:
            l_type = landmark.get('type', 'LANDMARK')
            l_source = "LORE" if landmark.get('is_lore_site', True) else "EMERGENT"
            scene_desc += f"NEARBY: {landmark['name']} [{l_type} | {l_source}] ({landmark['distance_km']}km away)"
            
        return scene_desc
