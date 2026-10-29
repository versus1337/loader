#include "cheat_ui.h"
#include "cheat_core.h"
#include "../autorization_ui/autorization_widgets.h"

using namespace cheat;

void CheatUI::Launch(HWND) {
    if (!Engine::Get().Running()) Engine::Get().Launch();
}

void CheatUI::DrawGroupGrid() {
    auto& e = Engine::Get();
    const int count = Engine::GroupCount();
    const int perCol = (count + 1) / 2;

    for (int i = 0; i < count; ++i) {
        int col = i / perCol;
        int row = i % perCol;
        ImGui::SetCursorPos(ImVec2(10.0f + col * 195.0f, 6.0f + row * 22.0f));

        bool on = e.GroupState(i);
        if (ImGui::Checkbox(Engine::GroupLabel(i), &on))
            e.SetGroup(i, on);
    }
}

void CheatUI::DrawLog() {
    auto lines = Engine::Get().LogSnapshot(400);
    for (const auto& l : lines)
        ImGui::TextUnformatted(l.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 6.0f)
        ImGui::SetScrollHereY(1.0f);
}

void CheatUI::Render(HWND hwnd, ImFont* regular, ImFont* small,
                     ImFont* semiBold, ImFont* header) {
    (void)regular; (void)header;
    auto& e = Engine::Get();

    ImGui::SetCursorPos(ImVec2(24.0f, 12.0f));
    if (semiBold) ImGui::PushFont(semiBold);
    ImGui::TextUnformatted("WOLFTEAM  //  EXTERNAL");
    if (semiBold) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(24.0f, 32.0f));
    if (small) ImGui::PushFont(small);
    ImGui::TextColored(ImVec4(0.58f, 0.64f, 0.72f, 1.0f), "%s", e.StatusLine().c_str());
    if (small) ImGui::PopFont();

    if (AutoWidgets::TitleBackBtn("win_back_2", ImVec2(824.0f, 16.0f), 16.0f))
        PostMessageW(hwnd, WM_APP + 1, 0, 0);
    if (AutoWidgets::TitleCloseBtn("win_close_2", ImVec2(872.0f, 16.0f), 16.0f))
        PostQuitMessage(0);

    float bx = 24.0f;
    const float by = 56.0f;

    ImGui::SetCursorPos(ImVec2(bx, by));
    if (!e.Running()) {
        if (ImGui::Button("Baglan", ImVec2(86, 24))) e.Launch();
    } else {
        if (ImGui::Button("Kes", ImVec2(86, 24))) e.Shutdown();
    }
    bx += 92;

    ImGui::SetCursorPos(ImVec2(bx, by));
    if (ImGui::Button(e.Armed() ? "PATCH KAPAT" : "PATCH AC", ImVec2(110, 24)))
        e.SetArmed(!e.Armed());
    bx += 116;

    ImGui::SetCursorPos(ImVec2(bx, by));
    if (ImGui::Button("OFSET GUNCELLE", ImVec2(126, 24))) e.UpdateOffsetsFromSigs();
    bx += 132;

    ImGui::SetCursorPos(ImVec2(bx, by));
    if (ImGui::Button("DOGRULA", ImVec2(86, 24))) e.Validate();
    bx += 92;

    ImGui::SetCursorPos(ImVec2(bx, by));
    if (ImGui::Button("GERI AL", ImVec2(80, 24))) e.RestoreAll();
    bx += 86;

    bool reloc = e.Reloc();
    ImGui::SetCursorPos(ImVec2(bx, by + 4.0f));
    if (ImGui::Checkbox("RELOC", &reloc)) e.SetReloc(reloc);

    ImGui::SetCursorPos(ImVec2(14.0f, 90.0f));
    ImGui::BeginChild("##patches", ImVec2(400.0f, 342.0f), true);
    DrawGroupGrid();
    ImGui::EndChild();

    static const char* nsItems[] = { "74 (JZ)", "90 90 (NOP)", "EB (JMP)", "75 (test)" };
    static const char* rfItems[] = { "74", "10 (test/no-op)", "75" };

    ImGui::SetCursorPos(ImVec2(14.0f, 438.0f));
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::Combo("No Spread", &m_nsCombo, nsItems, 4)) e.SetNsByte(m_nsCombo);

    ImGui::SetCursorPos(ImVec2(210.0f, 438.0f));
    ImGui::SetNextItemWidth(130.0f);
    if (ImGui::Combo("Rapid Fire", &m_rfCombo, rfItems, 3)) e.SetRfByte(m_rfCombo);

    ImGui::SetCursorPos(ImVec2(424.0f, 90.0f));
    ImGui::BeginChild("##logwrap", ImVec2(452.0f, 420.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    if (small) ImGui::PushFont(small);
    DrawLog();
    if (small) ImGui::PopFont();
    ImGui::EndChild();
}