#include "cheat_core.h"
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <regex>

namespace cheat {

static const char* kModPatterns[MOD_COUNT] = {
    "^(obj[a-z0-9]*\\.tmp|Object\\.lto)$",
    "^(csh[a-z0-9]*\\.tmp|CShell\\.dll)$",
    "^wolfteam\\.bin$",
    "^msvcr100\\.dll$",
    "^d3d9\\.dll$"
};

static const char* kModNames[MOD_COUNT] = { "obj", "csh", "wolf", "msvcr", "d3d9" };

static const char* kGroups[] = {
    "Restore HP", "Always HeadShot", "God Mode", "Remote Kill", "HP Booster",
    "All Freeze Player", "Powerful Bomb", "Bomb No Bounce", "Aimbot Ultra",
    "No Recoil", "Rapid Fire", "Jump Hack", "Infinite Ammo", "No Reload",
    "No Fall Damage", "Wolf Dash Speed", "Infinite Wolf Balls", "Anti Chat Ban",
    "Instant Respawn", "Respawn Where Died", "No Weapon Sp", "Movement Speed",
    "WallShot", "NoClip", "Swear Hack", "No Spread", "Name ESP"
};

enum {
    G_RESTORE_HP, G_ALWAYS_HS, G_GOD, G_REMOTE_KILL, G_HP_BOOST,
    G_FREEZE, G_PBOMB, G_NBOUNCE, G_AIMBOT, G_NORECOIL, G_RAPID,
    G_JUMP, G_INFAMMO, G_NORELOAD, G_NOFALL, G_WOLFDASH, G_INFWOLF,
    G_CHATBAN, G_RESPAWN, G_RESPAWNDIE, G_NOWSP, G_MOVE, G_WALLSHOT,
    G_NOCLIP, G_SWEAR, G_NOSPREAD, G_NAMESP
};

static std::string Hex2(uint8_t b) {
    char buf[8];
    sprintf_s(buf, "%02X", b);
    return buf;
}

static std::string BytesToHex(const std::vector<uint8_t>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) s += '-';
        s += Hex2(v[i]);
    }
    return s;
}

Engine& Engine::Get() { static Engine e; return e; }

Engine::Engine() {
    m_groups.assign(GroupCount(), false);
    char path[MAX_PATH]{};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (slash) *slash = '\0';
    m_offFile = std::string(path) + "\\offsets.txt";
    BuildPatches();
    LoadOffsets();
    ApplyFixedSigs();
    ApplyComboBytes();
}

const char* Engine::GroupLabel(int g) { return kGroups[g]; }
int Engine::GroupCount() { return (int)(sizeof(kGroups) / sizeof(kGroups[0])); }

