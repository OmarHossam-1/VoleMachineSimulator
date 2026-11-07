// ============================================================
//  GuiUI.cpp  —  Vole Machine Simulator  •  GUI Module
//  Dear ImGui  +  DirectX 11  +  Win32  backend
// ============================================================
//  Entry point exported to main.cpp:  int runGuiApp()
//  Fonts:   Segoe UI 17pt (UI),  Consolas 16pt (tables/hex)
//  Theme:   Rich dark-slate IDE dashboard
// ============================================================

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")

#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <commdlg.h>

// ImGui headers — resolved via $(ProjectDir)gui\imgui in vcxproj
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

// Backend — resolved via $(ProjectDir) in vcxproj
#include "VoleMachine.h"
#include "Instruction.h"
#include "GuiUI.h"

// ============================================================
//  DirectX globals
// ============================================================
static ID3D11Device*           g_pd3dDevice          = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext   = nullptr;
static IDXGISwapChain*         g_pSwapChain          = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// ============================================================
//  Machine & UI state
// ============================================================
static VoleMachine g_machine;

static char  g_pcInputBuf[8]   = "0";
static char  g_instrInputBuf[8]= "";
static bool  g_showAbout       = false;
static bool  g_showHaltPopup   = false;
static bool  g_showHelp        = false;   // "Help - Operations Set" modal
static bool  g_showPresetModal = false;   // "Select Preset Program" modal
static int   g_selectedPresetIndex = 0;   // Selected preset card index (0-5)
static bool  g_fetchDone       = false;
static bool  g_decodeDone      = false;
static bool  g_hasDecoded      = false;
static int   g_instrWriteAddr  = 0;   // rolling write pointer for Add Instruction

static Instruction g_lastInstr = {0,0,0,0,0};
static std::string g_decodeMsg = "";

// ============================================================
//  Preset Programs Data & Loader
// ============================================================
struct PresetProgram {
    const char* title;
    const char* category;
    uint8_t     r, g, b;      // Pill badge accent RGB tint
    const char* description;
    std::vector<std::string> instructions;
};

static const PresetProgram kPresetPrograms[6] = {
    {
        "Memory & Register Transfer (Load, Copy, Store)",
        "Data Transfer",
        56, 189, 248, // Cyan #38BDF8
        "Loads 10 (0x0A) into R1 (Opc 2), copies R1 to R2 (Opc 4), stores to RAM[0x20] (Opc 3), loads back into R3 (Opc 1), prints 10 to Screen, and halts.",
        {"210A", "4012", "3220", "1320", "3300", "C000"}
    },
    {
        "Simple Integer Addition (10 + 20 = 30)",
        "Integer Math",
        16, 185, 129, // Emerald #10B981
        "Loads 10 (0x0A) into R1 and 20 (0x14) into R2, adds them into R3 (0x1E = 30) using Opc 5, prints 30 on the Screen, and halts.",
        {"210A", "2214", "5312", "3300", "C000"}
    },
    {
        "Simple Float Addition (0.5 + 0.25 = 0.75)",
        "Floating Point",
        168, 85, 247, // Purple #A855F7
        "Loads 0.5 (0x48) into R1 and 0.25 (0x38) into R2, adds them into R3 (0x4C = 0.75) using Opc 6, and halts.",
        {"2148", "2238", "6312", "C000"}
    },
    {
        "Basic Logic Gates (OR, AND, XOR)",
        "Bitwise Logic",
        245, 158, 11, // Amber #F59E0B
        "Loads 0x0F into R1 and 0xF0 into R2, computes OR into R3 (Opc 7), AND into R4 (Opc 8), XOR into R5 (Opc 9), and halts.",
        {"210F", "22F0", "7312", "8412", "9512", "C000"}
    },
    {
        "Bit Rotate & Conditional Jump (Opc A & B)",
        "Shift & Jump",
        59, 130, 246, // Blue #3B82F6
        "Loads 8 into R1, rotates right 1 step to become 4 (Opc A), prints 4 to Screen, then jumps over dummy instruction 2999 directly to Halt (Opc B).",
        {"2000", "2108", "A101", "3100", "B00C", "2999", "C000"}
    },
    {
        "Clear All (Reset Machine)",
        "Reset",
        239, 68, 68, // Crimson #EF4444
        "Clears all memory cells, registers, and screen output, resetting Program Counter (PC) to 0x00.",
        {}
    }
};

static void LoadPresetProgram(int index)
{
    if (index < 0 || index >= 6) return;
    const auto& preset = kPresetPrograms[index];

    // Clear all Memory cells, Registers, and Screen output, reset PC = 0x00
    g_machine.reset();

    // Reset simulator UI cycle tracking state
    g_instrWriteAddr = 0;
    g_fetchDone      = false;
    g_decodeDone     = false;
    g_hasDecoded     = false;
    g_lastInstr      = {0, 0, 0, 0, 0};
    g_decodeMsg      = "";
    g_showHaltPopup  = false;
    snprintf(g_pcInputBuf, sizeof(g_pcInputBuf), "00");

    // Write 16-bit hex instructions sequentially into Memory starting at address 0x00
    uint8_t addr = 0x00;
    for (const auto& hexStr : preset.instructions) {
        if (hexStr.length() >= 4) {
            try {
                uint8_t b0 = static_cast<uint8_t>(std::stoul(hexStr.substr(0, 2), nullptr, 16));
                uint8_t b1 = static_cast<uint8_t>(std::stoul(hexStr.substr(2, 2), nullptr, 16));
                g_machine.setMemoryCell(addr++, b0);
                g_machine.setMemoryCell(addr++, b1);
            } catch (...) {}
        }
    }
    g_instrWriteAddr = addr;
}

// ============================================================
//  Fonts  (loaded in runGuiApp before the render loop)
// ============================================================
static ImFont* g_fontUI   = nullptr;  // Segoe UI  17pt — labels, headers, buttons
static ImFont* g_fontMono = nullptr;  // Consolas  16pt — tables, hex boxes, IR/PC
static ImFont* g_fontBold = nullptr;  // Segoe UI Bold 19pt — section headers & app title

static HWND g_hWnd = nullptr;

// ============================================================
//  Colour palette  (hex → normalised float)
// ============================================================
//  #13151A  deep slate background
//  #1C1F26  card / panel surface
//  #252932  elevated input / frame
//  #2D323E  border / divider
//  #10B981  emerald  (active-PC row, run buttons)
//  #38BDF8  sky-blue accent  (title, headers, hover tints)
//  #E2E8F0  primary text
//  #94A3B8  muted  text

// ============================================================
//  Helper: byte → 8-char binary string
// ============================================================
static std::string toBinary8(uint8_t v)
{
    char buf[9]; buf[8] = '\0';
    for (int i = 7; i >= 0; --i)
        buf[7 - i] = ((v >> i) & 1) ? '1' : '0';
    return std::string(buf);
}

// ============================================================
//  Helper: 8-bit Vole float → double
//  Format: 1 sign | 3 exponent (excess-4) | 4 mantissa
// ============================================================
static double toVoleFloat(uint8_t v)
{
    if (v == 0) return 0.0;
    int sign     = (v >> 7) & 1;
    int exponent = (v >> 4) & 0x7;
    int mantissa =  v       & 0xF;
    double m     = 1.0 + static_cast<double>(mantissa) / 16.0;
    double result = m * std::pow(2.0, exponent - 4);
    return sign ? -result : result;
}

// ============================================================
//  Helper: signed int8 value
// ============================================================
static int toSignedInt(uint8_t v) { return static_cast<int>(static_cast<int8_t>(v)); }

