#include "autorization_app.h"
#include "autorization_widgets.h"
#include "../cheat/cheat_core.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"
#include "assets/autorization_bytes.h"

static std::string FindFontPath(const char* filename) {
    char exePath[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = '\0';

    std::string p1 = std::string(exePath) + "\\assets\\fonts\\" + filename;
    if (GetFileAttributesA(p1.c_str()) != INVALID_FILE_ATTRIBUTES) return p1;

    std::string p2 = std::string(exePath) + "\\..\\client\\assets\\fonts\\" + filename;
    if (GetFileAttributesA(p2.c_str()) != INVALID_FILE_ATTRIBUTES) return p2;

    std::string p3 = std::string("client\\assets\\fonts\\") + filename;
    if (GetFileAttributesA(p3.c_str()) != INVALID_FILE_ATTRIBUTES) return p3;

    std::string p4 = std::string("assets\\fonts\\") + filename;
    if (GetFileAttributesA(p4.c_str()) != INVALID_FILE_ATTRIBUTES) return p4;

    return "";
}

static bool IsValidTTF(const std::string& path) {
    if (path.empty()) return false;
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return false;
    unsigned char magic[4] = { 0 };
    size_t read = fread(magic, 1, 4, f);
    fclose(f);
    if (read < 4) return false;
    if (magic[0] == 0x00 && magic[1] == 0x01 && magic[2] == 0x00 && magic[3] == 0x00) return true;
    if (magic[0] == 'O' && magic[1] == 'T' && magic[2] == 'T' && magic[3] == 'O') return true;
    if (magic[0] == 't' && magic[1] == 'r' && magic[2] == 'u' && magic[3] == 'e') return true;
    return false;
}

AutorizationApp::AutorizationApp()
    : m_pDevice(nullptr),
      m_pTexCatboy(nullptr), m_catboyW(0), m_catboyH(0),
      m_pTexChibi(nullptr), m_chibiW(0), m_chibiH(0),
      m_pTexEnma(nullptr), m_enmaW(0), m_enmaH(0),
      m_pTexPurple(nullptr), m_purpleW(0), m_purpleH(0),
      m_fontTitle(nullptr), m_fontHeader(nullptr), m_fontSemiBold(nullptr),
      m_fontRegular(nullptr), m_fontSmall(nullptr),
      m_currentScreen(0), m_screenTransition(1.0f),
      m_keyFocused(false), m_keyEmpty(false),
      m_loginError(false), m_errorTimer(0.0f), m_shakeTimer(0.0f),
      m_showToast(false), m_toastTimer(0.0f),
      m_selectedCard(0), m_isLaunching(false), m_launchProgress(0.0f),
      m_isDragging(false),
      m_targetWinWidth(680), m_targetWinHeight(460),
      m_currentWinWidth(680), m_currentWinHeight(460),
      m_globalTime(0.0f)
{
    m_dragStart = { 0, 0 };
    memset(m_licenseKey, 0, sizeof(m_licenseKey));
    m_cardHover[0] = 0.0f;
    m_cardHover[1] = 0.0f;

    // Initialize 32 subtle celestial stardust particles (clean ambient, no mouse tracking)
    m_particles.clear();
    for (int i = 0; i < 32; i++) {
        AutoParticle p;
        p.x = (float)(rand() % 750);
        p.y = (float)(rand() % 520);
        p.vx = ((float)(rand() % 40) - 20.0f) * 0.2f;
        p.vy = -8.0f - (float)(rand() % 20);
        p.size = 1.0f + (float)(rand() % 14) * 0.1f;
        p.alpha = 0.10f + (float)(rand() % 14) * 0.01f;
        p.phase = (float)(rand() % 100) * 0.1f;
        p.color = (i % 3 == 0) ? IM_COL32(34, 211, 238, 255) : ((i % 3 == 1) ? IM_COL32(248, 250, 252, 255) : IM_COL32(96, 165, 250, 255));
        m_particles.push_back(p);
    }

    // Initialize 24 purple sakura cherry blossom petals for Screen 2
    m_petals.clear();
    for (int i = 0; i < 24; i++) {
        SakuraPetal sp;
        sp.x = (float)(rand() % 300);
        sp.y = (float)(rand() % 350);
        sp.vx = 8.0f + (float)(rand() % 18);
        sp.vy = 18.0f + (float)(rand() % 22);
        sp.rot = (float)(rand() % 360) * 0.01745f;
        sp.rotSpeed = ((float)(rand() % 40) - 20.0f) * 0.05f;
        sp.size = 3.5f + (float)(rand() % 8) * 0.5f;
        sp.alpha = 0.25f + (float)(rand() % 15) * 0.03f;
        m_petals.push_back(sp);
    }
}

AutorizationApp::~AutorizationApp() {
    cheat::Engine::Get().Shutdown();
    if (m_pTexCatboy) { m_pTexCatboy->Release(); m_pTexCatboy = nullptr; }
    if (m_pTexChibi)  { m_pTexChibi->Release();  m_pTexChibi = nullptr; }
    if (m_pTexEnma)   { m_pTexEnma->Release();   m_pTexEnma = nullptr; }
    if (m_pTexPurple) { m_pTexPurple->Release(); m_pTexPurple = nullptr; }
}

void AutorizationApp::LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig cfg;
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;
    cfg.RasterizerMultiply = 1.05f;

    std::string poppinsBold = FindFontPath("Poppins-Bold.ttf");
    std::string interBold   = FindFontPath("Inter-Bold.ttf");
    std::string interSemi   = FindFontPath("Inter-SemiBold.ttf");
    std::string interReg    = FindFontPath("Inter-Regular.ttf");

    const char* pPoppinsBold = IsValidTTF(poppinsBold) ? poppinsBold.c_str() : "C:\\Windows\\Fonts\\segoeuib.ttf";
    const char* pInterBold   = IsValidTTF(interBold)   ? interBold.c_str()   : (IsValidTTF(poppinsBold) ? poppinsBold.c_str() : "C:\\Windows\\Fonts\\segoeuib.ttf");
    const char* pInterSemi   = IsValidTTF(interSemi)   ? interSemi.c_str()   : (IsValidTTF(poppinsBold) ? poppinsBold.c_str() : "C:\\Windows\\Fonts\\segoeuib.ttf");
    const char* pInterReg    = IsValidTTF(interReg)    ? interReg.c_str()    : "C:\\Windows\\Fonts\\segoeui.ttf";

    m_fontTitle    = io.Fonts->AddFontFromFileTTF(pPoppinsBold, 18.0f, &cfg);
    m_fontHeader   = io.Fonts->AddFontFromFileTTF(pInterBold,   14.5f, &cfg);
    m_fontSemiBold = io.Fonts->AddFontFromFileTTF(pInterSemi,   13.0f, &cfg);
    m_fontRegular  = io.Fonts->AddFontFromFileTTF(pInterReg,    12.0f, &cfg);
    m_fontSmall    = io.Fonts->AddFontFromFileTTF(pInterReg,    11.0f, &cfg);
}

