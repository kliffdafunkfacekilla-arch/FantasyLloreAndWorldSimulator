# 🛠️ Windows C++ Development Setup Guide

You are running strictly on Windows, so the `dev.nix` file (which is for Cloud/Linux environments) will not work. You currently do not have a C++ compiler installed.

The easiest way to get `clang++` and the graphics libraries (`glfw`, `glew`) working is to use **MSYS2**. It acts like a package manager for Windows.

## Step 1: Install MSYS2
1. Download the installer from [msys2.org](https://www.msys2.org/).
2. Run the installer and click "Next" until finished.
3. When it finishes, a terminal window will open. If not, search "MSYS2 MSYS" in your Start Menu.

## Step 2: Install Dependencies
Copy and paste this command into the MSYS2 terminal and hit Enter. This installs Clang (compiler), GLFW (windowing), and GLEW (graphics):

```bash
pacman -S --noconfirm mingw-w64-x86_64-clang mingw-w64-x86_64-glfw mingw-w64-x86_64-glew mingw-w64-x86_64-make
```

## Step 3: Add to Path (Crucial!)
For your VS Code / PowerShell to see these tools, you need to add them to your Windows PATH.

1. Open Windows Search and type **"Edit the system environment variables"**.
2. Click **"Environment Variables"**.
3. Under **"User variables"** (top box), find the row named `Path` and click **Edit**.
4. Click **New** and paste this path:
   `C:\msys64\mingw64\bin`
   *(Note: If you installed MSYS2 somewhere else, adjust accordingly. `C:\msys64` is default)*.
5. Click OK -> OK -> OK.
6. **Restart VS Code** completely (Close and Reopen).

## Step 4: Verify
After restarting VS Code, open a terminal and type:
```powershell
clang++ --version
```
If you see version info, you are ready to build!

## Step 5: Build the Game
Run your build script:
```powershell
.\build.bat
```
