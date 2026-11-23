#!/bin/bash
# Cross-compiles the emulator for 64-bit Windows and packs it into dist/TK2000-<version>-windows-x64.zip
# (tk2000.exe, SDL2.dll and the frame picture: extract anywhere and double-click tk2000.exe)
#
# Requirements: MinGW-w64, CMake, python3 with Pillow (for the icon), curl (to download SDL2 the first time)
#   sudo apt install g++-mingw-w64-x86-64 cmake python3-pil curl zip
# To test it on Linux: wine dist/TK2000-<version>-windows-x64/tk2000.exe
#
# Usage: packaging/windows/make-zip.sh [version]

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${1:-1.0}"
SDL_VERSION="2.32.10"
BUILD="$ROOT/build-windows"
DEPS="$BUILD/deps"
SDL_DIR="$DEPS/SDL2-$SDL_VERSION/x86_64-w64-mingw32"
DIST="$ROOT/dist"
NAME="TK2000-$VERSION-windows-x64"
STAGE="$DIST/$NAME"

# 1) SDL2 development package for MinGW (headers, import library and SDL2.dll)
if [ ! -f "$SDL_DIR/bin/SDL2.dll" ]; then
	mkdir -p "$DEPS"
	curl -fL -o "$DEPS/SDL2-mingw.tar.gz" \
		"https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-devel-$SDL_VERSION-mingw.tar.gz"
	tar -xzf "$DEPS/SDL2-mingw.tar.gz" -C "$DEPS"
	rm "$DEPS/SDL2-mingw.tar.gz"
fi

# 2) Icon from the computer picture (square crop of the center), embedded in tk2000.exe
ICON="$BUILD/tk2000.ico"
mkdir -p "$BUILD"
python3 - "$ROOT/res/tk2000-frame.png" "$ICON" <<'EOF'
import sys
from PIL import Image
img = Image.open(sys.argv[1]).convert("RGBA")
w, h = img.size
s = min(w, h)
img = img.crop(((w - s) // 2, (h - s) // 2, (w + s) // 2, (h + s) // 2)).resize((256, 256), Image.LANCZOS)
img.save(sys.argv[2], sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
EOF

# 3) Release build
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_TOOLCHAIN_FILE="$ROOT/packaging/windows/mingw-w64-x86_64.cmake" \
	-DTK2000_SDL2_DIR="$SDL_DIR" -DTK2000_ICON="$ICON"
cmake --build "$BUILD" -j
x86_64-w64-mingw32-strip "$BUILD/src/tk2000.exe"

# 4) Folder and zip
rm -rf "$STAGE" "$DIST/$NAME.zip"
mkdir -p "$STAGE"
cp "$BUILD/src/tk2000.exe" "$SDL_DIR/bin/SDL2.dll" "$ROOT/res/tk2000-frame.png" "$STAGE/"
cp "$ROOT/LICENSE" "$STAGE/LICENSE.txt"
(cd "$DIST" && zip -qr "$NAME.zip" "$NAME")

echo "Created $DIST/$NAME.zip (SDL $SDL_VERSION)"
