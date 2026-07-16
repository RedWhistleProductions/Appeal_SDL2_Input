#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="$(tr -d '\r\n' < "$root/VERSION")"
deb_version="${version/-beta./~beta}"
package="appeal-plugin-sdl2-input"
stage="${TMPDIR:-/tmp}/${package}_${deb_version}_amd64"
output="$root/Dist/${package}_${deb_version}_amd64.deb"
make -C "$root" clean all
rm -rf "$stage"
mkdir -p "$stage/DEBIAN" "$stage/usr/share/appeal/Resources/Modules/Input/Appeal_SDL2_Input"
cp "$root/Appeal_SDL2_Input.so" "$stage/usr/share/appeal/Resources/Modules/Input/Appeal_SDL2_Input/"
cat > "$stage/DEBIAN/control" <<CONTROL
Package: $package
Version: $deb_version
Section: games
Priority: optional
Architecture: amd64
Maintainer: Red Whistle
Depends: appeal, libsdl2-2.0-0
Description: SDL2 input plugin for Appeal
 Provides keyboard, mouse, and controller input through SDL2.
CONTROL
find "$stage" -type d -exec chmod 755 {} +
find "$stage" -type f -exec chmod 644 {} +
mkdir -p "$root/Dist"
dpkg-deb --build --root-owner-group "$stage" "$output"
echo "Created $output"
