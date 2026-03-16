from saga_engine.core.state import CoreAttributes, VitalsState

def calculate_pools(stats: CoreAttributes) -> VitalsState:
    """Derives the 4 survival pools directly from the 12-stat chassis."""
    return VitalsState(
        # Body Pools
        max_hp = stats.might + stats.reflexes + stats.vitality,
        current_hp = stats.might + stats.reflexes + stats.vitality,
        
        max_stamina = stats.endurance + stats.fortitude + stats.finesse,
        current_stamina = stats.endurance + stats.fortitude + stats.finesse,
        
        # Mind Pools
        max_composure = stats.willpower + stats.logic + stats.awareness,
        current_composure = stats.willpower + stats.logic + stats.awareness,
        
        max_focus = stats.knowledge + stats.charm + stats.intuition,
        current_focus = stats.knowledge + stats.charm + stats.intuition
    )

def apply_holding_fees(vitals: VitalsState, loadout: dict) -> VitalsState:
    """Calculates resource locks based on equipped gear."""
    
    # Physical Holding Fees (e.g., Heavy Plate locks Stamina)
    if loadout.get("armor") == "Plate Armor":
        vitals.max_stamina -= 5  # Locks 5 Stamina
        vitals.current_stamina = min(vitals.current_stamina, vitals.max_stamina)
        
    # Mental Holding Fees (e.g., Robes/Cloaks lock Focus)
    if loadout.get("armor") == "Robes/Cloaks":
        vitals.max_focus -= 4    # Locks 4 Focus
        vitals.current_focus = min(vitals.current_focus, vitals.max_focus)
        
    return vitals
