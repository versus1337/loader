#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "imgui.h"

class CheatUI {
public:
    void Render(HWND hwnd, ImFont* regular, ImFont* small, ImFont* semiBold, ImFont* header);
    void Launch(HWND hwnd);

private:
    void DrawGroupGrid();
    void DrawLog();

    int m_nsCombo = 0;
    int m_rfCombo = 0;
};