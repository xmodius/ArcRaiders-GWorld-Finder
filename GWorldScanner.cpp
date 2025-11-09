/*
 * Arc Raiders GWorld Scanner
 * Automated GWorld offset finder using DMA hardware
 * Version: 2.0
 */

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

// Forward declare to avoid header dependency issues
typedef void* VMM_HANDLE;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef BYTE* PBYTE;
typedef unsigned long long QWORD;

// Function pointer types for dynamic loading
typedef VMM_HANDLE(*VMMDLL_Initialize_t)(DWORD argc, LPCSTR argv[]);
typedef void(*VMMDLL_Close_t)(VMM_HANDLE hVMM);
typedef BOOL(*VMMDLL_PidList_t)(VMM_HANDLE hVMM, PDWORD pPIDs, PSIZE_T pcPIDs);
typedef BOOL(*VMMDLL_ProcessGetInformation_t)(VMM_HANDLE hVMM, DWORD dwPID, PVOID pInfo, PSIZE_T pcbInfo);
typedef BOOL(*VMMDLL_Map_GetModuleU_t)(VMM_HANDLE hVMM, DWORD dwPID, PVOID pModuleMap, DWORD flags);
typedef BOOL(*VMMDLL_MemReadEx_t)(VMM_HANDLE hVMM, DWORD dwPID, QWORD qwA, PBYTE pb, DWORD cb, PDWORD pcbReadOpt, QWORD flags);
typedef void(*VMMDLL_MemFree_t)(PVOID pvMem);

// Known offsets from comprehensive dumper
#define GNAMES_OFFSET 0x7E97580

// UE5 Structure offsets
#define UWORLD_PERSISTENTLEVEL 0x38
#define UWORLD_OWNINGGAMEINSTANCE 0x1A0
#define UWORLD_LEVELS 0x178
#define UWORLD_GAMESTATE 0x158
#define ULEVEL_ACTORS 0xA0
#define ULEVEL_ACTORCOUNT 0xA8

// Scanning parameters
#define SCAN_RANGE_MB 100
#define CHUNK_SIZE (2 * 1024 * 1024)
#define MIN_CONFIDENCE 50

struct GWorldCandidate {
    uint64_t offset;
    uint64_t address;
    int score;
    uint64_t uWorldPtr;
    uint32_t actorCount;
};

// Global state
HMODULE hVmmDll = nullptr;
VMM_HANDLE hVMM = nullptr;
DWORD processId = 0;
uint64_t moduleBase = 0;

// Function pointers
VMMDLL_Initialize_t pVMMDLL_Initialize = nullptr;
VMMDLL_Close_t pVMMDLL_Close = nullptr;
VMMDLL_PidList_t pVMMDLL_PidList = nullptr;
VMMDLL_ProcessGetInformation_t pVMMDLL_ProcessGetInformation = nullptr;
VMMDLL_Map_GetModuleU_t pVMMDLL_Map_GetModuleU = nullptr;
VMMDLL_MemReadEx_t pVMMDLL_MemReadEx = nullptr;
VMMDLL_MemFree_t pVMMDLL_MemFree = nullptr;

void Log(const std::string& msg) {
    std::cout << msg << std::endl;
}

