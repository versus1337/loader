#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winternl.h>
#include <string>
#include <vector>
#include <cstdint>
#include <regex>

namespace cheat {

struct ModuleInfo {
    uintptr_t   base = 0;
    size_t      size = 0;
    std::string name;
};

class ProcMem {
public:
    bool Attach(uint32_t pid);
    void Detach();
    bool IsAlive() const;

    uint32_t Pid() const { return m_pid; }

    bool Read(uintptr_t addr, void* out, size_t len) const;
    bool Write(uintptr_t addr, const void* data, size_t len) const;
    bool WriteProtected(uintptr_t addr, const void* data, size_t len) const;
    bool QueryRegion(uintptr_t addr, PMEMORY_BASIC_INFORMATION out) const;

    std::vector<ModuleInfo> Modules() const;
    ModuleInfo FindModule(const std::regex& rx) const;
    size_t ModuleSize(uintptr_t base) const;
    std::string ImageNameAt(uintptr_t base) const;

private:
    HANDLE   m_h   = nullptr;
    uint32_t m_pid = 0;
};

bool EnableDebugPrivilege();
bool FindProcessByName(const wchar_t* exe, uint32_t* outPid);

} // namespace cheat