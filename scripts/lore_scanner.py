"""
SAGA Lore Auto-Tagger
Reads lore.json, analyzes each entry's CONTENT to determine
what type of entity it describes, then sets:
  - categoryName + cat (Bio, Faction, Resource, Locations, etc.)
  - isAgent / isFaction / isResource flags
  - tags[] for searchability

Also crawls data/lore/ for raw text files and generates suggestions.
Writes suggestions.json for the Smart Import popup.

Usage:
  python scripts/lore_scanner.py                    # uses default paths
  python scripts/lore_scanner.py --data-dir <path>  # specify data directory
"""
import json
import os
import sys

# ============================================================
# PATH RESOLUTION
# ============================================================
def get_paths():
    """Determine the data directory and raw lore directory."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    
    # Raw lore directory is always project_root/data/lore
    raw_dir = os.path.join(project_root, "data", "lore")

    # Check for explicit --data-dir argument
    data_dir = None
    for i, arg in enumerate(sys.argv):
        if arg == "--data-dir" and i + 1 < len(sys.argv):
            data_dir = sys.argv[i + 1].strip('"').strip("'")
            break

    if not data_dir:
        # Check SAGA_DATA_HUB environment variable
        env_hub = os.environ.get("SAGA_DATA_HUB", "").strip('"').strip("'")
        if env_hub and os.path.exists(os.path.join(env_hub, "lore.json")):
            data_dir = env_hub
        else:
            # Check default SAGA_Global_Data location
            saga_data = os.path.join(os.path.dirname(project_root), "SAGA_Global_Data")
            if os.path.exists(os.path.join(saga_data, "lore.json")):
                data_dir = saga_data
            else:
                # Fall back to local data/ directory
                data_dir = os.path.join(project_root, "data")
                
    return data_dir, raw_dir


DATA_DIR, RAW_LORE_DIR = get_paths()
LORE_PATH = os.path.join(DATA_DIR, "lore.json")
BACKUP_PATH = os.path.join(DATA_DIR, "lore.json.pretag")
SUGGESTIONS_PATH = os.path.join(DATA_DIR, "suggestions.json")

# ============================================================
# CLASSIFICATION RULES
# ============================================================
RULES = [
    {
        "name": "Faction",
        "category": "Faction",
        "flag": "isFaction",
        "tags": ["faction", "organization"],
        "keywords": [
            "faction dossier", "faction doss", "political entity",
            "founded by", "territories", "governance",
            "diplomatic", "sovereignty", "allegiance",
            "faction overview", "seat of power"
        ],
        "broad_keywords": [
            "kingdom", "empire", "clan", "tribe", "guild",
            "order", "dynasty", "alliance", "council",
            "republic", "nation", "dominion", "territory",
            "ruler", "leader", "capital", "army", "military",
            "political", "govern", "ruled", "banner",
            "vassal", "lord", "chief", "warlord"
        ],
        "strong_threshold": 1,
        "broad_threshold": 4,
    },
    {
        "name": "Creature",
        "category": "Bio",
        "flag": "isAgent",
        "tags": ["creature", "species"],
        "keywords": [
            "species profile", "species overview", "biological",
            "taxonomy", "classification:", "genus:",
            "life cycle", "population dynamics"
        ],
        "broad_keywords": [
            "species", "creature", "beast", "predator", "prey",
            "herbivore", "carnivore", "omnivore", "lifespan",
            "habitat", "diet", "breeding", "offspring",
            "anatomy", "physiology", "behavior", "behaviour",
            "herd", "pack", "swarm", "colony", "flock",
            "migration", "nocturnal", "diurnal", "venomous",
            "scales", "fur", "feathers", "claws", "fangs",
            "nest", "burrow", "lair", "den"
        ],
        "strong_threshold": 1,
        "broad_threshold": 4,
    },
    {
        "name": "Resource",
        "category": "Resource",
        "flag": "isResource",
        "tags": ["resource", "material"],
        "keywords": [
            "resource profile", "material properties",
            "trade good", "raw material", "extraction",
            "refinement process", "commodity"
        ],
        "broad_keywords": [
            "ore", "mineral", "metal", "wood", "timber",
            "crystal", "gem", "herb", "reagent", "fuel",
            "material", "harvest", "mine", "mining",
            "quarry", "forge", "smelt", "alloy", "ingot",
            "trade", "export", "import", "rarity",
            "ingredient", "potion", "elixir"
        ],
        "strong_threshold": 1,
        "broad_threshold": 4,
    },
    {
        "name": "Location",
        "category": "Locations",
        "flag": None,
        "tags": ["location", "geography"],
        "keywords": [
            "geography", "cartography", "region profile",
            "geographic overview", "terrain type",
            "coordinates", "bordered by"
        ],
        "broad_keywords": [
            "biome", "forest", "desert", "mountain", "ocean",
            "tundra", "jungle", "swamp", "terrain", "climate",
            "ecosystem", "river", "valley", "plateau",
            "island", "coastal", "volcanic", "glacier",
            "north", "south", "east", "west", "border"
        ],
        "strong_threshold": 1,
        "broad_threshold": 5,
    },
]

FALLBACK_CATEGORIES = {
    "magic": {"category": "Magic and Religion", "tags": ["magic", "religion"]},
    "spell": {"category": "Magic and Religion", "tags": ["magic"]},
    "ritual": {"category": "Magic and Religion", "tags": ["magic"]},
    "technology": {"category": "Tech", "tags": ["technology"]},
    "invention": {"category": "Tech", "tags": ["technology"]},
    "history": {"category": "History", "tags": ["history", "event"]},
    "war of": {"category": "History", "tags": ["history", "military"]},
    "battle of": {"category": "History", "tags": ["history", "military"]},
    "culture": {"category": "Cultures", "tags": ["culture", "civilization"]},
    "customs": {"category": "Cultures", "tags": ["culture"]},
    "tradition": {"category": "Cultures", "tags": ["culture"]},
}

SPECIES_TITLE_WORDS = [
    "drake", "wyrm", "wolf", "bear", "spider", "serpent",
    "hawk", "elk", "boar", "stalker", "grazer", "crawler",
    "beetle", "worm", "bat", "hound", "stag", "raptor",
    "viper", "scorpion", "crab", "leviathan", "hydra",
    "basilisk", "chimera", "griffin", "phoenix", "wyvern"
]


def classify_entry_text(title, content):
    """Analyze a lore entry's content and return classification."""
    full_text = title.lower() + " " + content.lower()

    best_category = None
    flags = {}
    tags = set()

    # --- PASS 1: Rule-based classification ---
    for rule in RULES:
        strong_hits = sum(1 for kw in rule["keywords"] if kw in full_text)
        broad_hits = sum(1 for kw in rule["broad_keywords"] if kw in full_text)

        matched = (strong_hits >= rule["strong_threshold"] or
                   broad_hits >= rule["broad_threshold"])

        if matched:
            if rule["flag"]:
                flags[rule["flag"]] = True
            tags.update(rule["tags"])
            if best_category is None:
                best_category = rule["category"]

    # --- PASS 2: Fallback category ---
    if best_category is None:
        for keyword, info in FALLBACK_CATEGORIES.items():
            if keyword in full_text:
                best_category = info["category"]
                tags.update(info["tags"])
                break

    # --- PASS 3: People detection ---
    people_indicators = [
        "born in", "died in", "biography", "life story",
        "childhood", "apprentice", "mentor", "rival",
        "hero", "villain", "king ", "queen ", "prince",
        "knight", "wizard", "mage", "warrior"
    ]
    people_hits = sum(1 for kw in people_indicators if kw in full_text)
    if people_hits >= 3 and best_category is None:
        best_category = "People"
        tags.update(["npc", "character"])

    # --- PASS 4: Title heuristics ---
    title_words = title.lower().split()
    for sw in SPECIES_TITLE_WORDS:
        if sw in title_words:
            flags["isAgent"] = True
            tags.add("creature")
            break

    # --- PASS 5: Content header detection ---
    if "## faction doss" in full_text or "faction dossier" in full_text:
        flags["isFaction"] = True
        tags.add("faction")

    return best_category, flags, tags


