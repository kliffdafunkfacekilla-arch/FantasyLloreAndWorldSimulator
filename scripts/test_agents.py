import sys
import os

# Add project root to sys.path so we can import src.agents
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from src.agents.director import DirectorAgent
from src.agents.lawyer import LawyerAgent
from src.agents.narrator import NarratorAgent

def main():
    print("--- SAGA Agent Test ---")
    
    # Initialize Agents
    director = DirectorAgent()
    lawyer = LawyerAgent()
    narrator = NarratorAgent()
    
    # 1. Director sets the scene
    print("\n[TEST] Director establishing scene at 100, 100...")
    scene = director.establish_scene(100, 100)
    print(scene)
    
    # 2. Lawyer checks a faction
    print("\n[TEST] Lawyer checking jurisdiction for 'The Iron Caldera'...")
    verdict = lawyer.check_jurisdiction("The Iron Caldera")
    print(verdict)
    
    # 3. Narrator tells the story
    print("\n[TEST] Narrator describing the scene...")
    story = narrator.narrate(scene)
    print(story)

if __name__ == "__main__":
    main()
