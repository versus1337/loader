#pragma once
#include "proc_mem.h"
#include <vector>
#include <string>

namespace cheat {

struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<bool>    wild;
    bool Valid() const { return !bytes.empty(); }
};

bool ParsePattern(const std::string& text, Pattern* out);
std::string PatternToString(const Pattern& p);

std::vector<uintptr_t> ScanRange(const ProcMem& mem, uintptr_t start, uintptr_t end,
                                 const Pattern& pat, size_t cap);
std::vector<uintptr_t> ScanAll(const ProcMem& mem, const Pattern& pat, size_t cap);

} // namespace cheat