void Engine::BuildPatches() {
    auto R = [&](const char* n, ModKey m, int off, std::vector<uint8_t> b,
                 int group, const char* sig, int cap = 0) {
        Patch p;
        p.name = n; p.mod = m; p.offset = off; p.defOffset = off;
        p.bytes = std::move(b); p.group = group; p.capLen = cap;
        if (sig && *sig) { p.sigStr = sig; p.sigFixed = true; }
        if (p.bytes.size() == 1 && p.bytes[0] == 0xD8) p.origByte = 0xD9;
        m_patches.push_back(std::move(p));
    };

    R("Restore HP", MOD_OBJ, 0x50318, {0xD8}, G_RESTORE_HP,
      "?? 45 ?? d9 5d ?? d9 45 ?? dc 1d ?? ?? ?? ?? df e0 f6 c4 ?? 7b");
    R("Always HeadShot", MOD_OBJ, 0x46003C, {0xD8}, G_ALWAYS_HS,
      "?? 45 ?? d9 5d ?? d9 45 ?? dc 1d ?? ?? ?? ?? df e0 f6 c4 ?? 7b");
    R("God Mode", MOD_OBJ, 0x1EF6D, {0xD8}, G_GOD, "");
    R("Remote Kill", MOD_OBJ, 0xE3109, {0x74}, G_REMOTE_KILL, "");
    R("HP Booster", MOD_OBJ, 0x5C89F, {0x75}, G_HP_BOOST, "");
    R("All Freeze Player", MOD_OBJ, 0x3D637, {0x74}, G_FREEZE, "");
    R("Powerful Bomb", MOD_OBJ, 0x139437, {0xF6,0xC4,0x41,0x74}, G_PBOMB, "");
    R("Bomb No Bounce", MOD_OBJ, 0x139421, {0xF6,0xC4,0x41,0x74}, G_NBOUNCE, "");
    R("Aimbot Ultra", MOD_OBJ, 0x1C92A, {0xEB}, G_AIMBOT, "74 28 15 68 8D 58");

    R("No Recoil", MOD_CSH, 0x45A37C, {0,0,0,0}, G_NORECOIL, "?? ?? ?? ?? 00 00 34");
    R("Rapid Fire", MOD_CSH, 0x1218DA, {0x74}, G_RAPID, "", 2);
    R("Jump Hack", MOD_CSH, 0x4628E4, {0x00,0x00,0xC0,0x3F}, G_JUMP, "29 5C 4F 3F 00 50");
    R("Infinite Ammo", MOD_CSH, 0x676AD, {0xEB}, G_INFAMMO, "74 0E 83 F8 02 74 09 83 F8 05");
    R("No Reload", MOD_CSH, 0x1565EA, {0xEB}, G_NORELOAD, "?? 06 8B 86 6C 01");
    R("No Fall Damage", MOD_CSH, 0xF9D53, {0x74}, G_NOFALL, "EE C7 00 54 03 46");
    R("Wolf Dash Speed", MOD_CSH, 0xE5876, {0x75}, G_WOLFDASH, "");
    R("Infinite Wolf Balls", MOD_CSH, 0xBFB4C, {0xB0,0x01,0x90,0x88}, G_INFWOLF, "8A 45 08 88 86 E5");
    R("Anti Chat Ban", MOD_CSH, 0x168390, {0xF6,0x81,0x00,0x00}, G_CHATBAN, "80 B9 18 D2 03 00 00 0F");
    R("Instant Respawn", MOD_CSH, 0xA2060, {0x74}, G_RESPAWN, "34 46 EB 06 8B 35");
    R("Respawn Where Died", MOD_CSH, 0xF7D75, {0xEB}, G_RESPAWNDIE, "68 4B 72 3F 34 64");
    R("No Weapon Sp", MOD_CSH, 0x1E8E80, {0x74}, G_NOWSP, "75 12 0F B7 45 08");
    R("Movement Speed", MOD_CSH, 0x1ABC31, {0x75}, G_MOVE, "74 34 8D 8E 30 02 00 00 E8");
    R("Speed Hack Enable", MOD_CSH, 0x12461, {0x74}, G_MOVE, "75 0C D9 05 50 70");
    R("Speed Hack Value", MOD_CSH, 0x4373E8, {0x00,0x00,0x80,0x3F}, G_MOVE,
      "CD CC CC 3D 00 00 00 00 00");
    R("Name ESP", MOD_CSH, 0x3A21A8, {0xEB}, G_NAMESP, "74 ?? ?? ?? ?? ?? ?? 8B");
    R("No Spread", MOD_CSH, 0x123834, {0x74}, G_NOSPREAD, "", 2);

    R("WallShot 1", MOD_WOLF, 0x4F171, {0x05,0,0,0,0}, G_WALLSHOT, "15 C8 29 72 00 C7 40 4C");
    R("WallShot 2", MOD_WOLF, 0x67533, {0x05,0,0,0,0}, G_WALLSHOT, "F6 C4 41 75 15 D8 D5");
    R("WallShot 3", MOD_WOLF, 0x59F8B, {0x05,0,0,0,0}, G_WALLSHOT, "D8 4E 08 D9 46 04 D8 0E");
    R("WallShot 4", MOD_WOLF, 0x5A131, {0x05,0,0,0,0}, G_WALLSHOT, "85 1C FF FF FF D9 5D F8");
    R("NoClip", MOD_WOLF, 0x85F21, {0x0F,0x85,0xD3,0x00}, G_NOCLIP, "00 FF D6 68 CC 62");

    R("Swear Hack", MOD_MSVCR, 0x1339B, {0x75}, G_SWEAR, "74 3F 8B D1 2B C3");
}

