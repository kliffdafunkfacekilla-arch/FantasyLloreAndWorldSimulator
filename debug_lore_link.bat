@echo off
set "PATH=C:\msys64\mingw64\bin;%PATH%"
clang++ -o bin/OmnisLoreKeeper.exe obj/App_Lore.o obj/EditorUI.o obj/AssetManager.o obj/LoreScribe.o obj/imgui.o obj/imgui_draw.o obj/imgui_tables.o obj/imgui_widgets.o obj/imgui_impl_glfw.o obj/imgui_impl_opengl3.o -L C:/msys64/mingw64/lib -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -DGLEW_STATIC 2> lore_link_error.txt
type lore_link_error.txt