void LogError(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

bool LoadVmmDll() {
    Log("[*] Loading vmm.dll...");
    
    // Try loading with full path
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = exePath;
    size_t lastSlash = exeDir.find_last_of("\\");
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash + 1);
    }
    std::string dllPath = exeDir + "vmm.dll";
    
    Log("    Trying: " + dllPath);
    
    // Set DLL directory to help with dependencies
    SetDllDirectoryA(exeDir.c_str());
    
    hVmmDll = LoadLibraryA(dllPath.c_str());
    if (!hVmmDll) {
        DWORD error = GetLastError();
        LogError("Failed to load vmm.dll");
        LogError("Windows Error Code: " + std::to_string(error));
        
        if (error == 126) {
            LogError("Error 126: The specified module could not be found");
            LogError("This usually means a dependency DLL is missing");
            LogError("Required: leechcore.dll, vcruntime140.dll, and VC++ Redistributable");
        } else if (error == 193) {
            LogError("Error 193: Not a valid Win32 application");
            LogError("Make sure you're using the x64 version");
        }
        
        LogError("Make sure vmm.dll is in the same directory as this executable");
        return false;
    }

    // Load all function pointers
    pVMMDLL_Initialize = (VMMDLL_Initialize_t)GetProcAddress(hVmmDll, "VMMDLL_Initialize");
    pVMMDLL_Close = (VMMDLL_Close_t)GetProcAddress(hVmmDll, "VMMDLL_Close");
    pVMMDLL_PidList = (VMMDLL_PidList_t)GetProcAddress(hVmmDll, "VMMDLL_PidList");
    pVMMDLL_ProcessGetInformation = (VMMDLL_ProcessGetInformation_t)GetProcAddress(hVmmDll, "VMMDLL_ProcessGetInformation");
    pVMMDLL_Map_GetModuleU = (VMMDLL_Map_GetModuleU_t)GetProcAddress(hVmmDll, "VMMDLL_Map_GetModuleU");
    pVMMDLL_MemReadEx = (VMMDLL_MemReadEx_t)GetProcAddress(hVmmDll, "VMMDLL_MemReadEx");
    pVMMDLL_MemFree = (VMMDLL_MemFree_t)GetProcAddress(hVmmDll, "VMMDLL_MemFree");

    if (!pVMMDLL_Initialize || !pVMMDLL_Close || !pVMMDLL_PidList || 
        !pVMMDLL_ProcessGetInformation || !pVMMDLL_Map_GetModuleU || 
        !pVMMDLL_MemReadEx || !pVMMDLL_MemFree) {
        LogError("Failed to load required functions from vmm.dll");
        return false;
    }

    Log("[SUCCESS] vmm.dll loaded successfully");
    return true;
}

bool InitializeDMA() {
    Log("\n[1/4] Initializing DMA connection...");
    
    LPCSTR args[] = { "", "-device", "fpga" };
    hVMM = pVMMDLL_Initialize(3, args);
    
    if (!hVMM) {
        LogError("Failed to initialize DMA");
        LogError("Make sure:");
        LogError("  1. DMA card is properly connected");
        LogError("  2. Drivers are installed");
        LogError("  3. Gaming PC is powered on");
        return false;
    }
    
    Log("[SUCCESS] DMA initialized");
    return true;
}

