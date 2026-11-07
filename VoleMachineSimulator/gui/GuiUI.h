#pragma once
// ============================================================
//  GuiUI.h  —  Vole Machine Simulator  •  GUI module public API
// ============================================================
// Call runGuiApp() from main.cpp; it owns the Win32 window,
// DirectX 11 device, and ImGui render loop. Includes built-in
// preset programs modal for easy test-program tracing.
// ============================================================

/// @brief Creates the Win32 window, initialises DirectX 11 + Dear ImGui,
///        runs the message/render loop, and returns when the user exits.
/// @return 0 on clean exit, 1 on D3D init failure.
int runGuiApp();