bool AutorizationApp::CreateTextureFromBytes(const unsigned char* bytes, unsigned int len,
                                            ID3D11ShaderResourceView** ppSrv, int* outW, int* outH) {
    if (!bytes || len == 0 || !m_pDevice) return false;

    int channels = 0;
    unsigned char* data = stbi_load_from_memory(bytes, len, outW, outH, &channels, 4);
    if (!data) return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = *outW;
    desc.Height = *outH;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub;
    ZeroMemory(&sub, sizeof(sub));
    sub.pSysMem = data;
    sub.SysMemPitch = (*outW) * 4;

    ID3D11Texture2D* pTex = nullptr;
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &sub, &pTex);
    if (SUCCEEDED(hr) && pTex) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        m_pDevice->CreateShaderResourceView(pTex, &srvDesc, ppSrv);
        pTex->Release();
    }

    stbi_image_free(data);
    return (*ppSrv != nullptr);
}

bool AutorizationApp::Initialize(ID3D11Device* pDevice) {
    m_pDevice = pDevice;
    if (!m_pDevice) return false;

    CreateTextureFromBytes(catboy_bytes, catboy_len, &m_pTexCatboy, &m_catboyW, &m_catboyH);
    CreateTextureFromBytes(chibi_bytes, chibi_len, &m_pTexChibi, &m_chibiW, &m_chibiH);
    CreateTextureFromBytes(katana_enma_bytes, katana_enma_len, &m_pTexEnma, &m_enmaW, &m_enmaH);
    CreateTextureFromBytes(katana_purple_bytes, katana_purple_len, &m_pTexPurple, &m_purpleW, &m_purpleH);

    return true;
}

