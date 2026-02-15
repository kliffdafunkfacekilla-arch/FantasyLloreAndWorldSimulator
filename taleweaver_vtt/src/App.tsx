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
            <div className="w-16 bg-[#08080a] border-r border-[#1e1e26] flex flex-col items-center py-6 gap-6">
                <div className="w-10 h-10 bg-yellow-600 rounded flex items-center justify-center shadow-lg shadow-yellow-900/20">
                    <Sword size={24} className="text-white" />
                </div>
                <button className="p-2 text-slate-500 hover:text-white transition-colors"><Scroll size={24} /></button>
                <button className="p-2 text-slate-500 hover:text-white transition-colors"><Shield size={24} /></button>
                <div className="flex-grow" />
                <button className="p-2 text-slate-500 hover:text-white transition-colors"><Terminal size={24} /></button>
            </div>

            {/* MAIN VIEWPORT */}
            <div className="flex-grow relative flex flex-col">

                {/* HUD: TOP BAR */}
                <div className="h-16 border-b border-[#1e1e26] bg-[#0a0a0e]/80 backdrop-blur-md flex items-center px-6 justify-between">
                    <div>
                        <h1 className="text-sm font-black tracking-widest text-yellow-500 uppercase">{meta.title || "Loading Encounter..."}</h1>
                        <p className="text-[10px] text-slate-500 uppercase font-bold tracking-tight">{meta.description}</p>
                    </div>

                    {player && (
                        <div className="flex items-center gap-4 bg-[#161621] px-4 py-2 rounded-sm border border-white/5">
                            <div className="text-right">
                                <div className="text-[10px] font-bold text-slate-500 uppercase">Vitality</div>
                                <div className="text-xs font-black text-green-400">{player.hp} / {player.maxHp} HP</div>
                            </div>
                            <div className="w-24 h-2 bg-slate-800 rounded-full overflow-hidden border border-black/20">
                                <div
                                    className="h-full bg-gradient-to-r from-green-600 to-green-400"
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
            <div className="w-80 bg-[#0a0a0e] border-l border-[#1e1e26] flex flex-col">
                <div className="p-4 border-b border-[#1e1e26] flex items-center gap-2">
                    <Terminal size={14} className="text-yellow-500" />
                    <h2 className="text-[10px] font-black uppercase tracking-widest text-slate-400">Tactical Log</h2>
                </div>
                <div className="flex-grow p-4 overflow-y-auto space-y-3 font-mono text-[11px]">
                    {log.map((entry, i) => (
                        <div key={i} className="leading-relaxed border-l-2 border-slate-800 pl-3 py-1 bg-white/5 rounded-r-sm">
                            <span className="text-yellow-600 font-bold opacity-50 select-none mr-2">LOG</span>
                            <span className="text-slate-300">{entry}</span>
                        </div>
                    ))}
                </div>
                <div className="p-6 bg-[#08080a] border-t border-[#1e1e26]">
                    <div className="text-[9px] font-bold text-slate-600 mb-2 uppercase text-center tracking-tighter">Current Turn: {meta.turn}</div>
                    <button className="w-full bg-[#161621] hover:bg-[#1e1e2e] text-[10px] font-black uppercase tracking-[0.2em] py-3 rounded-sm border border-white/5 transition-all active:bg-black">
                        Pass Turn
                    </button>
                </div>
            </div>

        </div>
    );
}

export default App;