void Engine::ApplyFixedSigs() {
    for (auto& p : m_patches)
        if (p.sigFixed) ParsePattern(p.sigStr, &p.sig);
}

void Engine::ApplyComboBytes() {
    static const uint8_t ns[][2] = { {0x74,0}, {0x90,0x90}, {0xEB,0}, {0x75,0} };
    static const int     nsLen[] = { 1, 2, 1, 1 };
    static const uint8_t rf[]    = { 0x74, 0x10, 0x75 };

    std::lock_guard<std::mutex> lk(m_stateMutex);
    for (auto& p : m_patches) {
        if (p.group == G_NOSPREAD) {
            int i = m_nsByte.load();
            p.bytes.assign(ns[i], ns[i] + nsLen[i]);
            p.verified = false;
        } else if (p.group == G_RAPID) {
            p.bytes.assign(1, rf[m_rfByte.load()]);
            p.verified = false;
        }
    }
}

void Engine::SetGroup(int group, bool on) {
    std::lock_guard<std::mutex> lk(m_stateMutex);
    if (group >= 0 && group < (int)m_groups.size()) m_groups[group] = on;
}

bool Engine::GroupState(int group) const {
    std::lock_guard<std::mutex> lk(m_stateMutex);
    return (group >= 0 && group < (int)m_groups.size()) ? m_groups[group] : false;
}

void Engine::Log(const std::string& line) {
    std::lock_guard<std::mutex> lk(m_logMutex);
    m_log.push_back(line);
    while (m_log.size() > 400) m_log.pop_front();
}

std::vector<std::string> Engine::LogSnapshot(size_t maxLines) const {
    std::lock_guard<std::mutex> lk(m_logMutex);
    size_t n = (std::min)(maxLines, m_log.size());
    return std::vector<std::string>(m_log.end() - n, m_log.end());
}

std::string Engine::ModuleOf(uintptr_t addr) const {
    for (int i = 0; i < MOD_COUNT; ++i) {
        uintptr_t b = m_bases[i];
        if (b && addr >= b && addr < b + 0x2000000) {
            char buf[64];
            sprintf_s(buf, "%s+%X", kModNames[i], (unsigned)(addr - b));
            return buf;
        }
    }
    char buf[24];
    sprintf_s(buf, "0x%08X", (unsigned)addr);
    return buf;
}

std::string Engine::StatusLine() const {
    if (!m_pid.load()) return "Bagli degil";
    char buf[256];
    sprintf_s(buf, "PID %u  |  obj:%s  csh:%s  wolf:%s  msvcr:%s  |  %s",
        (unsigned)m_pid.load(),
        m_bases[MOD_OBJ] ? "OK" : "--", m_bases[MOD_CSH] ? "OK" : "--",
        m_bases[MOD_WOLF] ? "OK" : "--", m_bases[MOD_MSVCR] ? "OK" : "--",
        m_armed.load() ? "ARMED" : "idle");
    return buf;
}

void Engine::SaveOffsets() {
    FILE* f = nullptr;
    if (fopen_s(&f, m_offFile.c_str(), "w") != 0 || !f) return;
    fprintf(f, "# WolfBypass offsets\n");
    for (const auto& p : m_patches)
        fprintf(f, "%s=%X\n", p.name.c_str(), p.offset);
    fclose(f);
}

