import sys
import os
import json

# Add project root to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from scripts.campaign_system import CampaignGenerator

def main():
    print("--- SAGA World Generation Test ---")
    gen = CampaignGenerator()
    
    print("\n[STEP] Generating Simulation-Flavored Campaign for hero 'Arthur'...")
    campaign = gen.create_new_campaign("Arthur")
    
    print(f"\n[SUCCESS] Campaign Created: {campaign.campaign_id}")
    print(f"Global Wealth Metric: {campaign.global_meta.get('global_wealth', 0.0)}")
    
    print("\n[QUEST LIST]")
    for i, pp in enumerate(campaign.plot_points):
        print(f"\n--- {pp.stage_name} ---")
        for q in pp.quests:
            print(f"  > {q.title}")
            print(f"    Location: {q.target_location_id or 'Unknown'}")
            print(f"    Description: {q.description}")

if __name__ == "__main__":
    main()
