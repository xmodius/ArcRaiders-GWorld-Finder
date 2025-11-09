# Arc Raiders GWorld Scanner v2.0

**Production-grade automated GWorld offset finder for Arc Raiders using DMA hardware.**

---

## 🎯 Overview

This tool automatically scans memory to find the **GWorld offset** for Arc Raiders by validating Unreal Engine 5 structure patterns. It eliminates the need for manual reverse engineering with tools like ReClass.NET.

### Key Features

- ✅ **Fully automated** - No manual memory searching required
- ✅ **Structure validation** - Scores candidates based on UE5 patterns
- ✅ **Production-ready** - Robust error handling and logging
- ✅ **Self-contained** - Dynamic DLL loading, no header dependencies
- ✅ **Clean architecture** - Optimized C++ with proper resource management

---

## 📋 Requirements

### Hardware Setup

**2-PC DMA Configuration:**
- **Gaming PC**: Running Arc Raiders
- **Cheat PC**: Running this scanner
- **DMA Card**: ScreamerM2, ZDMA, or compatible FPGA device
- **Connection**: PCIe/USB between Gaming PC and DMA card

### Software Requirements

**On Cheat PC:**
- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++ tools
- Git for Windows
- MemProcFS v5.16+ (auto-downloaded by setup script)

### Game State Requirements

**Critical for successful scanning:**
- Arc Raiders **must be running** on Gaming PC
- **Must be in an active match** (not menu, lobby, or loading screen)
- Wait **15-20 seconds after spawning** for structures to initialize
- Move around in game to ensure actors are loaded

---

## 🚀 Quick Start

### Step 1: Clone Repository

```cmd
cd C:\
git clone https://github.com/xmodius/ArcRaiders-GWorld-Finder.git
cd ArcRaiders-GWorld-Finder
```

### Step 2: Download MemProcFS

```cmd
setup.bat
```

This downloads and extracts MemProcFS v5.16.6 (~5.35 MB) with all required DLLs.

### Step 3: Build Scanner

```cmd
build.bat
```

Compiles `GWorldScanner.cpp` into `GWorldScanner.exe`.

### Step 4: Run Scanner

**Before running:**
1. Launch Arc Raiders on Gaming PC
2. Join an active match
3. Wait 15-20 seconds after spawning

```cmd
GWorldScanner.exe
```

**Scan duration:** 2-3 minutes

---

## 📊 How It Works

### Scanning Process

1. **Load vmm.dll** - Dynamically loads MemProcFS library
2. **Initialize DMA** - Connects to DMA hardware
3. **Find Process** - Locates `PioneerGame.exe` on Gaming PC
4. **Get Module Base** - Retrieves game executable base address
5. **Scan Memory** - Searches ±100MB around known GNames offset
6. **Validate Structures** - Checks each pointer for valid UE5 patterns
7. **Score Candidates** - Assigns confidence scores (0-100)
8. **Display Results** - Shows top 5 candidates sorted by confidence

### Structure Validation

Each candidate is scored based on:

| Check | Points | Description |
|-------|--------|-------------|
| Valid pointer | 10 | Pointer in valid memory range |
| PersistentLevel | 20 | Valid ULevel pointer |
| Actors array | 25 | Valid array with reasonable count |
| Actor validation | 5 each | First 5 actors are valid (max 25) |
| GameInstance | 15 | Valid UGameInstance pointer |
| Levels array | 10 | Valid TArray pointer |
| First level | 10 | First level in array is valid |
| GameState | 10 | Valid AGameStateBase pointer |

**Maximum score:** 100 points

### Confidence Levels

- **80-100**: Very high confidence (almost certainly correct)
- **60-79**: High confidence (likely correct)
- **50-59**: Medium confidence (needs verification)
- **0-49**: Low confidence (likely false positive)

---

## 📖 Usage Guide

### Running the Scanner

```cmd
GWorldScanner.exe
```

### Example Output

