#include "autorization_widgets.h"
#include "imgui_internal.h"
#include <cmath>

namespace AutoWidgets {

    // ============================================================
    // DARK INPUT BOX ("Login External", "Password External")
    // ============================================================
    bool DarkInputBox(const char* str_id, const char* hint, char* buf, size_t buf_size, 
                      ImVec2 size, bool is_password, bool* is_focused_out, ImFont* font, bool has_error) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        ImGui::PushID(str_id);

        const ImVec2 pos = window->DC.CursorPos;
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

        ImDrawList* draw = window->DrawList;
        float rounding = 6.0f;

        // Dark sleek background
        ImU32 bg_col = IM_COL32(18, 21, 28, 255);
        draw->AddRectFilled(bb.Min, bb.Max, bg_col, rounding);

        float padY = (size.y - 18.0f) * 0.5f;
        if (padY < 4.0f) padY = 4.0f;

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.96f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.40f, 0.44f, 0.52f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, padY));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);

        if (font) ImGui::PushFont(font);
        ImGuiInputTextFlags flags = is_password ? ImGuiInputTextFlags_Password : 0;
        
        // Pass "##input" to hide any label text from being rendered outside the box
        bool changed = ImGui::InputTextWithHint("##input", hint, buf, (int)buf_size, flags);
        bool focused = ImGui::IsItemActive();
        bool hovered = ImGui::IsItemHovered();
        if (is_focused_out) *is_focused_out = focused;

        if (font) ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        // Smooth focus border animation
        static float focus_table[512] = { 0 };
        int idx = (int)(window->GetID("##input") % 512);
        focus_table[idx] = Lerp(focus_table[idx], focused ? 1.0f : (hovered ? 0.35f : 0.0f), g.IO.DeltaTime * 14.0f);

        // Ambient outer neon glow when focused
        if (focus_table[idx] > 0.02f) {
            float glowAlpha = 55.0f * focus_table[idx];
            draw->AddRect(ImVec2(bb.Min.x - 1.5f, bb.Min.y - 1.5f), ImVec2(bb.Max.x + 1.5f, bb.Max.y + 1.5f),
                          IM_COL32(6, 182, 212, (int)glowAlpha), rounding + 1.5f, 0, 1.5f);
        }

        // Error border or normal/focused border
        if (has_error) {
            draw->AddRect(bb.Min, bb.Max, IM_COL32(239, 68, 68, 255), rounding, 0, 1.3f);
            draw->AddRect(ImVec2(bb.Min.x - 1.0f, bb.Min.y - 1.0f), ImVec2(bb.Max.x + 1.0f, bb.Max.y + 1.0f),
                          IM_COL32(239, 68, 68, 80), rounding + 1.0f, 0, 1.0f);
        } else {
            ImU32 border_col = IM_COL32(
                (int)(36 + 60 * focus_table[idx]),
                (int)(42 + 80 * focus_table[idx]),
                (int)(54 + 110 * focus_table[idx]),
                (int)(190 + 65 * focus_table[idx])
            );
            draw->AddRect(bb.Min, bb.Max, border_col, rounding, 0, 1.0f);
        }

        ImGui::PopID();
        return changed;
    }

    // ============================================================
    // LOGIN BUTTON ("Login >")
    // ============================================================
    bool LoginButton(const char* label, ImVec2 size, ImU32 bg_normal, ImU32 bg_hover, ImFont* font) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(label);
        const ImVec2 pos = window->DC.CursorPos;
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

        ImGui::ItemSize(size, 0.0f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        static float anim_table[512] = { 0 };
        int idx = (int)(id % 512);
        anim_table[idx] = Lerp(anim_table[idx], hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 14.0f);

        ImVec4 c_norm = ImGui::ColorConvertU32ToFloat4(bg_normal);
        ImVec4 c_hov  = ImGui::ColorConvertU32ToFloat4(bg_hover);
        ImVec4 c_cur(
            Lerp(c_norm.x, c_hov.x, anim_table[idx]),
            Lerp(c_norm.y, c_hov.y, anim_table[idx]),
            Lerp(c_norm.z, c_hov.z, anim_table[idx]),
            Lerp(c_norm.w, c_hov.w, anim_table[idx])
        );

        if (held) {
            c_cur.x *= 0.92f; c_cur.y *= 0.92f; c_cur.z *= 0.92f;
        }

        ImDrawList* draw = window->DrawList;
        float rounding = 6.0f;

        draw->AddRectFilled(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(c_cur), rounding);

        ImU32 border_col = IM_COL32(48, 56, 72, (int)(180 + 75 * anim_table[idx]));
        draw->AddRect(bb.Min, bb.Max, border_col, rounding, 0, 1.0f);

        // Center text + chevron
        if (font) ImGui::PushFont(font);
        ImVec2 text_sz = ImGui::CalcTextSize(label);
        ImVec2 text_pos(
            bb.Min.x + (size.x - text_sz.x) * 0.5f,
            bb.Min.y + (size.y - text_sz.y) * 0.5f
        );
        draw->AddText(text_pos, IM_COL32(245, 248, 255, 255), label);
        if (font) ImGui::PopFont();

        return pressed;
    }

    // ============================================================
    // PLAY BUTTON ("▶")
    // ============================================================
    bool PlayButton(const char* str_id, ImVec2 pos, float size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        const ImRect bb(pos, ImVec2(pos.x + size, pos.y + size));
        const ImGuiID id = window->GetID(str_id);

        ImGui::SetCursorScreenPos(pos);
        ImGui::ItemSize(ImVec2(size, size), 0.0f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        ImDrawList* draw = window->DrawList;

        // Hover circle
        if (hovered) {
            draw->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f), size * 0.52f, IM_COL32(34, 197, 94, 40));
        }

        // White/Green Triangle ▶
        float cx = pos.x + size * 0.5f;
        float cy = pos.y + size * 0.5f;
        float r = size * 0.32f;

        ImVec2 p1(cx - r * 0.7f, cy - r);
        ImVec2 p2(cx + r * 1.0f, cy);
        ImVec2 p3(cx - r * 0.7f, cy + r);

        ImU32 play_col = hovered ? IM_COL32(34, 197, 94, 255) : IM_COL32(255, 255, 255, 255);
        draw->AddTriangleFilled(p1, p2, p3, play_col);

        return pressed;
    }

    // ============================================================
    // TITLE CONTROL BUTTONS (Close, Minimize, Back)
    // ============================================================
    bool TitleCloseBtn(const char* str_id, ImVec2 pos, float size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImRect bb(pos, ImVec2(pos.x + size, pos.y + size));
        ImGuiID id = window->GetID(str_id);

        ImGui::SetCursorScreenPos(pos);
        ImGui::ItemSize(ImVec2(size, size), 0.0f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        ImDrawList* draw = window->DrawList;
        if (hovered) {
            draw->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f), size * 0.55f, IM_COL32(239, 68, 68, 220));
        }

        ImU32 col = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(148, 163, 184, 230);
        float pad = size * 0.28f;
        draw->AddLine(ImVec2(pos.x + pad, pos.y + pad), ImVec2(pos.x + size - pad, pos.y + size - pad), col, 1.4f);
        draw->AddLine(ImVec2(pos.x + size - pad, pos.y + pad), ImVec2(pos.x + pad, pos.y + size - pad), col, 1.4f);

        return pressed;
    }

    bool TitleMinBtn(const char* str_id, ImVec2 pos, float size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImRect bb(pos, ImVec2(pos.x + size, pos.y + size));
        ImGuiID id = window->GetID(str_id);

        ImGui::SetCursorScreenPos(pos);
        ImGui::ItemSize(ImVec2(size, size), 0.0f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        ImDrawList* draw = window->DrawList;
        if (hovered) {
            draw->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f), size * 0.55f, IM_COL32(50, 60, 75, 200));
        }

        ImU32 col = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(148, 163, 184, 230);
        float pad = size * 0.25f;
        draw->AddLine(ImVec2(pos.x + pad, pos.y + size * 0.5f), ImVec2(pos.x + size - pad, pos.y + size * 0.5f), col, 1.4f);

        return pressed;
    }

    bool TitleBackBtn(const char* str_id, ImVec2 pos, float size) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImRect bb(pos, ImVec2(pos.x + size, pos.y + size));
        ImGuiID id = window->GetID(str_id);

        ImGui::SetCursorScreenPos(pos);
        ImGui::ItemSize(ImVec2(size, size), 0.0f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        ImDrawList* draw = window->DrawList;
        if (hovered) {
            draw->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.5f), size * 0.55f, IM_COL32(50, 60, 75, 200));
        }

        ImU32 col = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(148, 163, 184, 230);
        float arrow_y = pos.y + size * 0.5f;
        float start_x = pos.x + size * 0.75f;
        float end_x   = pos.x + size * 0.25f;
        draw->AddLine(ImVec2(start_x, arrow_y), ImVec2(end_x, arrow_y), col, 1.4f);
        draw->AddLine(ImVec2(end_x + 3.5f, arrow_y - 3.5f), ImVec2(end_x, arrow_y), col, 1.4f);
        draw->AddLine(ImVec2(end_x + 3.5f, arrow_y + 3.5f), ImVec2(end_x, arrow_y), col, 1.4f);

        return pressed;
    }

    // ============================================================
    // GREEN TOAST BANNER ("✔ Successfully! Subscription Successfully Selected!")
    // ============================================================
    void DrawSuccessToast(ImDrawList* draw, ImVec2 pos, ImVec2 size, ImFont* font_bold, ImFont* font_regular) {
        float rounding = 6.0f;

        // Dark forest green background
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(5, 46, 22, 240), rounding);
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(22, 101, 52, 220), rounding, 0, 1.0f);

        // Green check circle icon (✔)
        float icon_cx = pos.x + 16.0f;
        float icon_cy = pos.y + 14.0f;
        float r = 7.0f;
        draw->AddCircle(ImVec2(icon_cx, icon_cy), r, IM_COL32(34, 197, 94, 255), 16, 1.3f);

        // Checkmark
        draw->AddLine(ImVec2(icon_cx - 3.5f, icon_cy), ImVec2(icon_cx - 0.5f, icon_cy + 3.0f), IM_COL32(34, 197, 94, 255), 1.5f);
        draw->AddLine(ImVec2(icon_cx - 0.5f, icon_cy + 3.0f), ImVec2(icon_cx + 4.0f, icon_cy - 2.5f), IM_COL32(34, 197, 94, 255), 1.5f);

        // "Successfully!" bold title
        if (font_bold) ImGui::PushFont(font_bold);
        draw->AddText(ImVec2(pos.x + 28.0f, pos.y + 6.0f), IM_COL32(34, 197, 94, 255), "Successfully!");
        if (font_bold) ImGui::PopFont();

        // "Subscription Successfully Selected!" subtitle
        if (font_regular) ImGui::PushFont(font_regular);
        draw->AddText(ImVec2(pos.x + 12.0f, pos.y + 24.0f), IM_COL32(134, 239, 172, 255), "Subscription Successfully Selected!");
        if (font_regular) ImGui::PopFont();
    }

    // ============================================================
    // RED ERROR TOAST BANNER ("✖ Error! Please enter Username & Password!")
    // ============================================================
    void DrawErrorToast(ImDrawList* draw, ImVec2 pos, ImVec2 size, const char* msg, ImFont* font_bold, ImFont* font_regular) {
        float rounding = 6.0f;

        // Dark crimson / wine background with red border
        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(40, 10, 16, 245), rounding);
        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(239, 68, 68, 220), rounding, 0, 1.0f);

        // Red cross circle icon (✖)
        float icon_cx = pos.x + 16.0f;
        float icon_cy = pos.y + 14.0f;
        float r = 7.0f;
        draw->AddCircle(ImVec2(icon_cx, icon_cy), r, IM_COL32(239, 68, 68, 255), 16, 1.3f);
        draw->AddLine(ImVec2(icon_cx - 2.5f, icon_cy - 2.5f), ImVec2(icon_cx + 2.5f, icon_cy + 2.5f), IM_COL32(239, 68, 68, 255), 1.5f);
        draw->AddLine(ImVec2(icon_cx + 2.5f, icon_cy - 2.5f), ImVec2(icon_cx - 2.5f, icon_cy + 2.5f), IM_COL32(239, 68, 68, 255), 1.5f);

        // "Action Required!" bold title
        if (font_bold) ImGui::PushFont(font_bold);
        draw->AddText(ImVec2(pos.x + 28.0f, pos.y + 6.0f), IM_COL32(248, 113, 113, 255), "Action Required!");
        if (font_bold) ImGui::PopFont();

        // Subtitle message
        if (font_regular) ImGui::PushFont(font_regular);
        draw->AddText(ImVec2(pos.x + 12.0f, pos.y + 24.0f), IM_COL32(254, 202, 202, 255), msg ? msg : "Enter username and password!");
        if (font_regular) ImGui::PopFont();
    }
}
