# Building ArpSequencer

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| CMake | ≥ 3.22 | https://cmake.org |
| C++ compiler | C++17 | MSVC 2022, GCC 11+, Clang 13+ |
| Git | any | to fetch JUCE via FetchContent |
| Ninja (opt.) | any | faster builds |

JUCE 7.0.9 is fetched automatically via `FetchContent` — no manual clone needed.

---

## Quick Build (all platforms)

```bash
# 1. Clone repo
git clone https://github.com/Mxnny88/mannyzeneke.git
cd mannyzeneke

# 2. Configure (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release --parallel

# 4. (Optional) Run unit tests
cmake --build build --target ArpSequencerTests
./build/ArpSequencerTests
```

Plugin artefacts are output to:
```
build/ArpSequencer_artefacts/Release/
    VST3/ArpSequencer.vst3
    AU/ArpSequencer.component          (macOS only)
    Standalone/ArpSequencer[.exe/.app]
```

---

## Windows

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Then run the Inno Setup script (`Installer/windows_install.iss`) to create
a self-contained installer EXE.

---

## macOS

```bash
cmake -B build -G Xcode
cmake --build build --config Release

# Install (requires admin)
cd Installer
./mac_install.sh --all
```

---

## Linux (VST3 only)

```bash
sudo apt install cmake ninja-build libx11-dev libxinerama-dev \
    libxext-dev libfreetype6-dev libwebkit2gtk-4.0-dev \
    libglu1-mesa-dev libasound2-dev

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Copy manually
sudo cp -r build/ArpSequencer_artefacts/Release/VST3/ArpSequencer.vst3 \
    ~/.vst3/
```

---

## Build Options

| CMake flag | Default | Description |
|------------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build unit test executable |
| `CMAKE_BUILD_TYPE` | `Debug` | Use `Release` for distribution |

---

## Supported Sample Rates & Bit Depths

- 44 100 Hz – 192 000 Hz
- 32-bit float (JUCE standard)
- 32-bit and 64-bit host compatibility (VST3 handles both)
