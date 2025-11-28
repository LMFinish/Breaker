# Breaker

🇺🇸 A tool designed to edit a parameter file format available in multiple Nintendo GameCube games based on the JSystem library, built using C++ and Dear ImGui.

🇧🇷 Um programa criado para editar um formato de arquivo de propriedades presente em alguns jogos do Nintendo GameCube baseados no JSystem (uma biblioteca usada em vários jogos da Nintendo), usando C++ e a interface Dear ImGui.

<img width="600" height="600" alt="Preview" src="https://github.com/user-attachments/assets/ac6df459-a537-487c-a3ad-94fdc237df20" />
<br>
<br>

(c) Kevin Andrade "LMFinish" 2025<br>
Special Thanks/Agradecimentos: [Dear ImGui](https://github.com/ocornut/imgui.git) & [L2DFileDialog](https://github.com/Limeoats/L2DFileDialog.git)

## Features (0.1 Alpha)
- ✅ Save/Load
- ✅ Parameter list
- ✅ Parameter editing
- ✅ Parameter documentation display (incomplete, but accurate where possible)

## Building (Windows + Visual Studio 2026)
1. Install [vcpkg](https://vcpkg.io/), integrate with VS and install app's dependencies
2. Fetch Dear ImGui through the project's Git submodules
3 Use VS to build solution
4. Make sure the Assets folder is located on the same folder as the program before running

## To-Do
- Cleaner code using more C++ methods
- Dedicated property editors (RGBA, float, etc.)
- Compatibility with Luigi's Mansion TH parameters
- Compatibility with Super Mario Sunshine
- Tradução PT (assim que resolver o problema com glyphs)
- Multi-DPI support
- Linux support (?)
- Logo