// ============================================================
//  Helper: human-readable decode message
// ============================================================
static std::string getDecodeMessage(const Instruction& inst)
{
    char buf[256];
    switch (inst.opcode) {
    case  1: snprintf(buf, sizeof(buf),
        "LOAD Register %X from Memory[%X%X].", inst.r, inst.x, inst.y); break;
    case  2: snprintf(buf, sizeof(buf),
        "LOAD Register %X with immediate value %X%X.", inst.r, inst.x, inst.y); break;
    case  3: snprintf(buf, sizeof(buf),
        "STORE Register %X to Memory[%X%X]%s.", inst.r, inst.x, inst.y,
        ((inst.x==0&&inst.y==0)||(inst.x==0xF&&inst.y==0xF)) ? " (Screen)" : ""); break;
    case  4: snprintf(buf, sizeof(buf),
        "MOVE value from Register %X to Register %X.", inst.x, inst.y); break;
    case  5: snprintf(buf, sizeof(buf),
        "ADD (integer) Register %X = Register %X + Register %X.", inst.r, inst.x, inst.y); break;
    case  6: snprintf(buf, sizeof(buf),
        "ADD (float) Register %X = Register %X + Register %X.", inst.r, inst.x, inst.y); break;
    case  7: snprintf(buf, sizeof(buf),
        "OR Register %X = Register %X | Register %X.", inst.r, inst.x, inst.y); break;
    case  8: snprintf(buf, sizeof(buf),
        "AND Register %X = Register %X & Register %X.", inst.r, inst.x, inst.y); break;
    case  9: snprintf(buf, sizeof(buf),
        "XOR Register %X = Register %X ^ Register %X.", inst.r, inst.x, inst.y); break;
    case 10: snprintf(buf, sizeof(buf),
        "ROTATE Register %X right by %X steps.", inst.r, inst.y); break;
    case 11: snprintf(buf, sizeof(buf),
        "JUMP to address %X%X if Register %X equals Register 0.", inst.x, inst.y, inst.r); break;
    case 12: snprintf(buf, sizeof(buf), "HALT the execution."); break;
    default: snprintf(buf, sizeof(buf), "UNKNOWN opcode %X.", inst.opcode); break;
    }
    return std::string(buf);
}

// ============================================================
//  Forward declarations
// ============================================================
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void CleanupRenderTarget();
static void RenderFrame();

extern IMGUI_IMPL_API LRESULT
    ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ============================================================
//  WndProc
// ============================================================
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0,
                (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
//  DrawMemoryTable  (uses mono font)
// ============================================================
static void DrawMemoryTable(uint8_t pc)
{
    ImGui::PushFont(g_fontMono);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 3.0f));

    constexpr ImGuiTableFlags kFlags =
        ImGuiTableFlags_ScrollY          |
        ImGuiTableFlags_BordersOuter     |
        ImGuiTableFlags_BordersInnerV    |
        ImGuiTableFlags_RowBg            |
        ImGuiTableFlags_SizingStretchSame;

    if (!ImGui::BeginTable("MemTable", 5, kFlags)) {
        ImGui::PopStyleVar();
        ImGui::PopFont();
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    // Equal proportional weights — no single column dominates
    ImGui::TableSetupColumn("Address",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Binary",   ImGuiTableColumnFlags_WidthStretch, 1.6f);
    ImGui::TableSetupColumn("Hex",      ImGuiTableColumnFlags_WidthStretch, 0.7f);
    ImGui::TableSetupColumn("Int",      ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableSetupColumn("Floating", ImGuiTableColumnFlags_WidthStretch, 1.1f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.973f, 0.980f, 0.988f, 1.0f)); // #F8FAFC
    ImGui::TableHeadersRow();
    ImGui::PopStyleColor();

    // Auto-scroll to keep PC row visible when it changes
    static uint8_t s_lastPc = 255;
    bool needScroll = (s_lastPc != pc);
    s_lastPc = pc;

    ImGuiListClipper clipper;
    clipper.Begin(256);
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            uint8_t val  = g_machine.getMemoryCell(i);
            bool    isPC = (i == static_cast<int>(pc));

            ImGui::TableNextRow();

            if (isPC) {
                // Vibrant Emerald row #10B981 with deep dark text #06100C
                const ImU32 emerald = IM_COL32(16, 185, 129, 255);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, emerald);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, emerald);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(6, 16, 12, 255));
            }

            // Col 0: Address
            ImGui::TableSetColumnIndex(0);
            if (!isPC) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(148, 163, 184, 255)); // #94A3B8
            ImGui::Text("%02X", i);
            if (!isPC) ImGui::PopStyleColor();

            // Data columns: non-zero in cyan #7DD3FC, zero in muted slate #64748B
            const ImU32 dataCol = (val != 0) ? IM_COL32(125, 211, 252, 255) : IM_COL32(100, 116, 139, 255);

            ImGui::TableSetColumnIndex(1);
            if (!isPC) ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
            ImGui::Text("%s", toBinary8(val).c_str());
            if (!isPC) ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(2);
            if (!isPC) ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
            ImGui::Text("%02X", val);
            if (!isPC) ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(3);
            if (!isPC) ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
            ImGui::Text("%d", toSignedInt(val));
            if (!isPC) ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(4);
            if (!isPC) ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
            {
                double fv = toVoleFloat(val);
                if (fv == 0.0) ImGui::Text("0");
                else           ImGui::Text("%.7g", fv);
            }
            if (!isPC) ImGui::PopStyleColor();

            if (isPC) ImGui::PopStyleColor(); // pop emerald row text
        }
    }
    clipper.End();

    if (needScroll) {
        float rowH = ImGui::GetTextLineHeightWithSpacing();
        ImGui::SetScrollY(static_cast<float>(pc) * rowH);
    }

    ImGui::EndTable();
    ImGui::PopStyleVar();
    ImGui::PopFont();
}

// ============================================================
//  DrawRegisterTable  (uses mono font)
// ============================================================
static void DrawRegisterTable()
{
    ImGui::PushFont(g_fontMono);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 3.0f));

    constexpr ImGuiTableFlags kFlags =
        ImGuiTableFlags_BordersOuter     |
        ImGuiTableFlags_BordersInnerV    |
        ImGuiTableFlags_RowBg            |
        ImGuiTableFlags_SizingStretchSame;

    if (!ImGui::BeginTable("RegTable", 5, kFlags)) {
        ImGui::PopStyleVar();
        ImGui::PopFont();
        return;
    }

    ImGui::TableSetupColumn("Address",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Binary",   ImGuiTableColumnFlags_WidthStretch, 1.6f);
    ImGui::TableSetupColumn("Hex",      ImGuiTableColumnFlags_WidthStretch, 0.7f);
    ImGui::TableSetupColumn("Int",      ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableSetupColumn("Floating", ImGuiTableColumnFlags_WidthStretch, 1.1f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.973f, 0.980f, 0.988f, 1.0f)); // #F8FAFC
    ImGui::TableHeadersRow();
    ImGui::PopStyleColor();

    for (int i = 0; i < 16; ++i) {
        uint8_t val = g_machine.getRegister(i);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(148, 163, 184, 255)); // #94A3B8
        ImGui::Text("%02X", i);
        ImGui::PopStyleColor();

        // Non-zero registers in cyan #7DD3FC, zero in muted slate #64748B
        const ImU32 dataCol = (val != 0) ? IM_COL32(125, 211, 252, 255) : IM_COL32(100, 116, 139, 255);

        ImGui::TableSetColumnIndex(1);
        ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
        ImGui::Text("%s", toBinary8(val).c_str());
        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(2);
        ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
        ImGui::Text("%02X", val);
        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(3);
        ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
        ImGui::Text("%d", toSignedInt(val));
        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(4);
        ImGui::PushStyleColor(ImGuiCol_Text, dataCol);
        {
            double fv = toVoleFloat(val);
            if (fv == 0.0) ImGui::Text("0");
            else           ImGui::Text("%.7g", fv);
        }
        ImGui::PopStyleColor();
    }

    ImGui::EndTable();
    ImGui::PopStyleVar();
    ImGui::PopFont();
}

