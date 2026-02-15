import { create } from 'zustand';
import type { SessionData, Position } from './types';

interface GameState extends SessionData {
    loadSession: (data: SessionData) => void;
    moveEntity: (id: string, newPos: Position) => void;
    addLog: (message: string) => void;
    selectedEntityId: string | null;
    selectEntity: (id: string | null) => void;
    fetchNewSession: (x?: number, y?: number) => Promise<void>;
    submitResult: (outcome: 'VICTORY' | 'DEFEAT') => Promise<void>;
}

export const useGameStore = create<GameState>((set, get) => ({
    meta: { title: "Initializing...", description: "", turn: 1, world_pos: [500, 500] },
    map: { width: 20, height: 20, grid: [], biome: 'wilderness' },
    entities: [],
    log: ["Tactical Interface initialized."],
    selectedEntityId: null,

    loadSession: (data) => set({ ...data }),

    moveEntity: (id, [x, y]) => set((state) => ({
        entities: state.entities.map(e =>
            e.id === id ? { ...e, pos: [x, y] } : e
        )
    })),

    addLog: (msg) => set((state) => ({
        log: [...state.log, `[T${state.meta.turn}] ${msg}`]
    })),

    selectEntity: (id) => set({ selectedEntityId: id }),

    fetchNewSession: async (x = 500, y = 500) => {
        try {
            // Using /api proxy prefix defined in vite.config.ts
            const response = await fetch(`/api/tactical/generate?x=${x}&y=${y}`);
            if (!response.ok) throw new Error('Failed to fetch session');
            const data = await response.json();
            set({ ...data, selectedEntityId: null });
            set((state) => ({ log: [...state.log, `[SYSTEM] Battle Map Loaded: ${data.meta.title}`] }));
        } catch (e) {
            console.error("Brain Server is offline!", e);
            set((state) => ({ log: [...state.log, "[ERROR] Brain Server is offline!"] }));
        }
    },

    submitResult: async (outcome) => {
        const { entities, meta } = get();
        const deadEnemies = entities.filter(e => e.hp <= 0 && e.type === 'enemy').map(e => e.id);

        try {
            const response = await fetch('/api/tactical/feedback', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    outcome,
                    enemies_killed: deadEnemies,
                    loot_taken: [],
                    x: meta.world_pos[0],
                    y: meta.world_pos[1]
                })
            });
            const data = await response.json();
            set((state) => ({ log: [...state.log, `[REPORT] ${data.message}`] }));
        } catch (e) {
            console.error("Failed to submit result", e);
        }
    }
}));