void AutorizationApp::SwitchScreen(HWND hwnd, int newScreen) {
    m_currentScreen = newScreen;
    if (newScreen == 0) {
        m_targetWinWidth = 680;
        m_targetWinHeight = 460;
    } else if (newScreen == 1) {
        m_targetWinWidth = 720;
        m_targetWinHeight = 490;
    } else {
        m_targetWinWidth = 900;
        m_targetWinHeight = 560;
    }
    m_currentWinWidth  = m_targetWinWidth;
    m_currentWinHeight = m_targetWinHeight;
    m_showToast = false;
    m_loginError = false;

    RECT r;
    GetWindowRect(hwnd, &r);
    SetWindowPos(hwnd, nullptr, r.left, r.top, m_currentWinWidth, m_currentWinHeight, SWP_NOZORDER | SWP_NOMOVE);
    HRGN hRgn = CreateRoundRectRgn(0, 0, m_currentWinWidth + 1, m_currentWinHeight + 1, 20, 20);
    SetWindowRgn(hwnd, hRgn, TRUE);
    DeleteObject(hRgn);
}

void AutorizationApp::HandleWindowDrag(HWND hwnd) {
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isLeftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    bool inTitleArea = (mousePos.y >= 0.0f && mousePos.y <= 50.0f && mousePos.x >= 0.0f && mousePos.x <= (float)m_currentWinWidth - 90.0f);

    if (inTitleArea && isLeftClicked && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive()) {
        m_isDragging = true;
        GetCursorPos(&m_dragStart);
    }

    if (m_isDragging) {
        if (isLeftDown) {
            POINT current;
            GetCursorPos(&current);
            int dx = current.x - m_dragStart.x;
            int dy = current.y - m_dragStart.y;
            if (dx != 0 || dy != 0) {
                RECT r;
                GetWindowRect(hwnd, &r);
                SetWindowPos(hwnd, nullptr, r.left + dx, r.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                m_dragStart = current;
            }
        } else {
            m_isDragging = false;
        }
    }
}

void AutorizationApp::UpdateAndDrawParticles(ImDrawList* draw, const ImVec2& win_pos, const ImVec2& win_size, float dt) {
    for (auto& p : m_particles) {
        p.y += p.vy * dt;
        p.x += (p.vx + sinf(m_globalTime * 1.5f + p.phase) * 0.8f) * dt;

        if (p.y < 0.0f) {
            p.y = win_size.y + 10.0f;
            p.x = (float)(rand() % (int)win_size.x);
        }
        if (p.x < 0.0f) p.x = win_size.x;
        if (p.x > win_size.x) p.x = 0.0f;

        float alphaF = p.alpha * (0.8f + 0.2f * sinf(m_globalTime * 3.0f + p.phase));
        ImVec4 colV = ImGui::ColorConvertU32ToFloat4(p.color);
        colV.w = alphaF;
        ImU32 c = ImGui::ColorConvertFloat4ToU32(colV);

        draw->AddCircleFilled(ImVec2(win_pos.x + p.x, win_pos.y + p.y), p.size, c, 12);
    }
}

void AutorizationApp::DrawGojoAtmosphere(ImDrawList* draw, float artX, float artY, float artW, float artH) {
    ImVec2 bgMin(artX, artY);
    ImVec2 bgMax(artX + artW, artY + artH);

    // Vibrant Gojo Blue atmosphere gradient (rich royal azure & electric cyan)
    draw->AddRectFilledMultiColor(
        bgMin, bgMax,
        IM_COL32(11, 13, 17, 255),   // top-left (seamless blend with left control panel)
        IM_COL32(2, 132, 199, 255),  // top-right (vibrant Gojo Azure Blue)
        IM_COL32(12, 74, 110, 255),  // bottom-right (deep ocean Gojo blue)
        IM_COL32(11, 13, 17, 255)    // bottom-left
    );

    // Ethereal glowing Gojo Blue infinity core
    ImVec2 glowCenter(artX + artW * 0.58f, artY + artH * 0.44f);
    draw->AddCircleFilled(glowCenter, 190.0f, IM_COL32(2, 132, 199, 45), 40);
    draw->AddCircleFilled(glowCenter, 120.0f, IM_COL32(14, 165, 233, 60), 32);
    draw->AddCircleFilled(glowCenter, 60.0f,  IM_COL32(56, 189, 248, 75), 24);

    // Celestial infinity halo / arcane ring behind Gojo's head
    float ringR = 130.0f;
    draw->AddCircle(glowCenter, ringR, IM_COL32(34, 211, 238, 75), 64, 1.4f);
    draw->AddCircle(glowCenter, ringR * 0.75f, IM_COL32(125, 211, 252, 50), 48, 1.0f);
    draw->AddCircle(glowCenter, ringR * 1.20f, IM_COL32(14, 165, 233, 40), 64, 1.0f);

    // Rotating celestial accent ticks
    float rotAngle = m_globalTime * 0.4f;
    for (int i = 0; i < 8; i++) {
        float a = rotAngle + i * (3.14159265f / 4.0f);
        ImVec2 p1(glowCenter.x + cosf(a) * (ringR - 5.0f), glowCenter.y + sinf(a) * (ringR - 5.0f));
        ImVec2 p2(glowCenter.x + cosf(a) * (ringR + 6.0f), glowCenter.y + sinf(a) * (ringR + 6.0f));
        draw->AddLine(p1, p2, IM_COL32(56, 189, 248, 140), 1.6f);
    }
}

void AutorizationApp::DrawGojoElectricAura(ImDrawList* draw, float eyeCx, float eyeCy, float pulse) {
    // Multi-layered glowing Six Eyes bloom
    draw->AddCircleFilled(ImVec2(eyeCx, eyeCy), 28.0f * pulse, IM_COL32(6, 182, 212, (int)(25 * pulse)), 32);
    draw->AddCircleFilled(ImVec2(eyeCx, eyeCy), 16.0f * pulse, IM_COL32(14, 165, 233, (int)(60 * pulse)), 24);
    draw->AddCircleFilled(ImVec2(eyeCx, eyeCy), 8.5f * pulse,  IM_COL32(34, 211, 238, (int)(120 * pulse)), 20);
    draw->AddCircleFilled(ImVec2(eyeCx, eyeCy), 3.5f * pulse,  IM_COL32(255, 255, 255, 240), 16);

    // Dynamic Six Eyes Electric Arcs
    for (int i = 0; i < 4; i++) {
        float angle = m_globalTime * (8.0f + i * 2.0f) + i * 1.57f;
        float r1 = 10.0f + sinf(angle * 3.0f) * 4.0f;
        float r2 = 22.0f + cosf(angle * 2.0f) * 6.0f;
        ImVec2 p1(eyeCx + cosf(angle) * r1, eyeCy + sinf(angle) * r1);
        ImVec2 p2(eyeCx + cosf(angle + 0.4f) * (r1 + r2) * 0.5f, eyeCy + sinf(angle + 0.4f) * (r1 + r2) * 0.5f);
        ImVec2 p3(eyeCx + cosf(angle) * r2, eyeCy + sinf(angle) * r2);

        float arcAlpha = 0.5f + 0.5f * sinf(m_globalTime * 15.0f + i * 2.0f);
        ImU32 arcCol = IM_COL32(165, 243, 252, (int)(220 * arcAlpha));
        draw->AddLine(p1, p2, arcCol, 1.4f);
        draw->AddLine(p2, p3, arcCol, 1.4f);
    }
}

void AutorizationApp::UpdateAndDrawSakura(ImDrawList* draw, const ImVec2& cardMin, const ImVec2& cardMax, float dt) {
    for (auto& sp : m_petals) {
        sp.y += sp.vy * dt;
        sp.x += (sp.vx + sinf(m_globalTime * 2.0f + sp.rot) * 10.0f) * dt;
        sp.rot += sp.rotSpeed * dt;

        if (sp.y > (cardMax.y - cardMin.y)) {
            sp.y = -10.0f;
            sp.x = (float)(rand() % (int)(cardMax.x - cardMin.x));
        }

        ImVec2 pt(cardMin.x + sp.x, cardMin.y + sp.y);
        float s = sp.size;
        ImVec2 p1(pt.x + cosf(sp.rot) * s, pt.y + sinf(sp.rot) * s * 0.5f);
        ImVec2 p2(pt.x - cosf(sp.rot) * s, pt.y - sinf(sp.rot) * s * 0.5f);

        draw->AddLine(p1, p2, IM_COL32(216, 180, 254, (int)(sp.alpha * 255)), 2.2f);
    }
}

void AutorizationApp::RenderLoginScreen(HWND hwnd, ImDrawList* draw, const ImVec2& winPos, const ImVec2& winSize) {
    float dt = ImGui::GetIO().DeltaTime;

    // Handle shake animation when input error occurs
    float shakeX = 0.0f;
    if (m_shakeTimer > 0.0f) {
        m_shakeTimer -= dt;
        shakeX = sinf(m_shakeTimer * 50.0f) * 6.0f;
    }

    // -------------------------------------------------------------
    // Right Side: Atmospheric Midnight Background + Baby Gojo
    // -------------------------------------------------------------
    float artW = 390.0f;
    float artH = 450.0f;
    float artX = winPos.x + winSize.x - artW + 15.0f;
    float artY = winPos.y + 8.0f;

    // 1. Draw atmospheric midnight background behind Gojo (not pure black)
    DrawGojoAtmosphere(draw, artX, artY, artW, artH);

    // 2. Draw Baby Gojo character artwork
    if (m_pTexCatboy) {
        draw->AddImage((ImTextureID)m_pTexCatboy, ImVec2(artX, artY), ImVec2(artX + artW, artY + artH));

        // Soft gradient fade on left edge for seamless transition into the left form area
        draw->AddRectFilledMultiColor(
            ImVec2(artX, artY),
            ImVec2(artX + 80.0f, artY + artH),
            IM_COL32(11, 13, 17, 255),
            IM_COL32(11, 13, 17, 0),
            IM_COL32(11, 13, 17, 0),
            IM_COL32(11, 13, 17, 255)
        );

        // Electric Six Eyes Glow & Lightning Aura
        float eyeCx = artX + artW * 0.59f;
        float eyeCy = artY + artH * 0.46f;
        float pulse = 0.85f + 0.15f * sinf(m_globalTime * 4.0f);
        DrawGojoElectricAura(draw, eyeCx, eyeCy, pulse);
    }

    // -------------------------------------------------------------
    // Top-Left: Chibi Avatar & "Autorization" Title
    // -------------------------------------------------------------
    float startX = winPos.x + 32.0f;
    float startY = winPos.y + 24.0f;

    if (m_pTexChibi) {
        draw->AddImageRounded((ImTextureID)m_pTexChibi, 
            ImVec2(startX, startY), 
            ImVec2(startX + 28.0f, startY + 28.0f), 
            ImVec2(0, 0), ImVec2(1, 1), 
            IM_COL32(255, 255, 255, 255), 6.0f);
    }

    if (m_fontTitle) ImGui::PushFont(m_fontTitle);
    draw->AddText(ImVec2(startX + 38.0f, startY + 3.0f), IM_COL32(241, 245, 249, 255), "Autorization");
    if (m_fontTitle) ImGui::PopFont();

    // Top-Right: Window Controls (Minimize & Close)
    float btnY = winPos.y + 18.0f;
    float closeX = winPos.x + winSize.x - 28.0f;
    float minX   = closeX - 24.0f;

    if (AutoWidgets::TitleMinBtn("win_min_0", ImVec2(minX, btnY), 16.0f)) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }
    if (AutoWidgets::TitleCloseBtn("win_close_0", ImVec2(closeX, btnY), 16.0f)) {
        PostQuitMessage(0);
    }

    // -------------------------------------------------------------
    // Left Form Area: License Key Input & Login Action (Clean, No Paste Key Button)
    // -------------------------------------------------------------
    float formX = startX + shakeX;
    float formY = winPos.y + 105.0f;
    float formW = 245.0f;
    float inputH = 42.0f;

    // Subtitle
    if (m_fontRegular) ImGui::PushFont(m_fontRegular);
    draw->AddText(ImVec2(formX, formY - 26.0f), IM_COL32(148, 163, 184, 230), "Enter your license key to continue:");
    if (m_fontRegular) ImGui::PopFont();

    // Single sleek License Key input field
    ImGui::SetCursorScreenPos(ImVec2(formX, formY));
    AutoWidgets::DarkInputBox("license_key", "Enter License Key...", m_licenseKey, sizeof(m_licenseKey),
                             ImVec2(formW, inputH), false, &m_keyFocused, m_fontSemiBold, m_keyEmpty);

    // Login Action Button: "Login >" (directly below input, zero clutter)
    ImGui::SetCursorScreenPos(ImVec2(formX, formY + 54.0f));
    if (AutoWidgets::LoginButton("Login  >", ImVec2(formW, 40.0f), 
                                IM_COL32(22, 26, 36, 255), 
                                IM_COL32(36, 44, 58, 255), 
                                m_fontSemiBold)) {
        
        // Strict License Key Validation
        auto isAllWhitespace = [](const char* s) {
            if (!s || !*s) return true;
            for (; *s; ++s) {
                if (!isspace((unsigned char)*s)) return false;
            }
            return true;
        };

        m_keyEmpty = isAllWhitespace(m_licenseKey);

        if (m_keyEmpty) {
            m_loginError = true;
            m_errorTimer = 2.8f;
            m_shakeTimer = 0.35f;
            m_errorMsg = "Please enter your license key!";
        } else {
            // Valid key entered: Show successful toast, then transition!
            m_loginError = false;
            m_showToast = true;
            m_toastTimer = 1.3f;
        }
    }

    // Dynamic Notifications at the bottom (Error or Success)
    float bannerY = formY + 115.0f;

    if (m_loginError) {
        m_errorTimer -= dt;
        if (m_errorTimer <= 0.0f) {
            m_loginError = false;
            m_keyEmpty = false;
        }
        AutoWidgets::DrawErrorToast(draw, ImVec2(formX, bannerY), ImVec2(formW, 48.0f), m_errorMsg.c_str(), m_fontSemiBold, m_fontSmall);
    } else if (m_showToast) {
        AutoWidgets::DrawSuccessToast(draw, ImVec2(formX, bannerY), ImVec2(formW, 48.0f), m_fontSemiBold, m_fontSmall);

        m_toastTimer -= dt;
        if (m_toastTimer <= 0.0f) {
            SwitchScreen(hwnd, 1); // Transition to Subscription Screen!
        }
    }
}

