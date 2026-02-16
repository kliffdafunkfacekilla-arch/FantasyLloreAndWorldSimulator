import { useEffect } from 'react';
import { MapGrid } from './components/MapGrid';
import { useGameStore } from './store';
import { Terminal, Scroll, Skull, Shield, Sword } from 'lucide-react';

function App() {
    const { meta, log, fetchNewSession, submitResult, entities } = useGameStore();

    useEffect(() => {
        fetchNewSession();
    }, []);

    const player = entities.find(e => e.type === 'player');

    return (
        <div className="flex h-screen bg-[#0f0f13] text-slate-200 font-sans select-none">

            {/* SIDEBAR: NAVIGATION */}
            <div className="w-16 bg-[#050508]/90 backdrop-blur-xl border-r border-white/5 flex flex-col items-center py-6 gap-6 shadow-[20px_0_50px_rgba(0,0,0,0.3)]">
                <div className="w-10 h-10 bg-gradient-to-br from-yellow-500 to-amber-700 rounded-lg flex items-center justify-center shadow-[0_0_15px_rgba(245,158,11,0.3)] border border-yellow-400/20 group cursor-pointer hover:scale-105 transition-transform">
                    <Sword size={22} className="text-white drop-shadow-md" />
                </div>
                <button className="p-2.5 text-slate-500 hover:text-yellow-500 hover:bg-white/5 rounded-xl transition-all"><Scroll size={22} /></button>
                <button className="p-2.5 text-slate-500 hover:text-yellow-500 hover:bg-white/5 rounded-xl transition-all"><Shield size={22} /></button>
                <div className="flex-grow" />
                <button className="p-2.5 text-slate-500 hover:text-white hover:bg-white/5 rounded-xl transition-all"><Terminal size={22} /></button>
            </div>

            {/* MAIN VIEWPORT */}
            <div className="flex-grow relative flex flex-col">

                {/* HUD: TOP BAR */}
                <div className="h-20 border-b border-white/5 bg-[#050508]/40 backdrop-blur-xl flex items-center px-8 justify-between z-10">
                    <div className="flex flex-col gap-0.5">
                        <h1 className="text-xs font-black tracking-[0.3em] text-yellow-500/80 uppercase drop-shadow-[0_0_8px_rgba(234,179,8,0.3)]">
                            {meta.title || "ESTABLISHING LINK..."}
                        </h1>
                        <p className="text-[10px] text-slate-400/60 uppercase font-black tracking-widest">{meta.description || "Synthesizing tactical data from SAGA Brain..."}</p>
                    </div>

                    {player && (
                        <div className="flex items-center gap-6 bg-[#12121a]/80 px-6 py-2.5 rounded-xl border border-white/5 shadow-2xl">
                            <div className="text-right">
                                <div className="text-[9px] font-black text-slate-500 uppercase tracking-tighter">Combat Vitality</div>
                                <div className="text-sm font-black text-green-400 drop-shadow-[0_0_5px_rgba(74,222,128,0.3)]">{player.hp} / {player.maxHp} HP</div>
                            </div>
                            <div className="w-32 h-1.5 bg-black/40 rounded-full overflow-hidden border border-white/5">
                                <div
                                    className="h-full bg-gradient-to-r from-green-600 via-green-400 to-green-500 animate-pulse"
                                    style={{ width: `${(player.hp / player.maxHp) * 100}%` }}
                                />
                            </div>
                        </div>
                    )}
                </div>

                {/* THE BATTLE MAP */}
                <div className="flex-grow overflow-auto flex items-center justify-center bg-[#0d0d12] p-12">
                    <MapGrid />
                </div>

                {/* FOOTER CONTROLS */}
                <div className="absolute bottom-6 left-1/2 -translate-x-1/2 flex gap-4 z-50">
                    <button
                        onClick={() => fetchNewSession()}
                        className="bg-slate-800 hover:bg-slate-700 text-[10px] font-black uppercase tracking-tighter px-6 py-2 rounded-sm border border-white/10 transition-all active:scale-95"
                    >
                        Redeploy
                    </button>
                    <button
                        onClick={() => submitResult('VICTORY')}
                        className="bg-yellow-600 hover:bg-yellow-500 text-[10px] font-black uppercase tracking-tighter px-8 py-2 rounded-sm border border-yellow-400/20 text-white transition-all active:scale-95 shadow-xl shadow-yellow-900/20"
                    >
                        Confirm Objective
                    </button>
                </div>
            </div>

            {/* ADVENTURE LOG */}
            <div className="w-96 bg-[#050508]/95 backdrop-blur-2xl border-l border-white/5 flex flex-col shadow-[-20px_0_50px_rgba(0,0,0,0.4)]">
                <div className="p-6 border-b border-white/5 flex items-center justify-between">
                    <div className="flex items-center gap-3">
                        <div className="p-1.5 bg-yellow-500/10 rounded-lg border border-yellow-500/20">
                            <Terminal size={14} className="text-yellow-500" />
                        </div>
                        <h2 className="text-[11px] font-black uppercase tracking-[0.2em] text-slate-300">Chronicle</h2>
                    </div>
                    <div className="text-[10px] font-bold text-slate-600 bg-white/5 px-2 py-0.5 rounded border border-white/5">Turn {meta.turn}</div>
                </div>

                <div className="flex-grow p-6 overflow-y-auto space-y-4 font-mono text-[11px] custom-scrollbar">
                    {log.map((entry, i) => (
                        <div key={i} className="group animate-in fade-in slide-in-from-right-2 duration-500">
                            <div className="flex items-start gap-3 leading-relaxed border-l border-white/10 pl-4 py-2 hover:bg-white/5 rounded-r-lg transition-colors">
                                <div className="mt-1 w-1 h-1 rounded-full bg-yellow-600/50 group-hover:bg-yellow-500" />
                                <span className="text-slate-400 group-hover:text-slate-200 transition-colors uppercase tracking-tight">{entry}</span>
                            </div>
                        </div>
                    ))}
                </div>

                <div className="p-8 bg-gradient-to-t from-[#050508] to-transparent">
                    <button className="w-full bg-[#1a1a24] hover:bg-yellow-600 hover:text-white text-yellow-500 text-[11px] font-black uppercase tracking-[0.3em] py-4 rounded-xl border border-white/5 transition-all active:scale-95 shadow-xl hover:shadow-yellow-900/20">
                        End Turn
                    </button>
                    <p className="mt-4 text-center text-[9px] font-bold text-slate-600 uppercase tracking-widest opacity-50">SAGA Strategic Protocol V1.0</p>
                </div>
            </div>

        </div>
    );
}

export default App;
