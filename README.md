# Waldmeister II (v7.10) — Apple Silicon Port

A theorem prover for first-order unit equational logic, based on unfailing Knuth-Bendix completion. Originally developed by A. Buch, Th. Hillenbrand, A. Jaeger, and B. Löchner at the University of Kaiserslautern.

See [the official Waldmeister page](http://www.waldmeister.org/) for papers and documentation.

## History

This is a port of Waldmeister II (v7.10) from Linux to **macOS/Darwin**, supporting both Apple Silicon (ARM64) and Intel (x86_64) Macs.

## Changes from upstream

- **Darwin architecture detection** — added `ARCH=DARWIN` support in `Scripts/findarch`
- **Clang compiler compatibility** — replaced `-ggdb` with `-g`, added warning suppressions for older C code patterns
- **ARM64 timing** — replaced x86 `rdtsc` with `clock_gettime(CLOCK_MONOTONIC)`
- **Removed `-static` linking** — not supported on macOS
- **Separate strip step** for optimized builds

See [README-macOS.md](README-macOS.md) for full porting details, troubleshooting, and port history.

## Build

### Prerequisites

- Xcode Command Line Tools (`xcode-select --install`)
- GNU Make (`brew install make` if needed)

### Building

```bash
make realclean    # Clean previous artifacts
make std          # Standard build (with debug symbols)
```

Other targets: `make fast`, `make comp`, `make power`, `make prof`, `make all`.

Binaries are placed in `bin/` (e.g., `bin/WaldmeisterII.std`).

## Usage

```bash
./bin/WaldmeisterII.std problem.wm
./bin/WaldmeisterII.std --help
./bin/WaldmeisterII.std --man
```

### Options

| Option | Description |
|--------|-------------|
| `--add` | Add-weight heuristic (measures term size) |
| `--mix` | Mix-weight heuristic (size + orientability, default) |
| `--auto` | Activate automated control component |
| `--tex` | Proof output in LaTeX notation |
| `--prolog` | Proof output in Prolog notation |
| `--pcl` | Proof output in PCL notation |
| `--ascii` | Proof output as plain text (default) |
| `--noproof` | Suppress proof presentation |
| `-o FILE` | Write output to FILE |
| `--db=FILE` | Employ database FILE |

## Original Authors

- **Arnim Buch**
- **Thomas Hillenbrand**
- **Alexander Jaeger**
- **Bernd Löchner**

Copyright (C) 1994-2002. University of Kaiserslautern.

## License

See the copyright notice in `sources/RUN/PowerMain.c` for the original terms. This port adds only the minimal changes required to compile and run on macOS/Darwin.
