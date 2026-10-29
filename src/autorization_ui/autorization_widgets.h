#pragma once
#include "imgui.h"
#include <string>

namespace AutoWidgets {

    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // Dark sleek rectangular input field matching Autorization screenshot
    bool DarkInputBox(const char* str_id, const char* hint, char* buf, size_t buf_size, 
                      ImVec2 size, bool is_password, bool* is_focused_out, ImFont* font, bool has_error = false);

    // "Login >" button with chevron
    bool LoginButton(const char* label, ImVec2 size, ImU32 bg_normal, ImU32 bg_hover, ImFont* font);

    // Play triangle button "▶" on cards
    bool PlayButton(const char* str_id, ImVec2 pos, float size = 26.0f);

    // Window control buttons
    bool TitleCloseBtn(const char* str_id, ImVec2 pos, float size = 16.0f);
    bool TitleMinBtn(const char* str_id, ImVec2 pos, float size = 16.0f);
    bool TitleBackBtn(const char* str_id, ImVec2 pos, float size = 16.0f);

    // Green toast banner: "✔ Successfully! Subscription Successfully Selected!"
    void DrawSuccessToast(ImDrawList* draw, ImVec2 pos, ImVec2 size, ImFont* font_bold, ImFont* font_regular);

    // Red error toast banner: "✖ Error! Please enter Username & Password!"
    void DrawErrorToast(ImDrawList* draw, ImVec2 pos, ImVec2 size, const char* msg, ImFont* font_bold, ImFont* font_regular);
}