void Engine::LoadOffsets() {
    FILE* f = nullptr;
    if (fopen_s(&f, m_offFile.c_str(), "r") != 0 || !f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') continue;
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        int v = (int)strtol(eq + 1, nullptr, 16);
        for (auto& p : m_patches)
            if (_stricmp(p.name.c_str(), line) == 0) p.offset = v;
    }
    fclose(f);
}

void Engine::RefreshModules() {
    if (!m_mem.IsAlive()) return;
    uint64_t now = GetTickCount64();
    if (now - m_lastRefresh < 1500) return;
    m_lastRefresh = now;

    uintptr_t found[MOD_COUNT]{};
    for (int i = 0; i < MOD_COUNT; ++i) {
        std::regex rx(kModPatterns[i], std::regex::icase);
        ModuleInfo mi = m_mem.FindModule(rx);
        found[i] = mi.base;
    }

    std::lock_guard<std::mutex> lk(m_stateMutex);
    for (int i = 0; i < MOD_COUNT; ++i) m_bases[i] = found[i];
}

bool Engine::SigMatchesAt(uintptr_t addr, const Patch& p) {
    if (!p.sig.Valid()) return true;
    std::vector<uint8_t> cur(p.sig.bytes.size());
    if (!m_mem.Read(addr, cur.data(), cur.size())) return false;
    for (size_t i = 0; i < p.sig.bytes.size(); ++i)
        if (!p.sig.wild[i] && cur[i] != p.sig.bytes[i]) return false;
    return true;
}

uintptr_t Engine::PickBestHit(const std::vector<uintptr_t>& hits, uintptr_t base, int curOff) {
    for (auto a : hits) if ((int)(a - base) == curOff) return a;
    uintptr_t best = hits[0];
    long bd = labs((long)(int)(best - base) - curOff);
    for (auto a : hits) {
        long d = labs((long)(int)(a - base) - curOff);
        if (d < bd) { bd = d; best = a; }
    }
    return best;
}

bool Engine::TryRelocate(Patch& p) {
    uintptr_t b = m_bases[p.mod];
    if (!b || !p.sig.Valid()) return false;
    size_t size = m_mem.ModuleSize(b);
    if (!size) return false;

    auto hits = ScanRange(m_mem, b, b + size, p.sig, 64);
    if (hits.empty()) return false;

    uintptr_t best = PickBestHit(hits, b, p.offset);
    int no = (int)(best - b);
    if (no != p.offset) {
        char buf[128];
        sprintf_s(buf, "[DEGISTI] %s %X -> %X", p.name.c_str(), p.offset, no);
        Log(buf);
        p.offset = no;
        p.verified = false;
        return true;
    }
    return false;
}

bool Engine::EnsureSig(Patch& p) {
    if (p.sig.Valid()) return true;
    uintptr_t b = m_bases[p.mod];
    if (!b) return false;
    size_t size = m_mem.ModuleSize(b);
    if (!size) return false;

    std::vector<uint8_t> raw(40);
    if (!m_mem.Read(b + p.offset, raw.data(), raw.size())) return false;

    for (size_t len = 6; len <= 32; ++len) {
        Pattern pat;
        pat.bytes.assign(raw.begin(), raw.begin() + len);
        pat.wild.assign(len, false);
        for (int i = 0; i < p.capLen && i < (int)len; ++i) pat.wild[i] = true;
        auto hits = ScanRange(m_mem, b, b + size, pat, 8);
        if (hits.size() == 1) {
            p.sig = pat;
            p.sigStr = PatternToString(pat);
            return true;
        }
    }
    return false;
}

