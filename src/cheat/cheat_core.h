#pragma once
#include "proc_mem.h"
#include "sig_scan.h"
#include <vector>
#include <string>
#include <mutex>
#include <deque>
#include <atomic>
#include <thread>

namespace cheat {

enum ModKey { MOD_OBJ = 0, MOD_CSH, MOD_WOLF, MOD_MSVCR, MOD_D3D9, MOD_COUNT };

struct Patch {
    std::string          name;
    ModKey               mod = MOD_OBJ;
    int                  offset = 0;
    int                  defOffset = 0;
    int                  group = 0;
    int                  capLen = 0;
    uint8_t              origByte = 0;
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> original;
    uintptr_t            originalBase = 0;
    Pattern              sig;
    std::string          sigStr;
    bool                 sigFixed = false;
    bool                 verified = false, writeFailed = false, relocated = false, warned = false;
};

class Engine {
public:
    static Engine& Get();

    bool Launch();
    void Shutdown();
    bool Running() const { return m_running.load(); }

    void SetGroup(int group, bool on);
    bool GroupState(int group) const;
    void SetArmed(bool on) { m_armed.store(on); }
    bool Armed() const { return m_armed.load(); }
    void SetReloc(bool on) { m_reloc.store(on); }
    bool Reloc() const { return m_reloc.load(); }

    void UpdateOffsetsFromSigs();
    void Validate();
    void RestoreAll();

    void SetNsByte(int idx) { m_nsByte.store(idx); ApplyComboBytes(); }
    void SetRfByte(int idx) { m_rfByte.store(idx); ApplyComboBytes(); }
    int  NsByte() const { return m_nsByte.load(); }
    int  RfByte() const { return m_rfByte.load(); }

    std::vector<std::string> LogSnapshot(size_t maxLines) const;
    std::string StatusLine() const;
    uint32_t Pid() const { return m_pid.load(); }

    static const char* GroupLabel(int group);
    static int GroupCount();

private:
    Engine();

    void BuildPatches();
    void ApplyFixedSigs();
    void ApplyComboBytes();
    void RefreshModules();
    void ThreadMain();
    void Tick();
    void Log(const std::string& line);
    void SaveOffsets();
    void LoadOffsets();

    bool ResolveOffset(Patch& p, int& newOffset, bool& exact, bool& confident);
    bool EnsureSig(Patch& p);
    bool MakeSigUnique(Patch& p);
    bool SigMatchesAt(uintptr_t addr, const Patch& p);
    uintptr_t PickBestHit(const std::vector<uintptr_t>& hits, uintptr_t base, int curOff);
    bool TryRelocate(Patch& p);
    std::string ModuleOf(uintptr_t addr) const;

    ProcMem           m_mem;
    std::vector<Patch> m_patches;
    uintptr_t         m_bases[MOD_COUNT]{};
    std::string       m_offFile;
    uint64_t          m_lastRefresh = 0;

    mutable std::mutex      m_logMutex;
    std::deque<std::string> m_log;

    mutable std::mutex m_stateMutex;
    std::vector<bool>  m_groups;

    std::thread       m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_armed{false};
    std::atomic<bool> m_reloc{true};
    std::atomic<uint32_t> m_pid{0};
    std::atomic<int>  m_nsByte{0};
    std::atomic<int>  m_rfByte{0};
};

} // namespace cheat