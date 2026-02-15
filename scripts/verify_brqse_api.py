
import urllib.request
import json

BASE_URL = "http://localhost:8000"

def test_endpoint(path, data=None, method="GET"):
    url = f"{BASE_URL}{path}"
    print(f"[TEST] {method} {url}...")
    try:
        req = urllib.request.Request(url, method=method)
        if data:
            req.add_header('Content-Type', 'application/json')
            body = json.dumps(data).encode('utf-8')
            with urllib.request.urlopen(req, body, timeout=5) as response:
                return json.loads(response.read().decode())
        else:
            with urllib.request.urlopen(req, timeout=5) as response:
                return json.loads(response.read().decode())
    except Exception as e:
        print(f"[FAIL] {e}")
        return None

if __name__ == "__main__":
    # 1. Create Campaign
    new_camp = test_endpoint("/campaign/new", {"hero_name": "Alaric", "theme": "Assassination Conspiracy"}, method="POST")
    if new_camp:
        print(f"[PASS] Created Campaign: {new_camp}")
    
    # 2. Check Active
    active = test_endpoint("/campaign/active")
    if active:
        print(f"[PASS] Active Campaign Status: {active.get('status', 'OK')}")
        print(f"       Theme: {active.get('theme')}")
    
    # 3. Check POIs
    pois = test_endpoint("/campaign/pois?x=500&y=500")
    if pois is not None:
        print(f"[PASS] Fetched {len(pois)} story seeds near center.")
        if pois:
            # 4. Trigger POI
            poi_id = pois[0]['id']
            trigger = test_endpoint(f"/campaign/trigger/{poi_id}", method="POST")
            if trigger:
                print(f"[PASS] Triggered Side Quest: {trigger.get('quest', {}).get('title')}")
