#include "sig_scan.h"
#include <sstream>
#include <cstdio>
#include <algorithm>

namespace cheat {

static constexpr size_t CHUNK = 0x400000;

bool ParsePattern(const std::string& text, Pattern* out) {
    out->bytes.clear();
    out->wild.clear();
    std::istringstream ss(text);
    std::string tok;
    while (ss >> tok) {
        if (tok == "?" || tok == "??") {
            out->bytes.push_back(0);
            out->wild.push_back(true);
        } else {
            char* end = nullptr;
            long v = strtol(tok.c_str(), &end, 16);
            if (end == tok.c_str() || v < 0 || v > 0xFF) return false;
            out->bytes.push_back((uint8_t)v);
            out->wild.push_back(false);
        }
    }
    return !out->bytes.empty();
}

std::string PatternToString(const Pattern& p) {
    std::string s;
    char buf[8];
    for (size_t i = 0; i < p.bytes.size(); ++i) {
        if (i) s += ' ';
        if (p.wild[i]) s += "??";
        else { sprintf_s(buf, "%02X", p.bytes[i]); s += buf; }
    }
    return s;
}

static bool Matches(const uint8_t* data, const Pattern& pat) {
    for (size_t i = 0; i < pat.bytes.size(); ++i)
        if (!pat.wild[i] && data[i] != pat.bytes[i]) return false;
    return true;
}

std::vector<uintptr_t> ScanRange(const ProcMem& mem, uintptr_t start, uintptr_t end,
                                 const Pattern& pat, size_t cap) {
    std::vector<uintptr_t> res;
    if (!pat.Valid() || !mem.IsAlive()) return res;

    std::vector<uint8_t> buf(CHUNK + 64);
    uintptr_t a = start;

    while (a < end && res.size() < cap) {
        uintptr_t regionEnd = end;
        MEMORY_BASIC_INFORMATION mbi{};
        if (mem.QueryRegion(a, &mbi)) {
            size_t rs = (size_t)mbi.RegionSize;
            if (rs) regionEnd = (std::min)(end, (uintptr_t)mbi.BaseAddress + rs);
            bool readable = mbi.State == MEM_COMMIT
                         && !(mbi.Protect & PAGE_NOACCESS)
                         && !(mbi.Protect & PAGE_GUARD);
            if (!readable) { a = regionEnd > a ? regionEnd : a + 0x1000; continue; }
        }

        size_t take = (std::min)(CHUNK, (size_t)(regionEnd - a));
        if (!take) { a += 0x1000; continue; }
        size_t readLen = (std::min)(take + pat.bytes.size() - 1, (size_t)(regionEnd - a));
        if (!mem.Read(a, buf.data(), readLen)) { a += take; continue; }

        size_t maxOff = readLen - pat.bytes.size();
        for (size_t i = 0; i <= maxOff && i < take; ++i) {
            if (Matches(buf.data() + i, pat)) {
                res.push_back(a + i);
                if (res.size() >= cap) break;
            }
        }
        a += take;
    }
    return res;
}

std::vector<uintptr_t> ScanAll(const ProcMem& mem, const Pattern& pat, size_t cap) {
    return ScanRange(mem, 0x10000, 0x7FFFFFFF, pat, cap);
}

} // namespace cheat