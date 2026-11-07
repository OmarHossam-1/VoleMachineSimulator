@echo off
setlocal

:: ============================================================================
::  run_gui.bat - Vole Machine Simulator GUI Automated Build & Run Script
:: ============================================================================
::  Usage:
::    run_gui.bat            -> Launch GUI (compiles automatically if missing)
::    run_gui.bat --build    -> Build only (do not launch)
::    run_gui.bat --rebuild  -> Force clean re-compile and launch
:: ============================================================================

set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
cd /d "%ROOT_DIR%"

:: CLI Argument Parsing
set "DO_LAUNCH=1"
set "FORCE_REBUILD=0"

if /i "%~1"=="--build"        set "DO_LAUNCH=0"
if /i "%~1"=="build"          set "DO_LAUNCH=0"
if /i "%~1"=="-b"             set "DO_LAUNCH=0"

if /i "%~1"=="--rebuild"      set "FORCE_REBUILD=1"
if /i "%~1"=="rebuild"        set "FORCE_REBUILD=1"
if /i "%~1"=="--rebuild-only" (
    set "FORCE_REBUILD=1"
    set "DO_LAUNCH=0"
)

if "%FORCE_REBUILD%"=="1" (
    echo [INFO] Performing clean rebuild...
    if exist "%ROOT_DIR%\VoleMachineGUI.exe" del /f /q "%ROOT_DIR%\VoleMachineGUI.exe" >nul 2>&1
    if exist "%ROOT_DIR%\x64\Release\VoleMachineGUI.exe" del /f /q "%ROOT_DIR%\x64\Release\VoleMachineGUI.exe" >nul 2>&1
    goto :DETECT_COMPILER
)

:: ----------------------------------------------------------------------------
:: 1. Check if executable already exists
:: ----------------------------------------------------------------------------
if exist "%ROOT_DIR%\VoleMachineGUI.exe" (
    set "TARGET_EXE=%ROOT_DIR%\VoleMachineGUI.exe"
    goto :LAUNCH_OR_EXIT
)

if exist "%ROOT_DIR%\x64\Release\VoleMachineGUI.exe" (
    copy /y "%ROOT_DIR%\x64\Release\VoleMachineGUI.exe" "%ROOT_DIR%\VoleMachineGUI.exe" >nul 2>&1
    if exist "%ROOT_DIR%\VoleMachineGUI.exe" (
        set "TARGET_EXE=%ROOT_DIR%\VoleMachineGUI.exe"
    ) else (
        set "TARGET_EXE=%ROOT_DIR%\x64\Release\VoleMachineGUI.exe"
    )
    goto :LAUNCH_OR_EXIT
)

:: ----------------------------------------------------------------------------
:: 2. Auto-detect compiler (MSVC or MinGW/GCC)
:: ----------------------------------------------------------------------------
:DETECT_COMPILER
echo ============================================================================
echo   Vole Machine Simulator GUI - Automated Build ^& Run
echo ============================================================================
echo   Executable not found. Detecting compiler to build VoleMachineGUI.exe...
echo.

:: A) Check if cl.exe is already active
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Active MSVC compiler detected.
    goto :COMPILE_MSVC
)

:: B) Locate Visual Studio via vswhere.exe
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set "VS_DIR="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
        set "VS_DIR=%%i"
    )
)

if defined VS_DIR goto :INIT_MSVC

:: C) Check standard installation directories for VS
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
    goto :INIT_MSVC
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
    goto :INIT_MSVC
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise"
    goto :INIT_MSVC
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Professional\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Professional"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise"
    goto :INIT_MSVC
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\BuildTools\Common7\Tools\VsDevCmd.bat" (
    set "VS_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\BuildTools"
    goto :INIT_MSVC
)

goto :DETECT_MINGW

:INIT_MSVC
echo [INFO] Found Visual Studio at: %VS_DIR%
echo [INFO] Initializing MSVC x64 build environment...
call "%VS_DIR%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 goto :COMPILE_MSVC

:DETECT_MINGW
:: D) Check if MinGW / GCC (g++) is in PATH
where g++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Detected MinGW / GCC in PATH.
    goto :COMPILE_MINGW
)

:: Check standard MinGW directories
if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
    echo [INFO] Found MinGW at C:\msys64\ucrt64\bin
    goto :COMPILE_MINGW
)
if exist "C:\msys64\mingw64\bin\g++.exe" (
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
    echo [INFO] Found MinGW at C:\msys64\mingw64\bin
    goto :COMPILE_MINGW
)
if exist "C:\MinGW\bin\g++.exe" (
    set "PATH=C:\MinGW\bin;%PATH%"
    echo [INFO] Found MinGW at C:\MinGW\bin
    goto :COMPILE_MINGW
)
if exist "C:\mingw64\bin\g++.exe" (
    set "PATH=C:\mingw64\bin;%PATH%"
    echo [INFO] Found MinGW at C:\mingw64\bin
    goto :COMPILE_MINGW
)

