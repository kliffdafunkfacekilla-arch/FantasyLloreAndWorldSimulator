@echo off
TITLE SAGA Tactical VTT Launcher
echo [LAUNCHER] Starting Internal Tactical VTT...
echo [LAUNCHER] This will launch the React frontend inside FantasyLloreAndWorldSimulator.

cd taleweaver_vtt

if not exist node_modules (
    echo [LAUNCHER] Installing dependencies...
    call npm install
)

echo [LAUNCHER] Starting Frontend Server...
npm run dev

pause