void AutorizationApp::RenderSubscriptionScreen(HWND hwnd, ImDrawList* draw, const ImVec2& winPos, const ImVec2& winSize) {
    float dt = ImGui::GetIO().DeltaTime;

    // -------------------------------------------------------------
    // Top Bar: Mascot Avatar (left) + Window Controls (right)
    // -------------------------------------------------------------
    float avatarX = winPos.x + 28.0f;
    float avatarY = winPos.y + 18.0f;

    if (m_pTexChibi) {
        draw->AddImageRounded((ImTextureID)m_pTexChibi,
            ImVec2(avatarX, avatarY),
            ImVec2(avatarX + 26.0f, avatarY + 26.0f),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 255), 5.0f);
    }

    // Top Right: Back button (to return to Screen 0), Minimize, Close
    float btnY = winPos.y + 18.0f;
    float closeX = winPos.x + winSize.x - 28.0f;
    float minX   = closeX - 24.0f;
    float backX  = minX - 24.0f;

    if (AutoWidgets::TitleBackBtn("win_back_1", ImVec2(backX, btnY), 16.0f)) {
        SwitchScreen(hwnd, 0); // Smoothly return to login screen
    }
    if (AutoWidgets::TitleMinBtn("win_min_1", ImVec2(minX, btnY), 16.0f)) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }
    if (AutoWidgets::TitleCloseBtn("win_close_1", ImVec2(closeX, btnY), 16.0f)) {
        PostQuitMessage(0);
    }

    // -------------------------------------------------------------
    // Centered Title Header
    // -------------------------------------------------------------
    float centerX = winPos.x + winSize.x * 0.5f;

    if (m_fontTitle) ImGui::PushFont(m_fontTitle);
    const char* titleText = "SELECT SUBSCRIPTION";
    ImVec2 titleSz = ImGui::CalcTextSize(titleText);
    draw->AddText(ImVec2(centerX - titleSz.x * 0.5f, winPos.y + 22.0f), IM_COL32(248, 250, 252, 255), titleText);
    if (m_fontTitle) ImGui::PopFont();

    if (m_fontRegular) ImGui::PushFont(m_fontRegular);
    const char* subText = "Choose a subscription to launch";
    ImVec2 subSz = ImGui::CalcTextSize(subText);
    draw->AddText(ImVec2(centerX - subSz.x * 0.5f, winPos.y + 48.0f), IM_COL32(148, 163, 184, 240), subText);
    if (m_fontRegular) ImGui::PopFont();

    // -------------------------------------------------------------
    // Two Subscription Cards (Enma Katana & Purple Sakura Katana)
    // -------------------------------------------------------------
    float cardW = 285.0f;
    float cardH = 370.0f;
    float cardSpacing = 32.0f;
    float totalW = cardW * 2.0f + cardSpacing;
    float cardsStartX = winPos.x + (winSize.x - totalW) * 0.5f;
    float cardBaseY = winPos.y + 82.0f;

    for (int i = 0; i < 2; i++) {
        float cx = cardsStartX + i * (cardW + cardSpacing);
        float cy = cardBaseY;

        ImRect cardBb(ImVec2(cx, cy), ImVec2(cx + cardW, cy + cardH));
        bool hovered = ImGui::IsMouseHoveringRect(cardBb.Min, cardBb.Max);

        m_cardHover[i] = AutoWidgets::Lerp(m_cardHover[i], hovered ? 1.0f : 0.0f, dt * 12.0f);
        float lift = m_cardHover[i] * 5.0f;
        cy -= lift;

        ImVec2 cMin(cx, cy);
        ImVec2 cMax(cx + cardW, cy + cardH);

        // Card Drop Shadow
        if (m_cardHover[i] > 0.01f) {
            draw->AddRectFilled(ImVec2(cMin.x + 4.0f, cMin.y + 8.0f), ImVec2(cMax.x + 4.0f, cMax.y + 8.0f),
                                IM_COL32(0, 0, 0, (int)(80 * m_cardHover[i])), 12.0f);
        }

        // Card Background: Pitch charcoal black
        draw->AddRectFilled(cMin, cMax, IM_COL32(14, 16, 22, 255), 10.0f);

        // Card Border with glow on hover
        ImU32 borderCol = (i == 0)
            ? IM_COL32((int)(38 + 50 * m_cardHover[i]), (int)(44 + 90 * m_cardHover[i]), (int)(54 + 40 * m_cardHover[i]), 255)
            : IM_COL32((int)(38 + 90 * m_cardHover[i]), (int)(44 + 40 * m_cardHover[i]), (int)(54 + 110 * m_cardHover[i]), 255);
        draw->AddRect(cMin, cMax, borderCol, 10.0f, 0, 1.2f);

        // Card Artwork (Top 285px)
        ID3D11ShaderResourceView* tex = (i == 0) ? m_pTexEnma : m_pTexPurple;
        if (tex) {
            float imgPad = 8.0f;
            ImVec2 imgMin(cMin.x + imgPad, cMin.y + imgPad);
            ImVec2 imgMax(cMax.x - imgPad, cMin.y + 285.0f);

            draw->PushClipRect(imgMin, imgMax, true);
            draw->AddImageRounded((ImTextureID)tex, imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), 8.0f);

            // Floating petals effect over Card 2 (Purple Sakura)
            if (i == 1) {
                UpdateAndDrawSakura(draw, imgMin, imgMax, dt);
            }

            // Flame aura effect over Card 1 (Enma)
            if (i == 0 && m_cardHover[0] > 0.1f) {
                float flamePulse = sinf(m_globalTime * 6.0f) * 0.5f + 0.5f;
                draw->AddRectFilled(imgMin, imgMax, IM_COL32(220, 38, 38, (int)(15 * m_cardHover[0] * flamePulse)));
            }

            // Subtle bottom dark gradient overlay for smooth transition into card footer
            draw->AddRectFilledMultiColor(
                ImVec2(imgMin.x, imgMax.y - 50.0f),
                imgMax,
                IM_COL32(14, 16, 22, 0),
                IM_COL32(14, 16, 22, 0),
                IM_COL32(14, 16, 22, 255),
                IM_COL32(14, 16, 22, 255)
            );
            draw->PopClipRect();
        }

        // ---------------------------------------------------------
        // Card Footer: Title, Functions, Play Button
        // ---------------------------------------------------------
        float footX = cMin.x + 18.0f;
        float footY = cMin.y + 300.0f;

        // Line 1: Subscription Name
        const char* cardName = (i == 0) ? "External" : "External Plus";
        if (m_fontHeader) ImGui::PushFont(m_fontHeader);
        draw->AddText(ImVec2(footX, footY), IM_COL32(248, 250, 252, 255), cardName);
        if (m_fontHeader) ImGui::PopFont();

        // Line 2: Status Indicator (Glowing Green Dot) + Functions text
        float dotX = footX + 4.0f;
        float dotY = footY + 28.0f;
        draw->AddCircleFilled(ImVec2(dotX, dotY), 4.5f, IM_COL32(34, 197, 94, 60));
        draw->AddCircleFilled(ImVec2(dotX, dotY), 2.5f, IM_COL32(34, 197, 94, 255));

        const char* fnDesc = (i == 0) ? "External Functions" : "Brutal Functions";
        if (m_fontSmall) ImGui::PushFont(m_fontSmall);
        draw->AddText(ImVec2(dotX + 10.0f, dotY - 7.0f), IM_COL32(148, 163, 184, 255), fnDesc);
        if (m_fontSmall) ImGui::PopFont();

        // Right side Play Button (▶)
        float playSize = 32.0f;
        ImVec2 playPos(cMax.x - playSize - 18.0f, footY + 6.0f);
        std::string playBtnId = std::string("play_btn_") + std::to_string(i);

        if (AutoWidgets::PlayButton(playBtnId.c_str(), playPos, playSize)) {
            m_selectedCard = i;
            m_cheat.Launch(hwnd);
            SwitchScreen(hwnd, 2);
        }
    }

    // -------------------------------------------------------------
    // Launching Feedback Notification / Animation
    // -------------------------------------------------------------
    if (m_isLaunching) {
        m_launchProgress += dt * 1.5f;
        float barW = 360.0f;
        float barH = 28.0f;
        float barX = centerX - barW * 0.5f;
        float barY = winPos.y + winSize.y - 38.0f;

        draw->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(22, 27, 36, 240), 14.0f);
        draw->AddRect(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(34, 197, 94, 180), 14.0f, 0, 1.0f);

        float fillW = barW * (m_launchProgress < 1.0f ? m_launchProgress : 1.0f);
        draw->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + fillW, barY + barH), IM_COL32(34, 197, 94, 60), 14.0f);

        const char* msg = (m_launchProgress < 1.0f) ? m_statusMsg.c_str() : "Successfully Launched! Have fun!";
        if (m_fontSmall) ImGui::PushFont(m_fontSmall);
        ImVec2 msgSz = ImGui::CalcTextSize(msg);
        draw->AddText(ImVec2(barX + (barW - msgSz.x) * 0.5f, barY + (barH - msgSz.y) * 0.5f), IM_COL32(241, 245, 249, 255), msg);
        if (m_fontSmall) ImGui::PopFont();

        if (m_launchProgress > 3.0f) {
            m_isLaunching = false;
        }
    }
}