:: No supported compiler found
echo.
echo ============================================================================
echo   ERROR: No compatible C++ compiler was found!
echo ============================================================================
echo   Could not detect MSVC (Visual Studio) or MinGW / GCC (g++).
echo.
echo   To compile and run Vole Machine Simulator GUI, please install either:
echo     1. Microsoft Visual Studio (2017/2019/2022) with
echo        "Desktop development with C++" workload:
echo        https://visualstudio.microsoft.com/downloads/
echo     2. Visual Studio Build Tools with C++ workload
echo     3. MinGW-w64 / MSYS2 with g++ (added to your system PATH)
echo ============================================================================
echo.
pause
exit /b 1

:: ----------------------------------------------------------------------------
:: 3. Compile Vole Machine GUI
:: ----------------------------------------------------------------------------
:COMPILE_MSVC
echo [INFO] Compiling VoleMachineGUI with MSVC...

where msbuild.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 goto :COMPILE_MSVC_CL

echo [INFO] Invoking MSBuild (Release x64)...
msbuild "%ROOT_DIR%\VoleMachineSimulator.sln" /t:VoleMachineGUI /p:Configuration=Release /p:Platform=x64 /nologo /v:m
if %ERRORLEVEL% EQU 0 (
    if exist "%ROOT_DIR%\x64\Release\VoleMachineGUI.exe" (
        copy /y "%ROOT_DIR%\x64\Release\VoleMachineGUI.exe" "%ROOT_DIR%\VoleMachineGUI.exe" >nul 2>&1
        echo [SUCCESS] Build completed successfully: VoleMachineGUI.exe
        set "TARGET_EXE=%ROOT_DIR%\VoleMachineGUI.exe"
        goto :LAUNCH_OR_EXIT
    )
)

echo [WARNING] MSBuild attempt failed. Falling back to direct cl.exe...

:COMPILE_MSVC_CL
echo [INFO] Compiling directly with cl.exe...
cl.exe /nologo /std:c++17 /EHsc /O2 /W3 /D_UNICODE /DUNICODE ^
    /I"%ROOT_DIR%\VoleMachineSimulator" ^
    /I"%ROOT_DIR%\VoleMachineSimulator\gui" ^
    /I"%ROOT_DIR%\VoleMachineSimulator\gui\imgui" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\GuiMain.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\GuiUI.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui_draw.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui_tables.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui_widgets.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\backends\imgui_impl_win32.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\backends\imgui_impl_dx11.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\ALU.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\ControlUnit.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\Instruction.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\MemoryFile.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\RegisterFile.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\Screen.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\VoleMachine.cpp" ^
    /Fe:"%ROOT_DIR%\VoleMachineGUI.exe" ^
    /link /SUBSYSTEM:WINDOWS d3d11.lib dxgi.lib d3dcompiler.lib comdlg32.lib dwmapi.lib

set "BUILD_CODE=%ERRORLEVEL%"
del /f /q "%ROOT_DIR%\*.obj" >nul 2>&1

if %BUILD_CODE% EQU 0 (
    echo [SUCCESS] Build completed successfully: VoleMachineGUI.exe
    set "TARGET_EXE=%ROOT_DIR%\VoleMachineGUI.exe"
    goto :LAUNCH_OR_EXIT
) else (
    echo [ERROR] MSVC compilation failed with exit code %BUILD_CODE%.
    pause
    exit /b %BUILD_CODE%
)

:COMPILE_MINGW
echo [INFO] Compiling VoleMachineGUI with MinGW (g++)...
g++ -std=c++17 -O2 -DUNICODE -D_UNICODE ^
    -I"%ROOT_DIR%\VoleMachineSimulator" ^
    -I"%ROOT_DIR%\VoleMachineSimulator/gui" ^
    -I"%ROOT_DIR%\VoleMachineSimulator/gui/imgui" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\GuiMain.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\GuiUI.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui_draw.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui_tables.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\imgui_widgets.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\backends\imgui_impl_win32.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\gui\imgui\backends\imgui_impl_dx11.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\ALU.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\ControlUnit.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\Instruction.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\MemoryFile.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\RegisterFile.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\Screen.cpp" ^
    "%ROOT_DIR%\VoleMachineSimulator\VoleMachine.cpp" ^
    -o "%ROOT_DIR%\VoleMachineGUI.exe" ^
    -mwindows -ld3d11 -ldxgi -ld3dcompiler -lcomdlg32 -ldwmapi

set "BUILD_CODE=%ERRORLEVEL%"
if %BUILD_CODE% EQU 0 (
    echo [SUCCESS] Build completed successfully: VoleMachineGUI.exe
    set "TARGET_EXE=%ROOT_DIR%\VoleMachineGUI.exe"
    goto :LAUNCH_OR_EXIT
) else (
    echo [ERROR] MinGW compilation failed with exit code %BUILD_CODE%.
    pause
    exit /b %BUILD_CODE%
)

:: ----------------------------------------------------------------------------
:: 4. Launch Application or Exit
:: ----------------------------------------------------------------------------
:LAUNCH_OR_EXIT
if "%DO_LAUNCH%"=="1" (
    echo [INFO] Launching Vole Machine GUI...
    start "" "%TARGET_EXE%"
)
exit /b 0
