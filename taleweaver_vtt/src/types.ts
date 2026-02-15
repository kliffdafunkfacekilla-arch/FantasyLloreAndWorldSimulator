export type Position = [number, number]; // [x, y] on tactical grid

export interface Entity {
    id: string;
    name: string;
    type: 'player' | 'enemy' | 'npc';
    pos: Position;
    hp: number;
    maxHp: number;
    icon: string;
    tags: string[];
}

export interface MapData {
    width: number;
    height: number;
    grid: number[][]; // 0 = floor, 1 = wall
    biome: 'forest' | 'dungeon' | 'wilderness';
}

export interface SessionData {
    meta: {
        title: string;
        description: string;
        turn: number;
        world_pos: [number, number];
    };
    map: MapData;
    entities: Entity[];
    log: string[];
}
