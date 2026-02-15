import React from 'react';
import { useGameStore } from '../store';
import { clsx } from 'clsx';

const TILE_SIZE = 48; // Represents 5ft in the tactical space

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

    return (
        <div
            className="relative bg-[#0a0a0e] border-4 border-[#2d2d3d] shadow-2xl rounded-sm overflow-hidden"
            style={{
                width: map.width * TILE_SIZE,
                height: map.height * TILE_SIZE
            }}
        >
            {/* 1. RENDER TILES */}
            {map.grid.map((row, y) => (
                row.map((cellType, x) => {
                    return (
                        <div
                            key={`${x}-${y}`}
                            onClick={() => handleTileClick(x, y)}
                            className={clsx(
                                "absolute border-[0.5px] border-white/5",
                                cellType === 1 ? "bg-gray-800" : "bg-[#181825]"
                            )}
                            style={{
                                left: x * TILE_SIZE,
                                top: y * TILE_SIZE,
                                width: TILE_SIZE,
                                height: TILE_SIZE,
                            }}
                        />
                    );
                })
            ))}

            {/* 2. RENDER ENTITIES */}
            {entities.map((entity) => (
                <div
                    key={entity.id}
                    onClick={(e) => handleEntityClick(e, entity.id)}
                    className={clsx(
                        "absolute z-20 transition-all duration-300 ease-out cursor-pointer",
                        selectedEntityId === entity.id && "ring-2 ring-yellow-400 scale-110 drop-shadow-[0_0_8px_rgba(234,179,8,0.5)]"
                    )}
                    style={{
                        left: entity.pos[0] * TILE_SIZE,
                        top: entity.pos[1] * TILE_SIZE,
                        width: TILE_SIZE,
                        height: TILE_SIZE,
                    }}
                >
                    <div className="w-full h-full flex items-center justify-center p-1">
                        <div className={clsx(
                            "w-full h-full rounded-full border-2 flex items-center justify-center text-[10px] font-bold uppercase",
                            entity.type === 'player' ? "bg-blue-900/50 border-blue-400 text-blue-100" : "bg-red-900/50 border-red-400 text-red-100"
                        )}>
                            {entity.name[0]}
                        </div>
                    </div>
                    {/* Minimal HP Bar */}
                    <div className="absolute -top-1 left-1 right-1 h-1 bg-gray-900 rounded-full overflow-hidden">
                        <div
                            className={clsx("h-full transition-all duration-300", entity.type === 'player' ? "bg-green-500" : "bg-red-500")}
                            style={{ width: `${(entity.hp / entity.maxHp) * 100}%` }}
                        />
                    </div>
                </div>
            ))}
        </div>
    );
};