// ============================================================
//  OpenFileDialog
// ============================================================
static std::string OpenFileDialog(HWND owner)
{
    char szFile[MAX_PATH] = {0};
    OPENFILENAMEA ofn     = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = owner;
    ofn.lpstrFile   = szFile;
    ofn.nMaxFile    = sizeof(szFile);
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    ofn.Flags       = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return std::string(szFile);
    return "";
}

// ============================================================
//  UI Helper: Sleek pill section header with left accent bar
// ============================================================
static void SectionHeader(const char* label, float customWidth = 0.0f, float customHeight = 0.0f)
{
    ImFont* font = g_fontBold ? g_fontBold : (g_fontUI ? g_fontUI : ImGui::GetFont());
    if (font) ImGui::PushFont(font);
    ImVec2 textSize = ImGui::CalcTextSize(label);

    const float padX = 12.0f;
    const float padY = 7.0f;
    const float barH = (customHeight > 0.0f) ? customHeight : (textSize.y + padY * 2.0f);
    const float barW = (customWidth > 0.0f) ? customWidth : ImGui::GetContentRegionAvail().x;

    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + barW, p0.y + barH);

    // Reserve layout space in ImGui
    ImGui::Dummy(ImVec2(barW, barH));

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 1. Pill Background: Subtle dark indigo/slate #1E293B
    ImU32 bgCol = IM_COL32(30, 41, 59, 255);
    drawList->AddRectFilled(p0, p1, bgCol, 6.0f);

    // 2. Subtle border #334155
    drawList->AddRect(p0, p1, IM_COL32(51, 65, 85, 200), 6.0f, 0, 1.0f);

    // 3. Left vertical cyan accent indicator bar: 3px wide bright #38BDF8
    const float barThick = 3.0f;
    const float barMarginY = 5.0f;
    ImVec2 barMin = ImVec2(p0.x + 3.0f, p0.y + barMarginY);
    ImVec2 barMax = ImVec2(p0.x + 3.0f + barThick, p1.y - barMarginY);
    drawList->AddRectFilled(barMin, barMax, IM_COL32(56, 189, 248, 255), 2.0f);

    // 4. Crisp bright white/ice-blue bold text: #E2E8F0 (vertically centered)
    float textY = p0.y + (barH - textSize.y) * 0.5f;
    ImVec2 textPos = ImVec2(p0.x + padX + barThick + 2.0f, textY);
    drawList->AddText(textPos, IM_COL32(226, 232, 240, 255), label);
    if (font) ImGui::PopFont();

    if (customWidth == 0.0f) {
        ImGui::Spacing();
    }
}

// Push/pop large frame padding around prominent action buttons
static void PushActionButtonStyle()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
}
static void PopActionButtonStyle()
{
    ImGui::PopStyleVar();
}

// ── Helper: Draw a centered, rounded value box with recessed background ──
static void DrawCenteredValueBox(const char* valStr, float height = 30.0f)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    ImVec2 size(width, height);

    // Register bounding box with ImGui for layout and tooltip hover detection
    ImGui::Dummy(size);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    // Background: #080A0F deep recessed
    ImU32 bgCol = IM_COL32(8, 10, 15, 255);
    // Border: #3B82F6 glowing accent border
    ImU32 borderCol = IM_COL32(59, 130, 246, 215);

    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgCol, 5.0f);
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderCol, 5.0f, 0, 1.0f);

    if (g_fontMono) ImGui::PushFont(g_fontMono);
    ImVec2 textSize = ImGui::CalcTextSize(valStr);
    ImVec2 textPos(
        pos.x + (size.x - textSize.x) * 0.5f,
        pos.y + (size.y - textSize.y) * 0.5f
    );
    drawList->AddText(textPos, IM_COL32(252, 211, 77, 255), valStr); // #FCD34D bright amber
    if (g_fontMono) ImGui::PopFont();
}

