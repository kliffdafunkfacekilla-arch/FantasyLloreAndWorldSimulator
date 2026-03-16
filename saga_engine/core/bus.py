from typing import Callable, Dict, List, Any

class EventBus:
    """The Central Nervous System of the S.A.G.A. Engine"""
    
    def __init__(self):
        # The strict event channels dictated by the architecture
        self.subscribers: Dict[str, List[Callable]] = {
            "MAP_CLICK": [],
            "PLAYER_ACTION": [],
            "SKILL_CHECK": [],
            "CLASH_ATTACK": [],
            "INVESTIGATE_SEED": [],
            "STATE_UPDATE": []
        }

    def subscribe(self, event_type: str, callback: Callable):
        """Allows logic organs (like the rules_engine) to listen for specific events."""
        if event_type in self.subscribers:
            self.subscribers[event_type].append(callback)
        else:
            print(f"[BUS ERROR] Unrecognized event channel: {event_type}")

    def publish(self, event_type: str, payload: Any):
        """Immediately passes the payload to all subscribed functions."""
        if event_type in self.subscribers:
            for callback in self.subscribers[event_type]:
                callback(payload)
