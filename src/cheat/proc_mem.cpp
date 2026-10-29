#include "proc_mem.h"
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace {

typedef LONG(NTAPI* pNtReadVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, SIZE_T*);
typedef LONG(NTAPI* pNtWriteVirtualMemory)(HANDLE, PVOID, PVOID, SIZE_T, SIZE_T*);
typedef LONG(NTAPI* pNtProtectVirtualMemory)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
typedef LONG(NTAPI* pNtQueryVirtualMemory)(HANDLE, PVOID, ULONG,
                                           PMEMORY_BASIC_INFORMATION, SIZE_T, PSIZE_T);

struct NtApi {
    pNtReadVirtualMemory    Read    = nullptr;
    pNtWriteVirtualMemory   Write   = nullptr;
    pNtProtectVirtualMemory Protect = nullptr;
    pNtQueryVirtualMemory   Query   = nullptr;
    bool ok = false;

    NtApi() {
        HMODULE n = GetModuleHandleW(L"ntdll.dll");
        if (!n) return;
        Read    = (pNtReadVirtualMemory)   GetProcAddress(n, "NtReadVirtualMemory");
        Write   = (pNtWriteVirtualMemory)  GetProcAddress(n, "NtWriteVirtualMemory");
        Protect = (pNtProtectVirtualMemory)GetProcAddress(n, "NtProtectVirtualMemory");
        Query   = (pNtQueryVirtualMemory)  GetProcAddress(n, "NtQueryVirtualMemory");
        ok = Read && Write && Protect && Query;
    }
};

NtApi g_nt;

constexpr uint32_t ACCESS_NEEDED =
    PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION;

constexpr uint32_t MEM_IMAGE_ = 0x1000000, MEM_MAPPED_ = 0x40000;

} // namespace

namespace cheat {

bool EnableDebugPrivilege() {
    HANDLE th = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &th))
        return false;
    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    BOOL ok = LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &tp.Privileges[0].Luid);
    if (ok) AdjustTokenPrivileges(th, FALSE, &tp, 0, nullptr, nullptr);
    CloseHandle(th);
    return ok != FALSE;
}

bool FindProcessByName(const wchar_t* exe, uint32_t* outPid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exe) == 0) { *outPid = pe.th32ProcessID; found = true; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

bool ProcMem::Attach(uint32_t pid) {
    Detach();
    if (!g_nt.ok) return false;
    m_h = OpenProcess(ACCESS_NEEDED, FALSE, pid);
    if (!m_h) m_h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!m_h) return false;
    m_pid = pid;
    return true;
}

void ProcMem::Detach() {
    if (m_h) { CloseHandle(m_h); m_h = nullptr; }
    m_pid = 0;
}

bool ProcMem::IsAlive() const {
    if (!m_h) return false;
    DWORD code = 0;
    return GetExitCodeProcess(m_h, &code) && code == STILL_ACTIVE;
}

bool ProcMem::Read(uintptr_t addr, void* out, size_t len) const {
    SIZE_T got = 0;
    return m_h && g_nt.Read(m_h, (PVOID)addr, out, len, &got) == 0 && got == len;
}

bool ProcMem::Write(uintptr_t addr, const void* data, size_t len) const {
    SIZE_T put = 0;
    return m_h && g_nt.Write(m_h, (PVOID)addr, (PVOID)data, len, &put) == 0;
}

bool ProcMem::WriteProtected(uintptr_t addr, const void* data, size_t len) const {
    if (!m_h) return false;
    const size_t page = 0x1000;
    uintptr_t pageStart = addr & ~(page - 1);
    size_t span  = (addr - pageStart) + len;
    size_t pages = (span + page - 1) & ~(page - 1);

    PVOID  protAddr = (PVOID)pageStart;
    SIZE_T protSize = pages;
    ULONG  oldProt  = 0;
    if (g_nt.Protect(m_h, &protAddr, &protSize, PAGE_EXECUTE_READWRITE, &oldProt) != 0)
        return false;

    bool ok = Write(addr, data, len);

    PVOID  p2 = (PVOID)pageStart;
    SIZE_T s2 = pages;
    ULONG  dummy = 0;
    g_nt.Protect(m_h, &p2, &s2, oldProt, &dummy);
    return ok;
}

bool ProcMem::QueryRegion(uintptr_t addr, PMEMORY_BASIC_INFORMATION out) const {
    SIZE_T outLen = 0;
    return m_h && g_nt.Query(m_h, (PVOID)addr, 0, out, sizeof(*out), &outLen) == 0;
}

std::string ProcMem::ImageNameAt(uintptr_t base) const {
    uint16_t mz = 0;
    if (!Read(base, &mz, sizeof(mz)) || mz != 0x5A4D) return {};
    int32_t e = 0;
    if (!Read(base + 0x3C, &e, sizeof(e)) || e <= 0 || e > 0x1000) return {};
    uintptr_t pe = base + e;
    uint32_t sig = 0;
    if (!Read(pe, &sig, sizeof(sig)) || sig != 0x4550) return {};
    uint16_t magic = 0;
    if (!Read(pe + 24, &magic, sizeof(magic))) return {};
    uint32_t ddOff = (magic == 0x10B) ? 96 : 112;
    uint32_t expRva = 0;
    if (!Read(pe + 24 + ddOff, &expRva, sizeof(expRva)) || expRva == 0) return {};
    uint32_t nameRva = 0;
    if (!Read(base + expRva + 12, &nameRva, sizeof(nameRva)) || nameRva == 0) return {};
    char buf[64]{};
    if (!Read(base + nameRva, buf, sizeof(buf) - 1)) return {};
    return std::string(buf);
}

size_t ProcMem::ModuleSize(uintptr_t base) const {
    int32_t e = 0;
    if (!Read(base + 0x3C, &e, sizeof(e)) || e <= 0 || e > 0x1000) return 0;
    uint32_t sizeOfImage = 0;
    if (!Read(base + e + 24 + 56, &sizeOfImage, sizeof(sizeOfImage))) return 0;
    return sizeOfImage;
}

std::vector<ModuleInfo> ProcMem::Modules() const {
    std::vector<ModuleInfo> out;
    if (!m_h) return out;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) {
            do {
                ModuleInfo mi;
                mi.base = (uintptr_t)me.modBaseAddr;
                mi.size = me.modBaseSize;
                char narrow[MAX_MODULE_NAME32 + 1]{};
                WideCharToMultiByte(CP_ACP, 0, me.szModule, -1, narrow, sizeof(narrow) - 1, nullptr, nullptr);
                mi.name = narrow;
                out.push_back(mi);
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
    }

    uintptr_t addr = 0x10000;
    while (addr < 0x7FFFFFFF) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!QueryRegion(addr, &mbi)) break;
        size_t rs = (size_t)mbi.RegionSize;
        if (!rs) break;
        if (mbi.State == MEM_COMMIT && (mbi.Type == MEM_IMAGE_ || mbi.Type == MEM_MAPPED_)) {
            std::string nm = ImageNameAt((uintptr_t)mbi.BaseAddress);
            if (!nm.empty()) {
                ModuleInfo mi;
                mi.base = (uintptr_t)mbi.BaseAddress;
                mi.size = rs;
                mi.name = nm;
                out.push_back(mi);
            }
        }
        addr = (uintptr_t)mbi.BaseAddress + rs;
    }
    return out;
}

ModuleInfo ProcMem::FindModule(const std::regex& rx) const {
    for (const auto& m : Modules())
        if (std::regex_match(m.name, rx))
            return m;
    return {};
}

} // namespace cheat