// ============================================================
//  RenderFrame  — full UI each frame
// ============================================================
static void RenderFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ── Full-window host ──────────────────────────────────────────────────
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("##Host", nullptr,
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize   |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // ── Top banner ───────────────────────────────────────────────────────
    ImGui::PushFont(g_fontBold ? g_fontBold : g_fontUI);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.984f, 0.749f, 0.141f, 1.0f)); // Soft Gold #FBBF24
    ImGui::Text("  Vole Machine Simulator");
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Column widths ────────────────────────────────────────────────────
    const float totalW = ImGui::GetContentRegionAvail().x;
    const float totalH = ImGui::GetContentRegionAvail().y;
    const float col1W  = totalW * 0.325f;
    const float col2W  = totalW * 0.325f;
    const float col3W  = totalW - col1W - col2W - 20.0f;

    uint8_t  pc = g_machine.getPC();

    // ====================================================================
    //  LEFT COLUMN — Main Memory
    // ====================================================================
    ImGui::BeginChild("LeftCol", ImVec2(col1W, totalH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    SectionHeader("Main Memory");

    const float memTableH = totalH - 142.0f;  // extra room so bottom controls never clip
    ImGui::BeginChild("MemTableArea", ImVec2(0, memTableH), true);
    DrawMemoryTable(pc);
    ImGui::EndChild();

    ImGui::Spacing();

    // ── Add Instruction row ───────────────────────────────────────────────
    // Input box styled with dark background, subtle bottom underline bar, and generous width
    float availW    = ImGui::GetContentRegionAvail().x;
    float clearBtnW = ImGui::CalcTextSize("Clear Memory").x + 16.0f;
    float addBtnW   = ImGui::CalcTextSize("Add Instruction").x + 16.0f;
    float spaceW    = 6.0f * 2.0f;
    float inputW    = availW - clearBtnW - addBtnW - spaceW;
    if (inputW < 130.0f) inputW = 130.0f;

    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.043f, 0.051f, 0.067f, 1.0f)); // #0B0D11 sleek dark
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.067f, 0.078f, 0.102f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0.078f, 0.094f, 0.125f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.180f, 0.216f, 0.282f, 0.60f)); // subtle dark border
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(0.988f, 0.827f, 0.302f, 1.0f)); // #FCD34D bright amber
    ImGui::PushStyleColor(ImGuiCol_TextDisabled,   ImVec4(0.500f, 0.550f, 0.630f, 1.0f)); // readable hint
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(8.0f, 6.0f));
    ImGui::PushItemWidth(inputW);

    bool addEnterPressed = ImGui::InputTextWithHint("##instrInput", "Add Instruction",
        g_instrInputBuf, sizeof(g_instrInputBuf), ImGuiInputTextFlags_EnterReturnsTrue);

    // Subtle bottom underline accent bar:
    // Bright blue #3B82F6 when focused/hovered, dark gray #3E4656 when idle
    bool isAddFocused = ImGui::IsItemActive();
    bool isAddHovered = ImGui::IsItemHovered();
    ImVec2 addMin = ImGui::GetItemRectMin();
    ImVec2 addMax = ImGui::GetItemRectMax();
    ImU32 addLineCol = (isAddFocused || isAddHovered)
        ? IM_COL32(59, 130, 246, 255)   // #3B82F6 Bright Blue
        : IM_COL32(62, 70, 86, 255);    // #3E4656 Dark Gray
    float addLineThick = isAddFocused ? 2.0f : 1.5f;
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(addMin.x + 1.0f, addMax.y - 1.0f),
        ImVec2(addMax.x - 1.0f, addMax.y - 1.0f),
        addLineCol, addLineThick);

    ImGui::PopItemWidth();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(6);

    // Auto-convert lowercase 'a'-'f' in g_instrInputBuf to uppercase in real time
    for (int i = 0; g_instrInputBuf[i] != '\0'; ++i) {
        if (g_instrInputBuf[i] >= 'a' && g_instrInputBuf[i] <= 'f') {
            g_instrInputBuf[i] = static_cast<char>(toupper(g_instrInputBuf[i]));
        }
    }

    ImGui::SameLine(0, 6);
    bool addBtnClicked = ImGui::Button("Add Instruction", ImVec2(addBtnW, 0));

    if (addEnterPressed || addBtnClicked) {
        std::string s = g_instrInputBuf;
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
        for (char& c : s) {
            c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        }

        if (s.length() == 4) {
            try {
                int b0 = std::stoi(s.substr(0, 2), nullptr, 16);
                int b1 = std::stoi(s.substr(2, 2), nullptr, 16);
                g_machine.setMemoryCell(g_instrWriteAddr,     static_cast<uint8_t>(b0));
                g_machine.setMemoryCell(g_instrWriteAddr + 1, static_cast<uint8_t>(b1));
                g_instrWriteAddr = (g_instrWriteAddr + 2) % 256;
                memset(g_instrInputBuf, 0, sizeof(g_instrInputBuf));
            } catch (...) {}
        }
    }

    ImGui::SameLine(0, 6);
    if (ImGui::Button("Clear Memory", ImVec2(clearBtnW, 0))) {
        for (int i = 0; i < 256; ++i) g_machine.setMemoryCell(i, 0);
        g_instrWriteAddr = 0;
        g_fetchDone  = false;
        g_decodeDone = false;
        g_hasDecoded = false;
        g_lastInstr  = {0,0,0,0,0};
        g_decodeMsg  = "";
    }

    ImGui::Spacing();
    PushActionButtonStyle();

    const float leftAvailW   = ImGui::GetContentRegionAvail().x;
    const float splitSpacing = 6.0f;
    const float halfBtnW     = (leftAvailW - splitSpacing) * 0.5f;

    if (ImGui::Button("Load File", ImVec2(halfBtnW, 0))) {
        std::string file = OpenFileDialog(g_hWnd);
        if (!file.empty()) {
            g_machine.loadProgram(file, 0);
            g_instrWriteAddr = 0;
            g_fetchDone  = false;
            g_decodeDone = false;
            g_hasDecoded = false;
            g_lastInstr  = {0,0,0,0,0};
            g_decodeMsg  = "";
        }
    }

    ImGui::SameLine(0, splitSpacing);

    // Preset Programs button with subtle cyan/blue accent border (#38BDF8)
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.220f, 0.741f, 0.973f, 0.75f));
    if (ImGui::Button("Preset Programs", ImVec2(halfBtnW, 0))) {
        g_showPresetModal = true;
    }
    ImGui::PopStyleColor();

    PopActionButtonStyle();

    ImGui::EndChild(); // LeftCol
    ImGui::SameLine();

    // ====================================================================
    //  MIDDLE COLUMN — Registers + Screen
    // ====================================================================
    ImGui::BeginChild("MidCol", ImVec2(col2W, totalH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    SectionHeader("Registers");

    const float regTableH = totalH * 0.44f;
    ImGui::BeginChild("RegTableArea", ImVec2(0, regTableH), true);
    DrawRegisterTable();
    ImGui::EndChild();

    ImGui::Spacing();
    if (ImGui::Button("Clear Registers", ImVec2(-1, 0)))
        for (int i = 0; i < 16; ++i) g_machine.setRegister(i, 0);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Screen header row: sleek pill header vertically centered with Clear Screen button
    {
        float availW = ImGui::GetContentRegionAvail().x;
        const float btnW = 110.0f;
        const float headerH = 34.0f;
        const float headerW = availW - btnW - 8.0f;

        SectionHeader("Screen", headerW, headerH);
        ImGui::SameLine(0, 8);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));
        if (ImGui::Button("Clear Screen", ImVec2(btnW, headerH)))
            g_machine.clearScreen();
        ImGui::PopStyleVar();
    }
    ImGui::Spacing();

    // Screen text area — recessed #080A0F, glowing border #2A324B, vibrant cyan output
    // Dynamically sized via GetContentRegionAvail().y to align flush with bottom controls
    {
        float screenH = ImGui::GetContentRegionAvail().y;
        if (screenH < 50.0f) screenH = 50.0f;

        std::string scr   = g_machine.getScreen();
        bool        empty = scr.empty();
        if (empty) scr = "To print on the screen, use 3R00 (or 3RFF) instruction.";

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.031f, 0.039f, 0.059f, 1.0f)); // #080A0F recessed
        ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.165f, 0.196f, 0.294f, 0.90f)); // #2A324B border
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        ImGui::PushFont(g_fontMono);
        ImGui::BeginChild("ScreenArea", ImVec2(0, screenH), true);
        ImGui::PushStyleColor(ImGuiCol_Text, empty
            ? ImVec4(0.392f, 0.455f, 0.545f, 1.0f)   // #64748B muted
            : ImVec4(0.490f, 0.827f, 0.988f, 1.0f));  // #7DD3FC vibrant cyan
        ImGui::TextWrapped("%s", scr.c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    ImGui::EndChild(); // MidCol
    ImGui::SameLine();

    // ====================================================================
    //  RIGHT COLUMN — Program Counter & Controls
    // ====================================================================
    ImGui::BeginChild("RightCol", ImVec2(col3W, totalH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    SectionHeader("Program Counter");

    // PC input (sleek dark background, subtle bottom underline bar, amber text #FCD34D) + Enter PC button
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.043f, 0.051f, 0.067f, 1.0f)); // #0B0D11 sleek dark
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.067f, 0.078f, 0.102f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0.078f, 0.094f, 0.125f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.180f, 0.216f, 0.282f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(0.988f, 0.827f, 0.302f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(8.0f, 6.0f));
    ImGui::PushFont(g_fontMono);
    ImGui::PushItemWidth(75.0f);
    bool pcEnterPressed = ImGui::InputText("##pcInput", g_pcInputBuf, sizeof(g_pcInputBuf),
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    ImGui::PopFont();

    // Subtle bottom underline accent bar for PC input
    bool isPCHovered = ImGui::IsItemHovered();
    bool isPCActive  = ImGui::IsItemActive();
    ImVec2 pcMin = ImGui::GetItemRectMin();
    ImVec2 pcMax = ImGui::GetItemRectMax();
    ImU32 pcLineCol = (isPCHovered || isPCActive)
        ? IM_COL32(59, 130, 246, 255)   // #3B82F6 Bright Blue
        : IM_COL32(62, 70, 86, 255);    // #3E4656 Dark Gray
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(pcMin.x + 1.0f, pcMax.y - 1.0f),
        ImVec2(pcMax.x - 1.0f, pcMax.y - 1.0f),
        pcLineCol, (isPCActive ? 2.0f : 1.5f));

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);

    // Auto-convert lowercase 'a'-'f' in g_pcInputBuf to uppercase in real time
    for (int i = 0; g_pcInputBuf[i] != '\0'; ++i) {
        if (g_pcInputBuf[i] >= 'a' && g_pcInputBuf[i] <= 'f') {
            g_pcInputBuf[i] = static_cast<char>(toupper(g_pcInputBuf[i]));
        }
    }

    ImGui::SameLine(0, 6);
    bool pcBtnClicked = ImGui::Button("Enter PC", ImVec2(-1, 0));
    if (pcEnterPressed || pcBtnClicked) {
        std::string s = g_pcInputBuf;
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
        if (!s.empty()) {
            try {
                int v = std::stoi(s, nullptr, 16);
                g_machine.setPC(static_cast<uint8_t>(v & 0xFF));
                snprintf(g_pcInputBuf, sizeof(g_pcInputBuf), "%X", v & 0xFF);
                g_fetchDone  = false;
                g_decodeDone = false;
                g_hasDecoded = false;
                g_lastInstr  = {0,0,0,0,0};
                g_decodeMsg  = "";
            } catch (...) {}
        }
    }

    ImGui::Spacing();

    // ── Fetch (disabled after fetch until execute resets the cycle) ───────
    {
        bool ok = !g_machine.isHalted() && !g_fetchDone;
        if (!ok) ImGui::BeginDisabled();
        PushActionButtonStyle();
        if (ImGui::Button("Fetch", ImVec2(-1, 0))) {
            g_machine.fetchStep();
            g_fetchDone  = true;
            g_decodeDone = false;
            g_hasDecoded = false;
            g_lastInstr  = {0,0,0,0,0};
            g_decodeMsg  = "";
        }
        PopActionButtonStyle();
        if (!ok) ImGui::EndDisabled();
    }

    // ── Decode (disabled until fetch done) ───────────────────────────────
    {
        bool ok = g_fetchDone && !g_decodeDone;
        if (!ok) ImGui::BeginDisabled();
        PushActionButtonStyle();
        if (ImGui::Button("Decode", ImVec2(-1, 0))) {
            g_machine.decodeStep();
            g_lastInstr  = g_machine.getLastInstruction();
            g_decodeMsg  = getDecodeMessage(g_lastInstr);
            g_decodeDone = true;
            g_hasDecoded = true;
        }
        PopActionButtonStyle();
        if (!ok) ImGui::EndDisabled();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Decoded Instruction Breakdown Bar (Compact 5-column single row) ───
    {
        uint16_t ir = (g_hasDecoded ? g_lastInstr.raw_value : g_machine.getIR());
        char irBuf[8] = "????";
        char opBuf[4] = "?";
        char rBuf[4]  = "?";
        char xBuf[4]  = "?";
        char yBuf[4]  = "?";

        if (g_fetchDone || g_hasDecoded) {
            snprintf(irBuf, sizeof(irBuf), "%04X", ir);
        }

        if (g_hasDecoded) {
            snprintf(opBuf, sizeof(opBuf), "%X", (ir >> 12) & 0xF);
            snprintf(rBuf,  sizeof(rBuf),  "%X", (ir >> 8)  & 0xF);
            snprintf(xBuf,  sizeof(xBuf),  "%X", (ir >> 4)  & 0xF);
            snprintf(yBuf,  sizeof(yBuf),  "%X", ir         & 0xF);
        }

        if (ImGui::BeginTable("InstructionBreakdown", 5, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableSetupColumn("IR",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("OP",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("R",    ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("XS",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("YT",   ImGuiTableColumnFlags_WidthStretch);

            // Row 1: Centered Labels in Electric Cyan (#38BDF8)
            ImGui::TableNextRow();
            const char* colLabels[5] = { "IR", "OP Code", "R", "X / S", "Y / T" };
            for (int col = 0; col < 5; ++col) {
                ImGui::TableSetColumnIndex(col);
                float colW = ImGui::GetContentRegionAvail().x;
                float textW = ImGui::CalcTextSize(colLabels[col]).x;
                float pad = (colW - textW) * 0.5f;
                if (pad > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.220f, 0.741f, 0.973f, 1.0f)); // #38BDF8
                ImGui::TextUnformatted(colLabels[col]);
                ImGui::PopStyleColor();
            }

            // Row 2: Centered Value Boxes with Tooltips
            ImGui::TableNextRow();
            const char* colValues[5] = { irBuf, opBuf, rBuf, xBuf, yBuf };
            const char* colTooltips[5] = {
                "Instruction Register (16-bit Hex)",
                "Operation Code (Nibble 1)",
                "Register R (Nibble 2)",
                "Operand X / Register S (Nibble 3)",
                "Operand Y / Register T (Nibble 4)"
            };
            for (int col = 0; col < 5; ++col) {
                ImGui::TableSetColumnIndex(col);
                DrawCenteredValueBox(colValues[col], 30.0f);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s: %s", colLabels[col], colTooltips[col]);
            }

            ImGui::EndTable();
        }
    }

    ImGui::Spacing();

    // ── Decode message box ────────────────────────────────────────────────
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.031f, 0.039f, 0.059f, 1.0f)); // #080A0F recessed
        ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.231f, 0.510f, 0.965f, 0.60f)); // #3B82F6 subtle accent border
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        ImGui::BeginChild("DecodeMsg", ImVec2(-1, 90.0f), true);
        if (g_decodeMsg.empty())
            ImGui::TextDisabled("Decoding Message");
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.988f, 0.827f, 0.302f, 1.0f)); // #FCD34D bright amber
            ImGui::TextWrapped("%s", g_decodeMsg.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();

    // ── Execute (locked until decode done; BeginDisabled snapshot) ────────
    {
        bool was = g_decodeDone;
        if (!was) ImGui::BeginDisabled();
        PushActionButtonStyle();
        if (ImGui::Button("Execute", ImVec2(-1, 0))) {
            g_machine.executeStep();
            g_fetchDone  = false;
            g_decodeDone = false;
            if (g_machine.isHalted()) g_showHaltPopup = true;
        }
        PopActionButtonStyle();
        if (!was) ImGui::EndDisabled();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Run One Cycle  (Vibrant Emerald Gradient look) ────────────────────
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.020f, 0.588f, 0.412f, 1.0f)); // #059669
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.063f, 0.725f, 0.506f, 1.0f)); // #10B981
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.204f, 0.827f, 0.600f, 1.0f)); // #34D399
    PushActionButtonStyle();
    if (ImGui::Button("Run One Cycle", ImVec2(-1, 0))) {
        if (!g_machine.isHalted()) {
            g_machine.step();
            g_fetchDone  = false;
            g_decodeDone = false;
            g_hasDecoded = true;
            g_lastInstr  = g_machine.getLastInstruction();
            g_decodeMsg  = getDecodeMessage(g_lastInstr);
            if (g_machine.isHalted()) g_showHaltPopup = true;
        }
    }
    PopActionButtonStyle();
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    // ── Run Until Halt  (Vibrant Emerald Gradient look) ───────────────────
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.020f, 0.588f, 0.412f, 1.0f)); // #059669
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.063f, 0.725f, 0.506f, 1.0f)); // #10B981
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.204f, 0.827f, 0.600f, 1.0f)); // #34D399
    PushActionButtonStyle();
    if (ImGui::Button("Run Untill Halt", ImVec2(-1, 0))) {
        g_machine.run();
        g_fetchDone  = false;
        g_decodeDone = false;
        g_hasDecoded = true;
        g_lastInstr  = g_machine.getLastInstruction();
        g_decodeMsg  = getDecodeMessage(g_lastInstr);
        if (g_machine.isHalted()) g_showHaltPopup = true;
    }
    PopActionButtonStyle();
    ImGui::PopStyleColor(3);

    // ── Push Help / About / Exit to the bottom ───────────────────────────
    {
        const float rowH  = ImGui::GetFrameHeightWithSpacing() + 10.0f;
        const float gap   = totalH - ImGui::GetCursorPosY() - rowH;
        if (gap > 0.0f) ImGui::Dummy(ImVec2(0, gap));
    }

    // Three equal-width footer buttons
    const float btnW = (col3W - 12.0f) / 3.0f;

    if (ImGui::Button("Help", ImVec2(btnW, 0)))
        g_showHelp = true;

    ImGui::SameLine(0, 6);
    if (ImGui::Button("About", ImVec2(btnW, 0)))
        g_showAbout = true;

    ImGui::SameLine(0, 6);
    // ── Exit  (Rich Crimson #DC2626 normal, #EF4444 hover, #B91C1C active) ─
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.863f, 0.149f, 0.149f, 1.0f)); // #DC2626
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.937f, 0.267f, 0.267f, 1.0f)); // #EF4444
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.725f, 0.110f, 0.110f, 1.0f)); // #B91C1C
    if (ImGui::Button("Exit", ImVec2(-1, 0)))
        PostQuitMessage(0);
    ImGui::PopStyleColor(3);

    ImGui::EndChild(); // RightCol

    ImGui::End(); // ##Host

    // ====================================================================
    //  "Done" Halt Modal  — beep + centered info popup, no Yes/No prompt
    // ====================================================================
    if (g_showHaltPopup) {
        MessageBeep(MB_OK);
        ImGui::OpenPopup("Done");
        g_showHaltPopup = false;
    }
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Done", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        // Info badge
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.220f, 0.741f, 0.973f, 1.0f));
        ImGui::Text("  [i]  ");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("The execution has been halted.");
        ImGui::Spacing();
        // Centred OK
        const float doneOkW = 100.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - doneOkW * 0.5f);
        if (ImGui::Button("OK", ImVec2(doneOkW, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ====================================================================
    //  About Popup — centered (ImGuiCond_Always), 2-col team table
    // ====================================================================
    if (g_showAbout) {
        ImGui::OpenPopup("About Vole Machine Simulator");
        g_showAbout = false;
    }
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 0), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("About Vole Machine Simulator", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.220f, 0.741f, 0.973f, 1.0f));
        ImGui::Text("Vole Machine Simulator");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped(
            "An interactive 16-bit Vole Machine CPU architecture simulator featuring "
            "real-time memory inspection, register tracking, and step-by-step "
            "instruction cycle visualization.");

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.886f, 0.910f, 0.941f, 1.0f));
        ImGui::Text("Created by:");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // 2-column borderless table: Name | ID (IDs vertically aligned)
        if (ImGui::BeginTable("TeamTable", 2,
            ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("ID",   ImGuiTableColumnFlags_WidthFixed, 90.0f);

            struct { const char* name; const char* id; } members[] = {
                {"Omar Hossam Eldin Zidan", "20250426"},
                {"Belal Ibrahim Sayed",     "20250117"},
                {"Kareem Emad Mohamed",     "20250488"},
                {"Shahd Ali Mohammed",      "20253056"},
            };
            for (auto& m : members) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::BulletText("%s", m.name);
                ImGui::TableSetColumnIndex(1);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.580f, 0.639f, 0.722f, 1.0f));
                ImGui::Text("%s", m.id);
                ImGui::PopStyleColor();
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Bold labels via accent colour
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.886f, 0.910f, 0.941f, 1.0f));
        ImGui::Text("Assignment:"); ImGui::PopStyleColor(); ImGui::SameLine(0, 4);
        ImGui::TextUnformatted("Assignment 2, Task 3 - Object-Oriented Programming (CS213)");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.886f, 0.910f, 0.941f, 1.0f));
        ImGui::Text("Instructor:"); ImGui::PopStyleColor(); ImGui::SameLine(0, 4);
        ImGui::TextUnformatted("Dr. Mohammed El-Ramly");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.886f, 0.910f, 0.941f, 1.0f));
        ImGui::Text("College:");    ImGui::PopStyleColor(); ImGui::SameLine(0, 4);
        ImGui::TextWrapped("Faculty of Computing and Artificial Intelligence - Cairo University");

        ImGui::Spacing();
        const float okW = 120.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - okW * 0.5f);
        if (ImGui::Button("OK", ImVec2(okW, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ====================================================================
    //  Help Modal — "Operations Set" instruction reference table
    // ====================================================================
    if (g_showHelp) {
        ImGui::OpenPopup("Help - Operations Set");
        g_showHelp = false;
    }
    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620, 0), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Help - Operations Set", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.220f, 0.741f, 0.973f, 1.0f));
        ImGui::Text("Vole Machine Instruction Set");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();

        // 3-column bordered reference table
        constexpr ImGuiTableFlags helpFlags =
            ImGuiTableFlags_BordersOuter   |
            ImGuiTableFlags_BordersInnerV  |
            ImGuiTableFlags_BordersInnerH  |
            ImGuiTableFlags_RowBg          |
            ImGuiTableFlags_SizingFixedFit;

        if (ImGui::BeginTable("OpsTable", 3, helpFlags))
        {
            ImGui::TableSetupColumn("Opc.", ImGuiTableColumnFlags_WidthFixed,   38.0f);
            ImGui::TableSetupColumn("Opr.", ImGuiTableColumnFlags_WidthFixed,   42.0f);
            ImGui::TableSetupColumn("Effect",ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            struct { const char* op; const char* opr; const char* effect; } ops[] = {
                {"1","RXY","Copy the content of RAM cell XY to register R."},
                {"2","RXY","Copy the bit-string XY directly to register R."},
                {"3","RXY","Copy the content of register R to RAM cell XY (Address 00 or FF prints to Screen)."},
                {"4","0RS","Copy the content of register R to register S."},
                {"5","RST","Add register S and register T (two's-complement) → register R."},
                {"6","RST","Add register S and register T (8-bit floating-point) → register R."},
                {"7","RST","Bitwise OR of register S and T → register R."},
                {"8","RST","Bitwise AND of register S and T → register R."},
                {"9","RST","Bitwise XOR of register S and T → register R."},
                {"A","R0X","Rotate the content of register R cyclically right X steps."},
                {"B","RXY","Jump to RAM cell XY if register R equals register 0."},
                {"C","000","Halt execution."},
            };

            ImGui::PushFont(g_fontMono);
            for (auto& row : ops) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.220f, 0.741f, 0.973f, 1.0f));
                ImGui::Text("%s", row.op);
                ImGui::PopStyleColor();
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", row.opr);
                ImGui::TableSetColumnIndex(2);
                ImGui::PopFont();   // switch to UI font for description
                ImGui::TextWrapped("%s", row.effect);
                ImGui::PushFont(g_fontMono);
            }
            ImGui::PopFont();
            ImGui::EndTable();
        }

        ImGui::Spacing();
        const float closeW = 110.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - closeW * 0.5f);
        if (ImGui::Button("Close", ImVec2(closeW, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    // ====================================================================
    //  Select Preset Program Modal
    // ====================================================================
    if (g_showPresetModal) {
        ImGui::OpenPopup("Select Preset Program");
        g_showPresetModal = false;
    }

    ImGui::SetNextWindowPos(
        ImGui::GetMainViewport()->GetCenter(),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(740, 0), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("Select Preset Program", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        // Modal Header
        ImGui::PushFont(g_fontBold ? g_fontBold : g_fontUI);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.220f, 0.741f, 0.973f, 1.0f)); // #38BDF8
        ImGui::Text("Select Preset Program");
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.580f, 0.639f, 0.722f, 1.0f)); // #94A3B8 Muted
        ImGui::TextUnformatted("- Select a test program to load into memory");
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Spacing();

        // Responsive scroll container for smaller window viewports
        float viewportH = ImGui::GetMainViewport()->Size.y;
        bool needScroll = (viewportH < 620.0f);
        if (needScroll) {
            ImGui::BeginChild("PresetScrollArea", ImVec2(0, viewportH - 180.0f), false);
        }

        int dblClickedPreset = -1;

        // Render the 6 selectable modern dark cards
        for (int i = 0; i < 6; ++i) {
            const auto& preset = kPresetPrograms[i];
            bool isSelected = (g_selectedPresetIndex == i);

            float cardW = ImGui::GetContentRegionAvail().x;

            ImFont* titleFont = g_fontBold ? g_fontBold : (g_fontUI ? g_fontUI : ImGui::GetFont());
            const float titleFontSize = 19.0f;

            ImFont* descFont = g_fontUI ? g_fontUI : ImGui::GetFont();
            const float descFontSize = 14.5f;

            float padX = 14.0f;
            float padY = 9.0f;
            float gapY = 4.0f;
            float wrapW = cardW - padX * 2.0f;

            ImVec2 descSize = descFont->CalcTextSizeA(descFontSize, FLT_MAX, wrapW, preset.description);
            float cardH = padY + titleFontSize + gapY + descSize.y + padY;
            if (cardH < 58.0f) cardH = 58.0f;

            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 p1 = ImVec2(p0.x + cardW, p0.y + cardH);

            ImGui::PushID(i);
            bool clicked = ImGui::InvisibleButton("##cardBtn", ImVec2(cardW, cardH));
            bool isHovered = ImGui::IsItemHovered();
            bool isClicked = ImGui::IsItemClicked();
            bool isDoubleClicked = isHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

            if (isClicked || clicked) {
                g_selectedPresetIndex = i;
            }

            if (isHovered) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            if (isDoubleClicked) {
                dblClickedPreset = i;
            }

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            // Background & border colors:
            // Selecting or hovering highlights border in bright cyan (#38BDF8) and gives subtle dark-cyan tint (#132A38)
            ImU32 bgCol;
            ImU32 borderCol;
            float borderThick;

            if (isSelected || isHovered) {
                bgCol       = IM_COL32(19, 42, 56, 255);       // #132A38 subtle dark-cyan tint
                borderCol   = IM_COL32(56, 189, 248, 255);      // #38BDF8 bright cyan highlight
                borderThick = isSelected ? 1.8f : 1.2f;
            } else {
                bgCol       = IM_COL32(22, 27, 38, 255);       // Modern dark surface
                borderCol   = IM_COL32(42, 50, 75, 200);       // #2A324B border
                borderThick = 1.0f;
            }

            drawList->AddRectFilled(p0, p1, bgCol, 7.0f);
            drawList->AddRect(p0, p1, borderCol, 7.0f, 0, borderThick);

            // 1. Top-left of card: Bold white program title
            ImVec2 titlePos(p0.x + padX, p0.y + padY);
            drawList->AddText(titleFont, titleFontSize, titlePos, IM_COL32(255, 255, 255, 255), preset.title);

            // 2. Top-right of card: Small rounded pill badge with category label and matching tint
            ImFont* badgeFont = g_fontUI ? g_fontUI : ImGui::GetFont();
            float badgeFontSize = 13.0f;
            ImVec2 badgeTextSize = badgeFont->CalcTextSizeA(badgeFontSize, FLT_MAX, 0.0f, preset.category);

            float pillPadX = 9.0f;
            float pillPadY = 3.0f;
            float pillW = badgeTextSize.x + pillPadX * 2.0f;
            float pillH = badgeTextSize.y + pillPadY * 2.0f;

            ImVec2 pillMin(p1.x - padX - pillW, p0.y + padY);
            ImVec2 pillMax(p1.x - padX, p0.y + padY + pillH);

            ImU32 badgeBgCol     = IM_COL32(preset.r, preset.g, preset.b, 45);
            ImU32 badgeBorderCol = IM_COL32(preset.r, preset.g, preset.b, 175);
            ImU32 badgeTextCol   = IM_COL32(preset.r, preset.g, preset.b, 255);

            drawList->AddRectFilled(pillMin, pillMax, badgeBgCol, pillH * 0.5f);
            drawList->AddRect(pillMin, pillMax, badgeBorderCol, pillH * 0.5f, 0, 1.0f);

            ImVec2 pillTextPos(pillMin.x + pillPadX, pillMin.y + pillPadY);
            drawList->AddText(badgeFont, badgeFontSize, pillTextPos, badgeTextCol, preset.category);

            // 3. Bottom of card: Muted gray description line
            ImVec2 descPos(p0.x + padX, p0.y + padY + titleFontSize + gapY);
            drawList->AddText(descFont, descFontSize, descPos, IM_COL32(148, 163, 184, 255),
                              preset.description, nullptr, wrapW);

            ImGui::PopID();
            ImGui::Spacing();
        }

        if (needScroll) {
            ImGui::EndChild();
        }

        // Direct double-click loading trigger
        if (dblClickedPreset >= 0) {
            g_selectedPresetIndex = dblClickedPreset;
            LoadPresetProgram(dblClickedPreset);
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Bottom-right: Cancel & Load Selected Preset buttons
        const float cancelW   = 100.0f;
        const float loadW     = 190.0f;
        const float btnGap    = 10.0f;
        const float totalBtnW = cancelW + btnGap + loadW;

        float modalAvailW = ImGui::GetContentRegionAvail().x;
        if (modalAvailW > totalBtnW) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (modalAvailW - totalBtnW));
        }

        if (ImGui::Button("Cancel", ImVec2(cancelW, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine(0, btnGap);

        // Prominent emerald load button (#059669 / #10B981)
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.020f, 0.588f, 0.412f, 1.0f)); // #059669
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.063f, 0.725f, 0.506f, 1.0f)); // #10B981
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.204f, 0.827f, 0.600f, 1.0f)); // #34D399
        if (ImGui::Button("Load Selected Preset", ImVec2(loadW, 0))) {
            LoadPresetProgram(g_selectedPresetIndex);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }

    // ── Present ──────────────────────────────────────────────────────────
    ImGui::Render();
    const float cc[4] = {0.043f, 0.055f, 0.078f, 1.0f}; // #0B0E14 Deep Obsidian
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, cc);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_pSwapChain->Present(1, 0);
}