bool FindProcess() {
    Log("\n[2/4] Finding Arc Raiders process...");
    
    SIZE_T cPids = 0;
    if (!pVMMDLL_PidList(hVMM, nullptr, &cPids) || cPids == 0) {
        LogError("Failed to enumerate processes");
        return false;
    }
    
    Log("    Found " + std::to_string(cPids) + " processes");
    
    std::vector<DWORD> pids(cPids);
    if (!pVMMDLL_PidList(hVMM, pids.data(), &cPids)) {
        LogError("Failed to get process list");
        return false;
    }
    
    Log("    Scanning for Arc Raiders...");
    
    // Process info structure (simplified)
    struct VMMDLL_PROCESS_INFORMATION {
        DWORD magic;
        WORD wVersion;
        WORD wSize;
        DWORD tpMemoryModel;
        DWORD tpSystem;
        BOOL fUserOnly;
        DWORD dwPID;
        DWORD dwPPID;
        DWORD dwState;
        CHAR szName[16];
        CHAR szNameLong[64];
        QWORD paDTB;
        QWORD paDTB_UserOpt;
    };
    
    const QWORD VMMDLL_PROCESS_INFORMATION_MAGIC = 0xc0ffee663df9301eULL;
    const WORD VMMDLL_PROCESS_INFORMATION_VERSION = 7;
    
    std::vector<std::string> gameProcesses;
    
    for (DWORD pid : pids) {
        VMMDLL_PROCESS_INFORMATION procInfo = { 0 };
        procInfo.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        procInfo.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        SIZE_T cbProcInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        
        if (pVMMDLL_ProcessGetInformation(hVMM, pid, &procInfo, &cbProcInfo)) {
            std::string procName = procInfo.szName;
            
            // Look for Arc Raiders related processes
            if (procName.find("Pioneer") != std::string::npos ||
                procName.find("ArcRaiders") != std::string::npos ||
                procName.find("Embark") != std::string::npos) {
                gameProcesses.push_back(procName + " (PID: " + std::to_string(pid) + ")");
            }
            
            // Check for exact match first
            if (procName.find("PioneerGame") != std::string::npos) {
                processId = pid;
                Log("[SUCCESS] Found: " + procName + " (PID: " + std::to_string(processId) + ")");
                return true;
            }
        }
    }
    
    // If not found, show potential matches
    if (!gameProcesses.empty()) {
        Log("\n[INFO] Found potential Arc Raiders processes:");
        for (const auto& proc : gameProcesses) {
            Log("    " + proc);
        }
        Log("\n[WARNING] None matched 'PioneerGame' - process name may have changed");
        Log("[INFO] Please check Task Manager on Gaming PC for exact process name");
    } else {
        LogError("Arc Raiders process not found via name search");
        Log("\n[INFO] Searched " + std::to_string(cPids) + " processes");
        Log("[INFO] ProcessGetInformation may have failed for all processes");
        
        // Try direct PID 1960 as fallback
        Log("\n[INFO] Attempting fallback: Trying PID 1960 directly...");
        
        // Check if PID 1960 exists in the list
        bool pidExists = std::find(pids.begin(), pids.end(), 1960) != pids.end();
        
        if (pidExists) {
            Log("[SUCCESS] PID 1960 found in process list!");
            Log("[INFO] Assuming this is PioneerGame.exe");
            processId = 1960;
            return true;
        } else {
            Log("[WARNING] PID 1960 not found in process list");
            LogError("Make sure the game is running on the Gaming PC");
        }
    }
    
    return false;
}

bool GetModuleBase() {
    Log("\n[3/4] Getting module base address...");
    
    // Module map structure (correct for v5.16+)
    struct VMMDLL_MAP_MODULEENTRY {
        QWORD vaBase;
        QWORD vaEntry;
        DWORD cbImageSize;
        BOOL fWoW64;
        LPSTR uszText;          // Pointer to module name
        DWORD _Reserved3;
        DWORD _Reserved4;
        LPSTR uszFullName;      // Pointer to full path
        DWORD tp;
        DWORD cbFileSizeRaw;
        DWORD cSection;
        DWORD cEAT;
        DWORD cIAT;
        DWORD _Reserved2;
        QWORD _Reserved1[3];
        PVOID pExDebugInfo;
        PVOID pExVersionInfo;
    };
    
    struct VMMDLL_MAP_MODULE {
        DWORD dwVersion;
        DWORD _Reserved1[5];
        QWORD pbMultiText;
        DWORD cbMultiText;
        DWORD cMap;
        VMMDLL_MAP_MODULEENTRY pMap[1];
    };
    
    PVOID pModuleMap = nullptr;
    if (!pVMMDLL_Map_GetModuleU(hVMM, processId, &pModuleMap, 0)) {
        LogError("Failed to get module map");
        return false;
    }
    
    VMMDLL_MAP_MODULE* pMap = (VMMDLL_MAP_MODULE*)pModuleMap;
    Log("    Found " + std::to_string(pMap->cMap) + " modules");
    
    // List first few modules for debugging
    Log("    Listing modules:");
    for (DWORD i = 0; i < std::min((DWORD)10, pMap->cMap); i++) {
        std::string modName = pMap->pMap[i].uszText;
        Log("      [" + std::to_string(i) + "] " + modName);
    }
    
    // Search for game module
    for (DWORD i = 0; i < pMap->cMap; i++) {
        std::string modName = pMap->pMap[i].uszText;
        
        // Try multiple patterns
        if (modName.find("PioneerGame") != std::string::npos ||
            modName.find("ArcRaiders") != std::string::npos ||
            (i == 0 && modName.find(".exe") != std::string::npos)) {  // First .exe is usually the main module
            
            moduleBase = pMap->pMap[i].vaBase;
            Log("\n[SUCCESS] Found module: " + modName);
            Log("[SUCCESS] Module base: 0x" + std::to_string(moduleBase));
            pVMMDLL_MemFree(pModuleMap);
            return true;
        }
    }
    
    pVMMDLL_MemFree(pModuleMap);
    LogError("Module not found");
    Log("[INFO] Searched " + std::to_string(pMap->cMap) + " modules");
    return false;
}