bool Engine::MakeSigUnique(Patch& p) {
    uintptr_t b = m_bases[p.mod];
    if (!b) return false;
    size_t size = m_mem.ModuleSize(b);
    if (!size) return false;

    std::vector<uint8_t> raw(64);
    if (!m_mem.Read(b + p.offset, raw.data(), raw.size())) return false;

    for (size_t len = p.bytes.size() + 3; len <= 48 && len <= raw.size(); ++len) {
        Pattern pat;
        pat.bytes.assign(raw.begin(), raw.begin() + len);
        pat.wild.assign(len, false);
        for (size_t i = 0; i < p.bytes.size() && i < len; ++i) pat.wild[i] = true;
        auto hits = ScanRange(m_mem, b, b + size, pat, 8);
        if (hits.size() == 1) {
            p.sig = pat;
            p.sigStr = PatternToString(pat);
            return true;
        }
    }
    return false;
}

int Engine::ResolveOffset(Patch& p, int& newOffset, bool& exact, bool& confident) {
    newOffset = p.offset;
    exact = confident = false;
    if (!p.sig.Valid()) return 0;

    uintptr_t b = m_bases[p.mod];
    if (!b) return 0;
    size_t size = m_mem.ModuleSize(b);
    if (!size) return 0;

    auto hits = ScanRange(m_mem, b, b + size, p.sig, 64);
    if (hits.empty()) return 0;

    for (auto a : hits)
        if ((int)(a - b) == p.offset) { exact = true; break; }

    uintptr_t best = PickBestHit(hits, b, p.offset);
    newOffset = (int)(best - b);

    if (hits.size() == 1 || exact) confident = true;
    else {
        long d1 = LONG_MAX, d2 = LONG_MAX;
        for (auto a : hits) {
            long d = labs((long)(int)(a - b) - p.offset);
            if (d < d1) { d2 = d1; d1 = d; }
            else if (d < d2) d2 = d;
        }
        confident = (d1 == 0) || (d1 * 3 < d2);
    }
    return (int)hits.size();
}

void Engine::UpdateOffsetsFromSigs() {
    if (!m_mem.IsAlive()) { Log("Once baglan!"); return; }

    {
        std::lock_guard<std::mutex> lk(m_stateMutex);
        for (int i = 0; i < MOD_COUNT; ++i) {
            std::regex rx(kModPatterns[i], std::regex::icase);
            ModuleInfo mi = m_mem.FindModule(rx);
            m_bases[i] = mi.base;
        }
    }

    Log("=== IMZADAN OFSET GUNCELLEME ===");
    int tek = 0, cok = 0, hata = 0, degisen = 0, supheli = 0, uzatilan = 0;

    std::lock_guard<std::mutex> lk(m_stateMutex);
    for (auto& p : m_patches) {
        if (!m_bases[p.mod]) { hata++; Log(p.name + " [modul yok]"); continue; }
        if (!p.sig.Valid() && !EnsureSig(p)) { hata++; Log(p.name + " [imza yok]"); continue; }

        int yeni = 0; bool exact = false, conf = false;
        int adet = ResolveOffset(p, yeni, exact, conf);

        if (adet > 1 && !conf && MakeSigUnique(p)) {
            uzatilan++;
            adet = ResolveOffset(p, yeni, exact, conf);
            Log(p.name + " [IMZA UZATILDI] " + p.sigStr);
        }

        if (adet == 0) { hata++; Log(p.name + " [BULUNAMADI]"); continue; }

        std::string tag = adet > 1 ? ("   [COKLU " + std::to_string(adet) + "]") : "";

        if (adet == 1 || exact || conf) {
            if (yeni == p.offset) Log(p.name + " " + std::to_string(p.offset) + " (ayni)" + tag);
            else {
                Log(p.name + " " + std::to_string(p.offset) + " -> " + std::to_string(yeni) + "  *GUNCELLENDI*" + tag);
                p.offset = yeni; p.verified = false; p.relocated = false;
                degisen++;
            }
            if (adet == 1) tek++; else cok++;
        } else {
            supheli++;
            Log(p.name + " " + std::to_string(p.offset) + " (SUPHELI, aday: " + std::to_string(yeni) + ")" + tag);
        }
    }

    char sum[192];
    sprintf_s(sum, "TEK: %d  COKLU: %d  UZATILAN: %d  DEGISEN: %d  SUPHELI: %d  hata: %d",
              tek, cok, uzatilan, degisen, supheli, hata);
    Log(sum);
    SaveOffsets();
    Log(">>> offsets.txt kaydedildi.");
}

