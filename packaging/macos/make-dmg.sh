#!/bin/bash
# Builds TK2000.app (with SDL bundled) and packs it into dist/TK2000.dmg
#
# Requirements: Xcode command line tools, CMake and SDL2 from Homebrew (brew install cmake sdl2)
# The app runs on the same architecture and minimum macOS version as the Homebrew libraries used.
#
# Usage: packaging/macos/make-dmg.sh [version]

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${1:-1.0}"
BUILD="$ROOT/build-release"
DIST="$ROOT/dist"
APP="$DIST/TK2000.app"
FRAMEWORKS="$APP/Contents/Frameworks"

# 1) Release build
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" -j

# 2) Bundle layout
rm -rf "$DIST"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$FRAMEWORKS"
cp "$BUILD/src/tk2000" "$APP/Contents/MacOS/tk2000"
cp "$ROOT/res/tk2000-frame.png" "$APP/Contents/Resources/"

# 3) SDL libraries: SDL2 linked by the executable and, for sdl2-compat, the SDL3 it loads at runtime
#    (sdl2-compat looks for @loader_path/libSDL3.dylib, so both go side by side)
SDL2_PATH="$(otool -L "$APP/Contents/MacOS/tk2000" | awk '/libSDL2/ {print $1}')"
cp -L "$SDL2_PATH" "$FRAMEWORKS/libSDL2-2.0.0.dylib"
install_name_tool -change "$SDL2_PATH" "@executable_path/../Frameworks/libSDL2-2.0.0.dylib" "$APP/Contents/MacOS/tk2000"
install_name_tool -id "@executable_path/../Frameworks/libSDL2-2.0.0.dylib" "$FRAMEWORKS/libSDL2-2.0.0.dylib"
if strings "$FRAMEWORKS/libSDL2-2.0.0.dylib" | grep -q "libSDL3.dylib"; then
	cp -L "$(brew --prefix sdl3)/lib/libSDL3.0.dylib" "$FRAMEWORKS/libSDL3.dylib"
	install_name_tool -id "@loader_path/libSDL3.dylib" "$FRAMEWORKS/libSDL3.dylib"
fi
chmod u+w "$FRAMEWORKS"/*.dylib

# 4) Icon from the computer picture (square crop of the center)
ICONSET="$DIST/TK2000.iconset"
mkdir -p "$ICONSET"
sips -c 1792 1792 "$ROOT/res/tk2000-frame.png" --out "$DIST/icon.png" >/dev/null
for size in 16 32 128 256 512; do
	sips -z $size $size "$DIST/icon.png" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
	sips -z $((size * 2)) $((size * 2)) "$DIST/icon.png" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/TK2000.icns"
rm -rf "$ICONSET" "$DIST/icon.png"

# 5) Info.plist
MINOS="$(otool -l "$APP/Contents/MacOS/tk2000" | awk '/LC_BUILD_VERSION/ {f=1} f && /minos/ {print $2; exit}')"
cat > "$APP/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key><string>TK2000</string>
	<key>CFBundleDisplayName</key><string>TK2000 Emulator</string>
	<key>CFBundleIdentifier</key><string>org.fblabs.tk2000</string>
	<key>CFBundleExecutable</key><string>tk2000</string>
	<key>CFBundleIconFile</key><string>TK2000</string>
	<key>CFBundlePackageType</key><string>APPL</string>
	<key>CFBundleShortVersionString</key><string>$VERSION</string>
	<key>CFBundleVersion</key><string>$VERSION</string>
	<key>LSMinimumSystemVersion</key><string>$MINOS</string>
	<key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
EOF

# 6) Ad-hoc signature (required on Apple Silicon after changing the binaries)
codesign --force --sign - "$FRAMEWORKS"/*.dylib
codesign --force --sign - "$APP"

# 7) Disk image with a shortcut to Applications
STAGE="$DIST/dmg"
mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
hdiutil create -volname "TK2000" -srcfolder "$STAGE" -ov -format UDZO "$DIST/TK2000-$VERSION.dmg" >/dev/null
rm -rf "$STAGE"

echo "Created $DIST/TK2000-$VERSION.dmg (macOS $MINOS+, $(lipo -archs "$APP/Contents/MacOS/tk2000"))"