uint64_t ReadUInt64(uint64_t address) {
    uint64_t value = 0;
    pVMMDLL_MemReadEx(hVMM, processId, address, (PBYTE)&value, sizeof(uint64_t), nullptr, 0x0001);
    return value;
}

uint32_t ReadUInt32(uint64_t address) {
    uint32_t value = 0;
    pVMMDLL_MemReadEx(hVMM, processId, address, (PBYTE)&value, sizeof(uint32_t), nullptr, 0x0001);
    return value;
}

bool IsValidPointer(uint64_t ptr) {
    if (ptr < 0x10000 || ptr > 0x7FFFFFFFFFFF) {
        return false;
    }
    
    uint64_t test = 0;
    DWORD bytesRead = 0;
    return pVMMDLL_MemReadEx(hVMM, processId, ptr, (PBYTE)&test, sizeof(uint64_t), &bytesRead, 0x0001) && bytesRead == sizeof(uint64_t);
}

int ValidateUWorldStructure(uint64_t uWorldPtr) {
    int score = 0;
    
    if (!IsValidPointer(uWorldPtr)) return 0;
    score += 10;
    
    uint64_t persistentLevel = ReadUInt64(uWorldPtr + UWORLD_PERSISTENTLEVEL);
    if (IsValidPointer(persistentLevel)) {
        score += 20;
        
        uint64_t actorsArray = ReadUInt64(persistentLevel + ULEVEL_ACTORS);
        uint32_t actorCount = ReadUInt32(persistentLevel + ULEVEL_ACTORCOUNT);
        
        if (IsValidPointer(actorsArray) && actorCount > 0 && actorCount < 100000) {
            score += 25;
            
            int validActors = 0;
            for (int i = 0; i < std::min((int)actorCount, 5); i++) {
                uint64_t actor = ReadUInt64(actorsArray + (i * 8));
                if (IsValidPointer(actor)) validActors++;
            }
            score += validActors * 5;
        }
    }
    
    if (IsValidPointer(ReadUInt64(uWorldPtr + UWORLD_OWNINGGAMEINSTANCE))) score += 15;
    if (IsValidPointer(ReadUInt64(uWorldPtr + UWORLD_LEVELS))) score += 10;
    if (IsValidPointer(ReadUInt64(ReadUInt64(uWorldPtr + UWORLD_LEVELS)))) score += 10;
    if (IsValidPointer(ReadUInt64(uWorldPtr + UWORLD_GAMESTATE))) score += 10;
    
    return score;
}