void Engine::Validate() {
    if (!m_mem.IsAlive()) { Log("Once baglan!"); return; }
    Log("Dogrulama (yazma YOK)...");
    int ok = 0, bad = 0, unk = 0;

    std::lock_guard<std::mutex> lk(m_stateMutex);
    for (auto& p : m_patches) {
        uintptr_t b = m_bases[p.mod];
        if (!b) { Log(p.name + " [modul yok]"); unk++; continue; }

        size_t len = (std::max)(p.sig.bytes.size(), p.bytes.size());
        std::vector<uint8_t> cur(len);
        if (!m_mem.Read(b + p.offset, cur.data(), len)) { Log(p.name + " [okunamadi]"); unk++; continue; }

        bool sigOk = true;
        for (size_t i = 0; i < p.sig.bytes.size(); ++i)
            if (!p.sig.wild[i] && cur[i] != p.sig.bytes[i]) { sigOk = false; break; }

        bool patched = true;
        for (size_t i = 0; i < p.bytes.size(); ++i)
            if (cur[i] != p.bytes[i]) { patched = false; break; }

        if (sigOk || patched) { ok++; Log(p.name + (patched ? "  OK (uygulanmis)" : "  OK")); }
        else { bad++; Log(p.name + "  UYUSMUYOR -> " + BytesToHex(cur)); }
    }
    char sum[96];
    sprintf_s(sum, "Uyumlu: %d  Supheli: %d  Bilinmez: %d", ok, bad, unk);
    Log(sum);
}

void Engine::RestoreAll() {
    if (!m_mem.IsAlive()) return;
    std::lock_guard<std::mutex> lk(m_stateMutex);
    for (auto& p : m_patches) {
        if (p.original.empty()) continue;
        uintptr_t b = m_bases[p.mod];
        if (!b || b != p.originalBase) continue;

        uintptr_t addr = b + p.offset;
        std::vector<uint8_t> cur(p.original.size());
        if (!m_mem.Read(addr, cur.data(), cur.size())) continue;
        if (std::equal(p.bytes.begin(), p.bytes.end(), cur.begin()))
            m_mem.WriteProtected(addr, p.original.data(), p.original.size());
    }
}