// ============================================================
//  DirectX helpers
// ============================================================
static void CreateRenderTarget()
{
    ID3D11Texture2D* pBuf = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBuf));
    if (pBuf) {
        g_pd3dDevice->CreateRenderTargetView(pBuf, nullptr, &g_mainRenderTargetView);
        pBuf->Release();
    }
}
static void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd               = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = hWnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL fl;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext);
    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dDeviceContext);
    if (hr != S_OK) return false;
    CreateRenderTarget();
    return true;
}
static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain        = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice        = nullptr; }
}

// ============================================================
//  ApplyTheme  — called once after ImGui context is created
// ============================================================
static void ApplyTheme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    // ── Geometry ───────────────────────────────────────────────────────
    s.WindowRounding    = 10.0f;
    s.ChildRounding     =  8.0f;
    s.FrameRounding     =  6.0f;
    s.PopupRounding     = 10.0f;
    s.ScrollbarRounding =  8.0f;
    s.GrabRounding      =  6.0f;
    s.TabRounding       =  6.0f;
    s.WindowBorderSize  =  1.0f;
    s.ChildBorderSize   =  1.0f;
    s.FrameBorderSize   =  1.0f;
    s.PopupBorderSize   =  1.0f;

    // ── Spacing ────────────────────────────────────────────────────────
    s.WindowPadding    = ImVec2(14.0f,  12.0f);
    s.FramePadding     = ImVec2( 8.0f,   5.0f);  // global; bumped per-button via PushStyleVar
    s.ItemSpacing      = ImVec2( 8.0f,   7.0f);
    s.ItemInnerSpacing = ImVec2( 6.0f,   4.0f);
    s.CellPadding      = ImVec2( 6.0f,   5.0f);
    s.ScrollbarSize    = 11.0f;
    s.GrabMinSize      = 10.0f;
    s.SeparatorTextBorderSize = 2.0f;

    // ── Colour palette (Tokyo-Night / Vibrant Cyberpunk Dark) ───────────
    auto& c = s.Colors;

    // Backgrounds
    c[ImGuiCol_WindowBg]         = ImVec4(0.043f, 0.055f, 0.078f, 1.0f); // #0B0E14 Deep Obsidian
    c[ImGuiCol_ChildBg]          = ImVec4(0.075f, 0.090f, 0.133f, 1.0f); // #131722 Rich Navy-Slate
    c[ImGuiCol_PopupBg]          = ImVec4(0.055f, 0.071f, 0.106f, 1.0f); // #0E121B

    // Borders
    c[ImGuiCol_Border]           = ImVec4(0.165f, 0.196f, 0.294f, 0.90f); // #2A324B Crisp glowing border
    c[ImGuiCol_BorderShadow]     = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frame / input fields
    c[ImGuiCol_FrameBg]          = ImVec4(0.031f, 0.039f, 0.059f, 1.0f); // #080A0F Deep recessed
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.063f, 0.078f, 0.118f, 1.0f); // #10141E
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.078f, 0.102f, 0.157f, 1.0f); // #141A28

    // Title bars
    c[ImGuiCol_TitleBg]          = ImVec4(0.043f, 0.055f, 0.078f, 1.0f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.075f, 0.090f, 0.133f, 1.0f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.043f, 0.055f, 0.078f, 0.8f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]      = ImVec4(0.043f, 0.055f, 0.078f, 1.0f);
    c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.165f, 0.196f, 0.294f, 1.0f); // #2A324B
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.231f, 0.510f, 0.965f, 0.8f); // #3B82F6
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.231f, 0.510f, 0.965f, 1.0f);

    // Check / grab
    c[ImGuiCol_CheckMark]        = ImVec4(0.220f, 0.741f, 0.973f, 1.0f); // sky #38BDF8
    c[ImGuiCol_SliderGrab]       = ImVec4(0.220f, 0.741f, 0.973f, 0.8f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.350f, 0.820f, 1.000f, 1.0f);

    // Standard Buttons — Sleek royal slate-blue #1E293B, hover electric blue #3B82F6
    c[ImGuiCol_Button]           = ImVec4(0.118f, 0.161f, 0.231f, 1.0f); // #1E293B
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.231f, 0.510f, 0.965f, 1.0f); // #3B82F6
    c[ImGuiCol_ButtonActive]     = ImVec4(0.114f, 0.306f, 0.847f, 1.0f); // #1D4ED8

    // Header / selectable
    c[ImGuiCol_Header]           = ImVec4(0.145f, 0.200f, 0.300f, 0.70f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.231f, 0.510f, 0.965f, 0.50f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.231f, 0.510f, 0.965f, 0.80f);

    // Separators
    c[ImGuiCol_Separator]        = ImVec4(0.165f, 0.196f, 0.294f, 0.90f); // #2A324B
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.220f,  0.741f,  0.973f,  0.8f);
    c[ImGuiCol_SeparatorActive]  = ImVec4(0.220f,  0.741f,  0.973f,  1.0f);

    // Resize grip
    c[ImGuiCol_ResizeGrip]       = ImVec4(0.220f, 0.741f, 0.973f, 0.20f);
    c[ImGuiCol_ResizeGripHovered]= ImVec4(0.220f, 0.741f, 0.973f, 0.67f);
    c[ImGuiCol_ResizeGripActive] = ImVec4(0.220f, 0.741f, 0.973f, 0.95f);

    // Tables
    c[ImGuiCol_TableHeaderBg]    = ImVec4(0.118f, 0.149f, 0.220f, 1.0f); // #1E2638 Deep Indigo-Slate
    c[ImGuiCol_TableBorderLight] = ImVec4(0.165f, 0.196f, 0.294f, 0.50f);
    c[ImGuiCol_TableBorderStrong]= ImVec4(0.165f, 0.196f, 0.294f, 0.90f);
    c[ImGuiCol_TableRowBg]       = ImVec4(0.075f, 0.090f, 0.133f, 1.0f); // #131722
    c[ImGuiCol_TableRowBgAlt]    = ImVec4(0.094f, 0.118f, 0.173f, 1.0f); // #181E2C

    // Text
    c[ImGuiCol_Text]             = ImVec4(0.973f, 0.980f, 0.988f, 1.0f); // #F8FAFC bright white
    c[ImGuiCol_TextDisabled]     = ImVec4(0.392f, 0.455f, 0.545f, 1.0f); // #64748B
}