```
===========================================
  Arc Raiders GWorld Scanner v2.0
  Automated Offset Finder
===========================================

[*] Loading vmm.dll...
[SUCCESS] vmm.dll loaded successfully

[1/4] Initializing DMA connection...
[SUCCESS] DMA initialized

[2/4] Finding Arc Raiders process...
[SUCCESS] Found: PioneerGame.exe (PID: 12345)

[3/4] Getting module base address...
[SUCCESS] Module base: 0x7FF76D960000

[4/4] Scanning for GWorld pointer...
    Range: ±100MB around GNames
    Estimated time: 2-3 minutes

    Progress: 100%

===========================================
  SCAN RESULTS
===========================================

[SUCCESS] Found 3 candidate(s)!

Candidate #1:
  Offset:     0x7E9A5C0
  Address:    0x7FF76E2FA5C0
  Confidence: 85/100
  UWorld Ptr: 0x1A2B3C4D5E6F
  Actors:     1247

Candidate #2:
  Offset:     0x7E9B120
  Address:    0x7FF76E2FB120
  Confidence: 72/100
  UWorld Ptr: 0x1A2B3C4D6000
  Actors:     1189

[RECOMMENDED] Use Candidate #1 (highest confidence)
GWorld Offset: 0x7E9A5C0

Add this to your cheat:
#define GWORLD_OFFSET 0x7E9A5C0

===========================================
```

### Using the Results

Add the offset to your DMA cheat:

```cpp
#define GWORLD_OFFSET 0x7E9A5C0  // From scanner output

uint64_t gworld_addr = module_base + GWORLD_OFFSET;
uint64_t gworld = ReadUInt64(gworld_addr);
uint64_t persistent_level = ReadUInt64(gworld + 0x38);
uint64_t actors_array = ReadUInt64(persistent_level + 0xA0);
uint32_t actor_count = ReadUInt32(persistent_level + 0xA8);
```

---

## 🔧 Troubleshooting

### No Candidates Found

**Symptoms:**
```
[WARNING] No GWorld candidates found
```

**Solutions:**
1. **Verify game state**: Must be in active match, not menu
2. **Wait longer**: Give structures 30+ seconds to initialize
3. **Move around**: Walk/run to trigger actor loading
4. **Check GNames**: Verify GNames offset is still correct
5. **Restart game**: Fresh launch may help

### DMA Initialization Failed

**Symptoms:**
```
[ERROR] Failed to initialize DMA
```

**Solutions:**
1. **Check hardware**: Ensure DMA card is properly connected
2. **Verify drivers**: Install/update DMA card drivers
3. **Power cycle**: Restart both PCs
4. **Test connection**: Use DMA vendor's test tool
5. **Check device**: Try `-device fpga://algo=0` parameter

### Process Not Found

**Symptoms:**
```
[ERROR] Arc Raiders process not found
```

**Solutions:**
1. **Launch game**: Start Arc Raiders on Gaming PC
2. **Wait for load**: Let game fully load before scanning
3. **Check process name**: Verify it's still `PioneerGame.exe`
4. **DMA connection**: Ensure DMA can see Gaming PC processes

### vmm.dll Not Found

**Symptoms:**
```
[ERROR] Failed to load vmm.dll
```