void Engine::Tick() {
    if (!m_armed.load()) return;

    std::lock_guard<std::mutex> lk(m_stateMutex);
    for (auto& p : m_patches) {
        uintptr_t b = m_bases[p.mod];
        if (!b) continue;

        uintptr_t addr = b + p.offset;
        size_t len = p.bytes.size();
        std::vector<uint8_t> cur(len);
        if (!m_mem.Read(addr, cur.data(), len)) continue;

        bool on = m_groups[p.group];

        if (on) {
            if (std::equal(cur.begin(), cur.end(), p.bytes.begin())) { p.verified = true; continue; }

            if (p.origByte && cur[0] != p.bytes[0] && cur[0] != p.origByte && !p.warned) {
                p.warned = true;
                Log("[OFFSET SUPHELI] " + p.name + " @ " + ModuleOf(addr) +
                    " bayt=0x" + Hex2(cur[0]) + " (beklenen 0x" + Hex2(p.origByte) +
                    " / yama 0x" + Hex2(p.bytes[0]) + ") -> offset kaymis olabilir");
            }

            if (p.sig.Valid() && !SigMatchesAt(addr, p)) {
                if (!p.relocated && m_reloc.load() && TryRelocate(p)) {
                    p.relocated = true;
                    addr = b + p.offset;
                    if (!m_mem.Read(addr, cur.data(), len)) continue;
                    if (std::equal(cur.begin(), cur.end(), p.bytes.begin())) { p.verified = true; continue; }
                } else if (!m_reloc.load() && !p.warned) {
                    p.warned = true;
                    Log("[SIG UYUSMUYOR | RELOC KAPALI] " + p.name + " @ " + ModuleOf(addr));
                }
            }

            size_t cap = p.capLen > 0 ? (size_t)p.capLen : len;
            if (p.original.empty() || p.originalBase != b || p.original.size() < cap) {
                std::vector<uint8_t> raw(cap);
                if (m_mem.Read(addr, raw.data(), cap)) { p.original = raw; p.originalBase = b; }
            }

            if (!m_mem.WriteProtected(addr, p.bytes.data(), len)) {
                if (!p.writeFailed) { p.writeFailed = true; Log("[YAZILAMADI] " + p.name + " @ " + ModuleOf(addr)); }
            } else {
                if (!p.verified) { p.verified = true; Log("[YAZILDI] " + p.name + " @ " + ModuleOf(addr) + " : " + BytesToHex(p.bytes)); }
                std::vector<uint8_t> back(len);
                if (m_mem.Read(addr, back.data(), len) &&
                    !std::equal(back.begin(), back.end(), p.bytes.begin()) && !p.warned) {
                    p.warned = true;
                    Log("[GERI ALINDI?] " + p.name + " - bayt yazildiktan sonra degismis");
                }
            }
        } else {
            p.verified = p.relocated = p.writeFailed = p.warned = false;
            if (!p.original.empty() && p.originalBase == b) {
                std::vector<uint8_t> c2(p.original.size());
                if (m_mem.Read(addr, c2.data(), c2.size()) &&
                    std::equal(p.bytes.begin(), p.bytes.end(), c2.begin()))
                    m_mem.WriteProtected(addr, p.original.data(), p.original.size());
            }
        }
    }
}

void Engine::ThreadMain() {
    while (m_running.load()) {
        if (!m_mem.IsAlive()) {
            Log(">>> Oyun kapandi, baglanti kesildi.");
            m_armed.store(false);
            m_running.store(false);
            break;
        }
        RefreshModules();
        Tick();
        Sleep(100);
    }
    m_mem.Detach();
    m_pid.store(0);
}

bool Engine::Launch() {
    if (m_running.load()) return true;
    EnableDebugPrivilege();

    uint32_t pid = 0;
    if (!FindProcessByName(L"WolfTeam.bin", &pid) && !FindProcessByName(L"WolfTeam.exe", &pid)) {
        Log("WolfTeam acik degil!");
        return false;
    }
    if (!m_mem.Attach(pid)) {
        Log("OpenProcess basarisiz. Yonetici olarak calistir.");
        return false;
    }

    {
        std::lock_guard<std::mutex> lk(m_stateMutex);
        for (auto& p : m_patches) {
            p.original.clear(); p.originalBase = 0;
            p.verified = p.writeFailed = p.relocated = p.warned = false;
        }
    }

    m_pid.store(pid);
    Log(">>> Baglandi. PID " + std::to_string(pid));
    Log(">>> Once 'OFSET GUNCELLE', sonra 'PATCH AC'.");

    m_running.store(true);
    m_thread = std::thread(&Engine::ThreadMain, this);
    return true;
}

void Engine::Shutdown() {
    if (!m_running.load() && !m_mem.IsAlive()) return;
    RestoreAll();
    m_armed.store(false);
    m_running.store(false);
    if (m_thread.joinable()) m_thread.join();
    m_mem.Detach();
    m_pid.store(0);
    memset(m_bases, 0, sizeof(m_bases));
    Log(">>> Kesildi.");
}

} // namespace cheat