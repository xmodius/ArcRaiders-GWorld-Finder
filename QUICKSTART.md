# 🚀 Quick Start Guide

## One-Command Setup

```cmd
cd C:\ && git clone https://github.com/xmodius/ArcRaiders-GWorld-Finder.git && cd ArcRaiders-GWorld-Finder && setup.bat && build.bat
```

---

## Step-by-Step

### 1. Clone Repository
```cmd
cd C:\
git clone https://github.com/xmodius/ArcRaiders-GWorld-Finder.git
cd ArcRaiders-GWorld-Finder
```

### 2. Download MemProcFS
```cmd
setup.bat
```
*Downloads ~5.35 MB*

### 3. Build Scanner
```cmd
build.bat
```
*Requires Visual Studio Developer Command Prompt*

### 4. Run Scanner
**Before running:**
- ✅ Arc Raiders running on Gaming PC
- ✅ In an active match (not menu)
- ✅ Wait 15-20 seconds after spawning

```cmd
GWorldScanner.exe
```

---

## Expected Output

```
===========================================
  Arc Raiders GWorld Scanner v2.0
===========================================

[SUCCESS] Found 3 candidate(s)!

Candidate #1:
  Offset:     0x7E9A5C0
  Confidence: 85/100
  
[RECOMMENDED] Use Candidate #1
GWorld Offset: 0x7E9A5C0
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No candidates found | Must be in active match, wait longer |
| DMA init failed | Check hardware connection |
| Process not found | Launch Arc Raiders on Gaming PC |
| vmm.dll not found | Run `setup.bat` first |

---

## Need Help?

📖 **Full Documentation:** [README.md](README.md)

🐛 **Report Issues:** [GitHub Issues](https://github.com/xmodius/ArcRaiders-GWorld-Finder/issues)
