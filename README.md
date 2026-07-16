# Appeal_SDL2_Input

An Appeal plugin project.

## Development status

This repository builds the SDL2 input backend as an Appeal plugin shared library.

## Developer dependencies

Third-party libraries are not stored in this repository.

- Windows: run `powershell -ExecutionPolicy Bypass -File Scripts/Setup-Dependencies.ps1`. It downloads a pinned MinGW SDL2 development archive, verifies its SHA-256 checksum, and installs it under the Git-ignored `.deps` directory.
- Debian, Fedora, or Arch Linux: run `bash Scripts/setup-dependencies.sh`. It installs the distribution's SDL2 development package.

Dependency versions, official URLs, checksums, and package names are recorded in `dependencies.json`.

## Distribution

The Windows installer downloads and verifies the SDL2 runtime during installation. The Debian package declares SDL2 as a runtime dependency so the system package manager downloads it.

Generated binaries and intermediate objects are intentionally excluded from version control.
