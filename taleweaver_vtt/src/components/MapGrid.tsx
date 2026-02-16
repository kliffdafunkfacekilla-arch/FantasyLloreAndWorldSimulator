import React from 'react';
import { useGameStore } from '../store';
import { clsx } from 'clsx';

const TILE_SIZE = 48;

export const MapGrid: React.FC = () => {
    const { map, entities, selectedEntityId, moveEntity, selectEntity, addLog } = useGameStore();

    const handleTileClick = (x: number, y: number) => {
        if (selectedEntityId) {
            const entity = entities.find(e => e.id === selectedEntityId);
            if (map.grid[y] && map.grid[y][x] === 1) {
                addLog("Blocked! You cannot move into a wall.");
                return;
            }
            if (entity) {
                moveEntity(selectedEntityId, [x, y]);
                selectEntity(null);
            }
        }
    };

    const handleEntityClick = (e: React.MouseEvent, id: string) => {
        e.stopPropagation();
        selectEntity(id);
    };

    // Asset logic
    const getFloorAsset = () => {
        if (map.biome === 'forest') return '/floor/grass_0_old.png';
        return '/floor/grey_dirt_0.png';
    };

    const getWallAsset = () => {
        if (map.biome === 'forest') return '/trees/tree_1_red.png';
        return '/wall/brick_dark_0.png';
    };

    return (
        <div
            className="relative bg-[#050508] border-8 border-[#1a1a24] shadow-[0_0_50px_rgba(0,0,0,0.8)] rounded-sm overflow-hidden"
            style={{
                width: map.width * TILE_SIZE,
                height: map.height * TILE_SIZE,
                backgroundImage: `url(${getFloorAsset()})`,
                backgroundSize: `${TILE_SIZE}px ${TILE_SIZE}px`
            }}
        >
            {/* 1. RENDER GRID OVERLAY */}
            <div
                className="absolute inset-0 pointer-events-none opacity-20"
                style={{
                    backgroundImage: `linear-gradient(to right, #444 1px, transparent 1px), linear-gradient(to bottom, #444 1px, transparent 1px)`,
                    backgroundSize: `${TILE_SIZE}px ${TILE_SIZE}px`
                }}
            />

            {/* 2. RENDER WALLS/OBJECTS */}
            {map.grid.map((row, y) => (
                row.map((cellType, x) => {
                    if (cellType === 0) return null;
                    return (
                        <div
                            key={`${x}-${y}`}
                            className="absolute pointer-events-none z-10"
                            style={{
                                left: x * TILE_SIZE,
                                top: y * TILE_SIZE,
                                width: TILE_SIZE,
                                height: TILE_SIZE,
                                backgroundImage: `url(${getWallAsset()})`,
                                backgroundSize: 'cover',
                                filter: 'drop-shadow(0 4px 4px rgba(0,0,0,0.5))'
                            }}
                        />
                    );
                })
            ))}

            {/* 3. CLICKABLE LAYER */}
            {map.grid.map((row, y) => (
                row.map((_, x) => (
                    <div
                        key={`click-${x}-${y}`}
                        onClick={() => handleTileClick(x, y)}
                        className="absolute cursor-pointer hover:bg-white/10 transition-colors z-0"
                        style={{
                            left: x * TILE_SIZE,
                            top: y * TILE_SIZE,
                            width: TILE_SIZE,
                            height: TILE_SIZE,
                        }}
                    />
                ))
            ))}

            {/* 4. RENDER ENTITIES */}
            {entities.map((entity) => (
                <div
                    key={entity.id}
                    onClick={(e) => handleEntityClick(e, entity.id)}
                    className={clsx(
                        "absolute z-20 transition-all duration-300 ease-out cursor-pointer group",
                        selectedEntityId === entity.id && "z-30 scale-110"
                    )}
                    style={{
                        left: entity.pos[0] * TILE_SIZE,
                        top: entity.pos[1] * TILE_SIZE,
                        width: TILE_SIZE,
                        height: TILE_SIZE,
                    }}
                >
                    {/* Pulsing selection ring */}
                    {selectedEntityId === entity.id && (
                        <div className="absolute inset-0 rounded-full border-4 border-yellow-400 animate-pulse sky-glow shadow-[0_0_20px_#eab308]" />
                    )}

                    <div className="w-full h-full flex items-center justify-center p-1">
                        <img
                            src={`/${entity.icon}`}
                            alt={entity.name}
                            className={clsx(
                                "w-full h-full object-contain drop-shadow-[0_4px_4px_rgba(0,0,0,0.8)]",
                                entity.type === 'enemy' && "sepia-[0.3] hue-rotate-[320deg]"
                            )}
                            onError={(e) => {
                                // Fallback if icon fails
                                (e.target as HTMLImageElement).src = 'icons/race/human_male.png';
                            }}
                        />
                    </div>

                    {/* Minimal HP Bar */}
                    <div className="absolute -bottom-1 left-2 right-2 h-1.5 bg-black/80 border border-white/10 rounded-full overflow-hidden shadow-lg">
                        <div
                            className={clsx("h-full transition-all duration-500", entity.type === 'player' ? "bg-gradient-to-r from-green-600 to-green-400" : "bg-gradient-to-r from-red-600 to-red-400")}
                            style={{ width: `${(entity.hp / entity.maxHp) * 100}%` }}
                        />
                    </div>

                    {/* Name Tag (Hover) */}
                    <div className="absolute -top-6 left-1/2 -translate-x-1/2 bg-black/90 text-white text-[10px] px-2 py-0.5 rounded border border-white/20 whitespace-nowrap opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none">
                        {entity.name}
                    </div>
                </div>
            ))}
        </div>
    );
};
