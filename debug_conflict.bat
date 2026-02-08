@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"
clang++ -c src/simulation/ConflictSystem.cpp -I include -std=c++17 -o conflict_test.o > conflict_debug.log 2>&1
if %ERRORLEVEL% EQU 0 (echo SUCCESS) else (echo FAILURE)