std::vector<GWorldCandidate> ScanForGWorld() {
    Log("\n[4/4] Scanning for GWorld pointer...");
    Log("    Range: ±" + std::to_string(SCAN_RANGE_MB) + "MB around GNames");
    Log("    Estimated time: 2-3 minutes\n");
    
    std::vector<GWorldCandidate> candidates;
    
    uint64_t gnamesAddress = moduleBase + GNAMES_OFFSET;
    uint64_t scanStart = gnamesAddress - (SCAN_RANGE_MB * 1024 * 1024);
    uint64_t scanEnd = gnamesAddress + (SCAN_RANGE_MB * 1024 * 1024);
    
    scanStart = (scanStart / 0x1000) * 0x1000;
    scanEnd = (scanEnd / 0x1000) * 0x1000;
    
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    uint64_t totalSize = scanEnd - scanStart;
    int lastProgress = -1;
    
    for (uint64_t addr = scanStart; addr < scanEnd; addr += CHUNK_SIZE) {
        int progress = (int)(((addr - scanStart) * 100) / totalSize);
        if (progress != lastProgress && progress % 5 == 0) {
            std::cout << "\r    Progress: " << progress << "%" << std::flush;
            lastProgress = progress;
        }
        
        DWORD bytesRead = 0;
        if (!pVMMDLL_MemReadEx(hVMM, processId, addr, buffer.data(), CHUNK_SIZE, &bytesRead, 0x0001)) {
            continue;
        }
        
        for (size_t i = 0; i < bytesRead - 8; i += 8) {
            uint64_t potentialPtr = *(uint64_t*)(buffer.data() + i);
            
            if (potentialPtr < 0x10000 || potentialPtr > 0x7FFFFFFFFFFF) continue;
            
            int score = ValidateUWorldStructure(potentialPtr);
            
            if (score >= MIN_CONFIDENCE) {
                GWorldCandidate candidate;
                candidate.offset = (addr + i) - moduleBase;
                candidate.address = addr + i;
                candidate.score = score;
                candidate.uWorldPtr = potentialPtr;
                
                uint64_t persistentLevel = ReadUInt64(potentialPtr + UWORLD_PERSISTENTLEVEL);
                candidate.actorCount = IsValidPointer(persistentLevel) ? ReadUInt32(persistentLevel + ULEVEL_ACTORCOUNT) : 0;
                
                candidates.push_back(candidate);
            }
        }
    }
    
    std::cout << "\r    Progress: 100%     " << std::endl;
    
    std::sort(candidates.begin(), candidates.end(), 
        [](const GWorldCandidate& a, const GWorldCandidate& b) {
            return a.score > b.score;
        });
    
    return candidates;
}

void DisplayResults(const std::vector<GWorldCandidate>& candidates) {
    std::cout << "\n===========================================" << std::endl;
    std::cout << "  SCAN RESULTS" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    if (candidates.empty()) {
        Log("\n[WARNING] No GWorld candidates found");
        Log("\nPossible reasons:");
        Log("  - Game is in menu/lobby (not in match)");
        Log("  - Structures haven't initialized yet");
        Log("  - Try waiting longer in match");
        Log("  - Move around in game to load actors");
        return;
    }
    
    Log("\n[SUCCESS] Found " + std::to_string(candidates.size()) + " candidate(s)!\n");
    
    int displayCount = std::min((int)candidates.size(), 5);
    for (int i = 0; i < displayCount; i++) {
        std::cout << "Candidate #" << (i + 1) << ":" << std::endl;
        std::cout << "  Offset:     0x" << std::hex << std::uppercase << candidates[i].offset << std::endl;
        std::cout << "  Address:    0x" << std::hex << std::uppercase << candidates[i].address << std::endl;
        std::cout << "  Confidence: " << std::dec << candidates[i].score << "/100" << std::endl;
        std::cout << "  UWorld Ptr: 0x" << std::hex << std::uppercase << candidates[i].uWorldPtr << std::endl;
        std::cout << "  Actors:     " << std::dec << candidates[i].actorCount << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "\n[RECOMMENDED] Use Candidate #1 (highest confidence)" << std::endl;
    std::cout << "GWorld Offset: 0x" << std::hex << std::uppercase << candidates[0].offset << std::endl;
    std::cout << "\nAdd this to your cheat:" << std::endl;
    std::cout << "#define GWORLD_OFFSET 0x" << std::hex << std::uppercase << candidates[0].offset << std::endl;
}

void Cleanup() {
    if (hVMM && pVMMDLL_Close) {
        pVMMDLL_Close(hVMM);
    }
    if (hVmmDll) {
        FreeLibrary(hVmmDll);
    }
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  Arc Raiders GWorld Scanner v2.0" << std::endl;
    std::cout << "  Automated Offset Finder" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    if (!LoadVmmDll()) {
        system("pause");
        return 1;
    }
    
    if (!InitializeDMA()) {
        Cleanup();
        system("pause");
        return 1;
    }
    
    if (!FindProcess()) {
        Cleanup();
        system("pause");
        return 1;
    }
    
    if (!GetModuleBase()) {
        Cleanup();
        system("pause");
        return 1;
    }
    
    auto candidates = ScanForGWorld();
    DisplayResults(candidates);
    
    std::cout << "\n===========================================" << std::endl;
    Cleanup();
    system("pause");
    
    return 0;
}