def main():
    print(f"[TAGGER] Data directory: {DATA_DIR}")
    print(f"[TAGGER] Raw lore dir:  {RAW_LORE_DIR}")

    suggestions = []

    # 1. Process existing lore.json
    if os.path.exists(LORE_PATH):
        with open(LORE_PATH, "r", encoding="utf-8") as f:
            lore = json.load(f)

        print(f"[TAGGER] Analyzing {len(lore)} entries in {LORE_PATH}...")

        # Backup original (only first time)
        if not os.path.exists(BACKUP_PATH):
            with open(BACKUP_PATH, "w", encoding="utf-8") as f:
                json.dump(lore, f, indent=2, ensure_ascii=False)
            print(f"[TAGGER] Backup saved to {BACKUP_PATH}")

        stats = {"tagged": 0, "unchanged": 0}
        
        for entry in lore:
            title = entry.get("title", "")
            content = entry.get("content", "")
            new_cat, new_flags, new_tags = classify_entry_text(title, content)
            changed = False

            # Set category (only if currently General)
            current_cat = entry.get("categoryName", entry.get("cat", "General"))
            if current_cat == "General" and new_cat:
                entry["categoryName"] = new_cat
                entry["cat"] = new_cat
                changed = True
            else:
                entry["categoryName"] = current_cat
                entry["cat"] = current_cat

            # Apply flags
            for key, val in new_flags.items():
                if val and not entry.get(key, False):
                    entry[key] = True
                    changed = True

            # Merge tags
            existing = set(entry.get("tags", []))
            additions = new_tags - existing
            if additions:
                entry["tags"] = sorted(existing | new_tags)
                changed = True

            if changed:
                stats["tagged"] += 1
                flag_type = "UNKNOWN"
                if entry.get("isAgent"): flag_type = "AGENT"
                if entry.get("isFaction"): flag_type = "FACTION"
                if entry.get("isResource"): flag_type = "RESOURCE"
                suggestions.append({
                    "name": title,
                    "type": flag_type,
                    "source": "lore.json"
                })
            else:
                stats["unchanged"] += 1

        # Save tagged lore
        with open(LORE_PATH, "w", encoding="utf-8") as f:
            json.dump(lore, f, indent=2, ensure_ascii=False)
        print(f"[TAGGER] Updated {stats['tagged']} entries in lore.json")

    # 2. Scan raw lore folder (new files)
    if os.path.exists(RAW_LORE_DIR):
        print(f"[TAGGER] Scanning for new files in {RAW_LORE_DIR}...")
        raw_files = [f for f in os.listdir(RAW_LORE_DIR) if f.endswith((".md", ".txt"))]
        
        for filename in raw_files:
            file_path = os.path.join(RAW_LORE_DIR, filename)
            with open(file_path, "r", encoding="utf-8") as f:
                content = f.read()
            
            title = os.path.splitext(filename)[0]
            _, flags, _ = classify_entry_text(title, content)
            
            flag_type = "UNKNOWN"
            if flags.get("isAgent"): flag_type = "AGENT"
            if flags.get("isFaction"): flag_type = "FACTION"
            if flags.get("isResource"): flag_type = "RESOURCE"
            
            suggestions.append({
                "name": title,
                "type": flag_type,
                "source": filename
            })

    # 3. Save suggestions for Smart Import popup
    with open(SUGGESTIONS_PATH, "w", encoding="utf-8") as f:
        json.dump(suggestions, f, indent=2, ensure_ascii=False)

    print(f"[TAGGER] Total suggestions generated: {len(suggestions)}")
    print(f"[TAGGER] suggestions.json saved to {SUGGESTIONS_PATH}")


if __name__ == "__main__":
    main()
