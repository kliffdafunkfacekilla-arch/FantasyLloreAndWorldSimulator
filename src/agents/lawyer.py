from .base_agent import SagaAgent

class LawyerAgent(SagaAgent):
    def __init__(self):
        super().__init__("Murdoch", "Lawyer")

    def check_jurisdiction(self, faction_name):
        self.log(f"Checking jurisdiction for {faction_name}...")
        
        # In a real scenario, this would check rules.json or a specific endpoint
        # For now, we verify if the faction exists in the database
        factions = self.query_brain("/factions")
        if not factions:
            return "OBJECTION: Database Unreachable."
            
        for f in factions:
            if f['name'] == faction_name:
                return f"SUSTAINED: {faction_name} is a recognized political entity (Power: {f['power']})."
        
        return "OVERRULED: Faction not recognized by the court."
