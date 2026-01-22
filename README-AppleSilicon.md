# Building Waldmeister on Apple Silicon (macOS)

This document describes how to build Waldmeister on Apple Silicon Macs (M1/M2/M3/M4).

## Prerequisites

1. **Xcode Command Line Tools** (provides clang compiler):
   ```bash
   xcode-select --install
   ```

2. **GNU Make** (should be available by default, or install via Homebrew):
   ```bash
   brew install make
   ```

## Building

1. Navigate to the Source directory:
   ```bash
   cd Source
   ```

2. Clean any previous build artifacts:
   ```bash
   make realclean
   ```

3. Build the standard version:
   ```bash
   make std
   ```

   Or build all versions:
   ```bash
   make all
   ```

## Build Targets

- `make std` - Standard build with debugging symbols
- `make fast` - Optimized build
- `make prof` - Profiling build (limited functionality on Apple Silicon)
- `make power` - Power user build (optimized)
- `make comp` - Competition build (optimized)

## Output

The compiled binaries will be placed in `bin/`:
- `bin/WaldmeisterII.std` - Standard build
- `bin/WaldmeisterII.fast` - Optimized build
- etc.

## Notes

### Static Linking
macOS does not support fully static linking. The binaries will be dynamically linked against system libraries.

### Profiling
The `-pg` profiling flag has limited support on Apple Silicon. For performance analysis, consider using Instruments.app instead.

### High-Resolution Timing
The x86-specific `rdtsc` instruction has been replaced with `clock_gettime(CLOCK_MONOTONIC)` for ARM64 compatibility. This provides nanosecond-resolution timing.

### Performance Counters
Hardware performance counters (ANALYZE == 3) are not fully supported on Apple Silicon in the same way as x86. The code will compile but counter functions will return 0.

## Troubleshooting

### Compiler Warnings
Some warnings have been suppressed for compatibility with the older codebase:
- `-Wno-deprecated-non-prototype`
- `-Wno-implicit-function-declaration`
- `-Wno-int-conversion`
- `-Wno-format`

### Architecture Detection
If the build fails to detect the architecture, check `.architecture` file. It should contain:
```
ARCH=DARWIN
```

If not, run:
```bash
./Scripts/findarch
```

## Changes Made for Apple Silicon Port

1. **Scripts/findarch**: Added Darwin detection
2. **lib/Makerules**: Added DARWIN-specific compiler flags
3. **include/antiwarnings.h**: Added Darwin signal handler typedef
4. **include/compiler-extra.h**: Added clang detection
5. **sources/INF/SuffSolver3.c**: ARM-compatible timing code
6. **sources/INF/Grundzusammenfuehrung.c**: ARM-compatible timing code
7. **sources/ORD/Ordnungen.c**: ARM-compatible timing code