**Solutions:**
1. **Run setup.bat**: Download MemProcFS files
2. **Check directory**: Ensure vmm.dll is in same folder as exe
3. **Manual download**: Get from [MemProcFS Releases](https://github.com/ufrisk/MemProcFS/releases)
4. **Antivirus**: Check if AV quarantined the DLL

### Low Confidence Scores

**If all candidates score below 60:**
1. Wait longer in match (30-60 seconds)
2. Try different game mode/map
3. Verify GNames offset is correct
4. Lower `MIN_CONFIDENCE` threshold in code
5. Use ReClass.NET for manual validation

---

## 🏗️ Building from Source

### Prerequisites

- Visual Studio 2022 with "Desktop development with C++"
- Windows SDK 10.0.22621.0 or later

### Build Steps

1. **Open Developer Command Prompt for VS 2022**
2. **Navigate to project directory**
   ```cmd
   cd C:\ArcRaiders-GWorld-Finder
   ```
3. **Run build script**
   ```cmd
   build.bat
   ```

### Manual Build

```cmd
cl.exe /EHsc /std:c++17 /O2 /W4 GWorldScanner.cpp /Fe:GWorldScanner.exe
```

---

## 📁 Project Structure

```
ArcRaiders-GWorld-Finder/
├── GWorldScanner.cpp       # Main scanner source code
├── build.bat               # Build script
├── setup.bat               # MemProcFS download script
├── README.md               # This file
├── LICENSE                 # MIT License
├── .gitignore              # Git ignore rules
│
├── vmm.dll                 # MemProcFS library (downloaded)
├── leechcore.dll           # LeechCore library (downloaded)
├── FTD3XX.dll              # FTDI driver (downloaded)
└── GWorldScanner.exe       # Compiled scanner (after build)
```

---

## 🔬 Technical Details

### Known Offsets

From comprehensive dumper output:

```cpp
#define GNAMES_OFFSET 0x7E97580

// UWorld structure
#define UWORLD_PERSISTENTLEVEL    0x38
#define UWORLD_OWNINGGAMEINSTANCE 0x1A0
#define UWORLD_LEVELS             0x178
#define UWORLD_GAMESTATE          0x158

// ULevel structure
#define ULEVEL_ACTORS      0xA0
#define ULEVEL_ACTORCOUNT  0xA8
```

### Scan Parameters

```cpp
#define SCAN_RANGE_MB 100           // ±100MB around GNames
#define CHUNK_SIZE (2 * 1024 * 1024) // 2MB chunks
#define MIN_CONFIDENCE 50            // Minimum score threshold
```

### Memory Alignment

- Scans on **8-byte boundaries** (pointer alignment)
- Chunks aligned to **4KB pages**
- Validates pointer ranges: `0x10000` to `0x7FFFFFFFFFFF`

---

## 🛡️ Security & Ethics

### Legal Notice

This tool is for **educational purposes only**. Use of this software to gain unfair advantages in online games may:
- Violate game Terms of Service
- Result in permanent account bans
- Be illegal in your jurisdiction

**Use at your own risk.** The authors are not responsible for any consequences.

### Ethical Use

- **Private matches only**: Do not use in public matchmaking
- **Research purposes**: For learning reverse engineering techniques
- **Respect others**: Do not ruin the game experience for legitimate players

---

## 📝 Changelog

### Version 2.0 (Current)

- ✅ Complete rewrite with production-grade architecture
- ✅ Dynamic DLL loading (no header dependencies)
- ✅ Robust error handling and logging
- ✅ Optimized scanning algorithm
- ✅ Improved structure validation
- ✅ Clean build system
- ✅ Comprehensive documentation

### Version 1.0

- Initial release with basic scanning functionality

---

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Areas for Improvement

- [ ] GUI interface
- [ ] Multi-threaded scanning
- [ ] Pattern signature scanning
- [ ] Automatic offset updates
- [ ] Support for other UE5 games

---

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details.

---

## 🙏 Credits

- **MemProcFS** by Ulf Frisk - DMA memory access framework
- **Unreal Engine** reverse engineering community
- **DMA hardware** community for tools and knowledge

---

## 📞 Support

**Issues:** [GitHub Issues](https://github.com/xmodius/ArcRaiders-GWorld-Finder/issues)

**Discussions:** [GitHub Discussions](https://github.com/xmodius/ArcRaiders-GWorld-Finder/discussions)

---

## ⚠️ Disclaimer

This software is provided "as is" without warranty of any kind. The authors assume no liability for any damages or legal consequences resulting from the use of this software.

**Arc Raiders** is a trademark of Embark Studios. This project is not affiliated with or endorsed by Embark Studios.

---

*Last updated: November 2025*
