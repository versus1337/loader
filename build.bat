@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 ( echo MSVC yok. "x86 Native Tools Command Prompt" ac. & exit /b 1 )
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /MT /O2 /DNDEBUG /DWIN32_LEAN_AND_MEAN /DNOMINMAX ^
 /I thirdparty /I thirdparty/imgui /I thirdparty/imgui/backends ^
 src\main_autorization.cpp ^
 src\autorization_ui\autorization_app.cpp src\autorization_ui\autorization_widgets.cpp ^
 src\cheat\proc_mem.cpp src\cheat\sig_scan.cpp src\cheat\cheat_core.cpp src\cheat\cheat_ui.cpp ^
 thirdparty\imgui\imgui.cpp thirdparty\imgui\imgui_draw.cpp ^
 thirdparty\imgui\imgui_tables.cpp thirdparty\imgui\imgui_widgets.cpp ^
 thirdparty\imgui\backends\imgui_impl_win32.cpp thirdparty\imgui\backends\imgui_impl_dx11.cpp ^
 /link /SUBSYSTEM:WINDOWS /OUT:build\loader.exe ^
 d3d11.lib dxgi.lib d3dcompiler.lib