void AutorizationApp::Render(HWND hwnd) {
    float dt = ImGui::GetIO().DeltaTime;
    m_globalTime += dt;

    // Handle borderless window dragging
    HandleWindowDrag(hwnd);

    // Fullscreen ImGui window inside client area
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_currentWinWidth, (float)m_currentWinHeight));

    ImGuiWindowFlags winFlags = 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoSavedSettings | 
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::Begin("##AutorizationMain", nullptr, winFlags);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImVec2(0, 0);
    ImVec2 winSize = ImVec2((float)m_currentWinWidth, (float)m_currentWinHeight);

    // Pure Black Window Background (#0B0D11) with Sleek Border
    draw->AddRectFilled(winPos, winSize, IM_COL32(11, 13, 17, 255), 16.0f);
    draw->AddRect(winPos, winSize, IM_COL32(36, 42, 54, 255), 16.0f, 0, 1.2f);

    // Subtle celestial stardust ambient particles (no cursor trail)
    UpdateAndDrawParticles(draw, winPos, winSize, dt);

    // Render corresponding screen based on state
    if (m_currentScreen == 0) {
        RenderLoginScreen(hwnd, draw, winPos, winSize);
    } else if (m_currentScreen == 1) {
        RenderSubscriptionScreen(hwnd, draw, winPos, winSize);
    } else {
        m_cheat.Render(hwnd, m_fontRegular, m_fontSmall, m_fontSemiBold, m_fontHeader);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
