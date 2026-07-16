#!/usr/bin/env bash

set -euo pipefail

if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y build-essential pkg-config libsdl2-dev
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y gcc-c++ make pkgconf-pkg-config SDL2-devel
elif command -v pacman >/dev/null 2>&1; then
    sudo pacman -S --needed base-devel pkgconf sdl2
else
    echo 'Unsupported package manager. Install a C++ compiler, make, pkg-config, and SDL2 development files.' >&2
    exit 1
fi

pkg-config --modversion sdl2
echo 'SDL2 development dependencies are ready.'
