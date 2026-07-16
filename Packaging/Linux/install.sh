#!/usr/bin/env bash
set -euo pipefail
package_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
artifact="$package_dir/Appeal_SDL2_Input.so"
destination="${APPEAL_RESOURCES:-$HOME/Documents/Appeal/Resources}/Modules/Input/Appeal_SDL2_Input"
if [[ ! -f "$artifact" ]]; then echo "Missing release artifact: $artifact" >&2; exit 1; fi
if command -v apt-get >/dev/null 2>&1 && [[ -n "libsdl2-2.0-0" ]]; then sudo apt-get update; sudo apt-get install -y libsdl2-2.0-0; fi
mkdir -p "$destination"
cp "$artifact" "$destination/"
echo "Installed Appeal_SDL2_Input in $destination"