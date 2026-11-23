#!/bin/bash
# Builds dist/TK2000-<version>-<arch>.AppImage: a single executable file with SDL bundled,
# which runs on most Linux distributions without installing anything
#
# Requirements: C++17 compiler, CMake, SDL2 development package, python3 with Pillow (for the icon),
# curl (to download linuxdeploy the first time)
#   sudo apt install g++ cmake libsdl2-dev python3-pil curl
# The AppImage runs on distributions with the same or newer glibc as the system used to build it,
# so build on the oldest distribution you want to support.
#
# Usage: packaging/linux/make-appimage.sh [version]

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${1:-1.0}"
ARCH="$(uname -m)"
BUILD="$ROOT/build-release"
TOOLS="$ROOT/build-release/tools"
DIST="$ROOT/dist"
APPDIR="$DIST/TK2000.AppDir"
OUTPUT="$DIST/TK2000-$VERSION-$ARCH.AppImage"

# 1) Release build
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j

# 2) linuxdeploy (copies the libraries into the AppDir and packs the AppImage)
LINUXDEPLOY="$TOOLS/linuxdeploy-$ARCH.AppImage"
if [ ! -x "$LINUXDEPLOY" ]; then
	mkdir -p "$TOOLS"
	curl -fL -o "$LINUXDEPLOY" \
		"https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
	chmod +x "$LINUXDEPLOY"
fi

# 3) AppDir layout (the frame picture goes next to the executable, where the emulator looks for it)
rm -rf "$APPDIR" "$OUTPUT"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/256x256/apps"
cp "$BUILD/src/tk2000" "$APPDIR/usr/bin/tk2000"
cp "$ROOT/res/tk2000-frame.png" "$APPDIR/usr/bin/"

# 4) Icon from the computer picture (square crop of the center)
python3 - "$ROOT/res/tk2000-frame.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/tk2000.png" <<'EOF'
import sys
from PIL import Image
img = Image.open(sys.argv[1]).convert("RGBA")
w, h = img.size
s = min(w, h)
img = img.crop(((w - s) // 2, (h - s) // 2, (w + s) // 2, (h + s) // 2))
img.resize((256, 256), Image.LANCZOS).save(sys.argv[2])
EOF

# 5) Menu entry
cat > "$APPDIR/usr/share/applications/tk2000.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=TK2000
GenericName=TK2000 Emulator
Comment=Microdigital TK2000 computer emulator
Exec=tk2000
Icon=tk2000
Terminal=false
Categories=Game;Emulator;
EOF

# 6) Bundle the libraries and pack (APPIMAGE_EXTRACT_AND_RUN lets linuxdeploy run without FUSE)
cd "$DIST"
APPIMAGE_EXTRACT_AND_RUN=1 LINUXDEPLOY_OUTPUT_VERSION="$VERSION" OUTPUT="$OUTPUT" \
	"$LINUXDEPLOY" --appdir "$APPDIR" --output appimage
rm -rf "$APPDIR"

GLIBC="$(ldd --version | awk 'NR == 1 {print $NF}')"
echo "Created $OUTPUT (glibc $GLIBC+)"
