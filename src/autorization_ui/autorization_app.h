#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include "imgui.h"
#include <string>
#include <vector>
#include "../cheat/cheat_ui.h"

struct AutoParticle {
    float x, y;
    float vx, vy;
    float size;
    float alpha;
    float phase;
    ImU32 color;
};

struct SakuraPetal {
    float x, y;
    float vx, vy;
    float rot;
    float rotSpeed;
    float size;
    float alpha;
};

class AutorizationApp {
public:
    AutorizationApp();
    ~AutorizationApp();

    void LoadFonts();
    bool Initialize(ID3D11Device* pDevice);
    void Render(HWND hwnd);
    void SwitchScreenPublic(HWND h, int s) { SwitchScreen(h, s); }

    int GetTargetWidth() const { return m_targetWinWidth; }
    int GetTargetHeight() const { return m_targetWinHeight; }
    int GetCurrentWidth() const { return m_currentWinWidth; }
    int GetCurrentHeight() const { return m_currentWinHeight; }

private:
    bool CreateTextureFromBytes(const unsigned char* bytes, unsigned int len,
                                ID3D11ShaderResourceView** ppSrv, int* outW, int* outH);
    void HandleWindowDrag(HWND hwnd);
    void SwitchScreen(HWND hwnd, int newScreen);

    void UpdateAndDrawParticles(ImDrawList* draw, const ImVec2& win_pos, const ImVec2& win_size, float dt);
    void UpdateAndDrawSakura(ImDrawList* draw, const ImVec2& cardMin, const ImVec2& cardMax, float dt);
    void DrawGojoAtmosphere(ImDrawList* draw, float artX, float artY, float artW, float artH);
    void DrawGojoElectricAura(ImDrawList* draw, float eyeCx, float eyeCy, float pulse);

    void RenderLoginScreen(HWND hwnd, ImDrawList* draw, const ImVec2& winPos, const ImVec2& winSize);
    void RenderSubscriptionScreen(HWND hwnd, ImDrawList* draw, const ImVec2& winPos, const ImVec2& winSize);

private:
    ID3D11Device* m_pDevice;

    ID3D11ShaderResourceView* m_pTexCatboy;
    int m_catboyW, m_catboyH;

    ID3D11ShaderResourceView* m_pTexChibi;
    int m_chibiW, m_chibiH;

    ID3D11ShaderResourceView* m_pTexEnma;
    int m_enmaW, m_enmaH;

    ID3D11ShaderResourceView* m_pTexPurple;
    int m_purpleW, m_purpleH;

    ImFont* m_fontTitle;
    ImFont* m_fontHeader;
    ImFont* m_fontSemiBold;
    ImFont* m_fontRegular;
    ImFont* m_fontSmall;

    int m_currentScreen;
    float m_screenTransition;

    char m_licenseKey[128];
    bool m_keyFocused;
    bool m_keyEmpty;

    bool m_loginError;
    float m_errorTimer;
    std::string m_errorMsg;
    float m_shakeTimer;

    bool m_showToast;
    float m_toastTimer;

    int m_selectedCard;
    float m_cardHover[2];
    bool m_isLaunching;
    float m_launchProgress;
    std::string m_statusMsg;

    bool m_isDragging;
    POINT m_dragStart;
    int m_currentWinWidth;
    int m_currentWinHeight;
    int m_targetWinWidth;
    int m_targetWinHeight;

    float m_globalTime;
    std::vector<AutoParticle> m_particles;
    std::vector<SakuraPetal> m_petals;

    CheatUI m_cheat;
};