// ============================================================
//  runGuiApp()  — public entry point (called from main.cpp)
// ============================================================
int runGuiApp()
{
    // Enable crisp DPI-aware rendering before creating any window
    ImGui_ImplWin32_EnableDpiAwareness();

    // ── Register window class ─────────────────────────────────────────────
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr),
        nullptr, nullptr, nullptr, nullptr,
        L"VoleGUI", nullptr
    };
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(
        wc.lpszClassName, L"Vole Machine",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1180, 700,
        nullptr, nullptr, wc.hInstance, nullptr);
    g_hWnd = hwnd;

    // Customize Windows title bar to match dark theme using DWM API
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDarkMode, sizeof(useDarkMode));
    DwmSetWindowAttribute(hwnd, 19, &useDarkMode, sizeof(useDarkMode)); // Older Win10 compatibility
    COLORREF captionColor = RGB(15, 23, 42); // Deep Navy-Slate #0F172A matching our UI
    DwmSetWindowAttribute(hwnd, 35 /*DWMWA_CAPTION_COLOR*/, &captionColor, sizeof(captionColor));
    COLORREF textColor = RGB(226, 232, 240); // Crisp Ice-White text
    DwmSetWindowAttribute(hwnd, 36 /*DWMWA_TEXT_COLOR*/, &textColor, sizeof(textColor));

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // ── ImGui context ─────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // no imgui.ini persistence

    ApplyTheme();

    // ── Load fonts ────────────────────────────────────────────────────────
    // Segoe UI 17pt — primary UI font (becomes default)
    g_fontUI = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\segoeui.ttf", 17.0f);
    if (!g_fontUI) {
        // Fallback: build default font at 17pt
        ImFontConfig cfg; cfg.SizePixels = 17.0f;
        g_fontUI = io.Fonts->AddFontDefault(&cfg);
    }

    // Segoe UI Bold 19pt — section headers & top banner
    g_fontBold = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\segoeuib.ttf", 19.0f);
    if (!g_fontBold) {
        g_fontBold = g_fontUI;
    }

    // Consolas 16pt — monospace for tables & hex displays
    g_fontMono = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\consola.ttf", 16.0f);
    if (!g_fontMono) {
        ImFontConfig cfg; cfg.SizePixels = 16.0f;
        g_fontMono = io.Fonts->AddFontDefault(&cfg);
    }

    // ── Backends ──────────────────────────────────────────────────────────
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // ── Message / render loop ─────────────────────────────────────────────
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        RenderFrame();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
