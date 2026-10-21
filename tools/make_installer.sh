#!/bin/bash
# Builds build/installer/Pressura-<version>.pkg from the Release build.
#
#   ./tools/make_installer.sh
#
# Unsigned by default: macOS warns once when the .pkg is opened (Privacy & Security → Open Anyway),
# but the installed plugins are not quarantined, so no Terminal step is needed.
# With an Apple Developer account, set these to sign and notarize instead:
#   SIGN_APP="Developer ID Application: Name (TEAMID)"
#   SIGN_INSTALLER="Developer ID Installer: Name (TEAMID)"
#   NOTARY_PROFILE=<keychain profile created with: xcrun notarytool store-credentials>
set -euo pipefail
cd "$(dirname "$0")/.."

NAME=Pressura
ID=com.gueibrielon.pressura
VERSION=$(sed -n 's/^project(OPTO2 VERSION \([0-9.]*\)).*/\1/p' CMakeLists.txt)
ART=build/OPTO2_artefacts/Release
OUT=build/installer
WORK=$OUT/work

for f in "$ART/AU/$NAME.component" "$ART/VST3/$NAME.vst3" "$ART/Standalone/$NAME.app"; do
    [ -d "$f" ] || { echo "Missing $f: build Release first (cmake --build build)"; exit 1; }
done

rm -rf "$OUT"
mkdir -p "$WORK"/{au,vst3,app,resources}
cp -R "$ART/AU/$NAME.component"   "$WORK/au/"
cp -R "$ART/VST3/$NAME.vst3"      "$WORK/vst3/"
cp -R "$ART/Standalone/$NAME.app" "$WORK/app/"
cp tools/installer/welcome.html   "$WORK/resources/"
cp Resources/OFL.txt              "$WORK/resources/license-font.txt"
xattr -cr "$WORK"

# Developer ID if given, otherwise ad-hoc (free; Apple Silicon needs every bundle signed to load)
for b in "$WORK/au/$NAME.component" "$WORK/vst3/$NAME.vst3" "$WORK/app/$NAME.app"; do
    if [ -n "${SIGN_APP:-}" ]; then
        codesign --force --deep --options runtime --timestamp --sign "$SIGN_APP" "$b"
    else
        codesign --force --deep --sign - "$b"
    fi
done

# One component package per format. Bundles must not be relocatable, or Installer
# "upgrades" any copy with the same bundle ID wherever it finds it instead of installing here.
component () {  # <dir> <pkg id suffix> <install location>
    local plist="$WORK/$2.plist"
    pkgbuild --analyze --root "$WORK/$1" "$plist" >/dev/null
    plutil -replace 0.BundleIsRelocatable -bool NO "$plist"
    pkgbuild --root "$WORK/$1" --component-plist "$plist" --identifier "$ID.$2" \
             --version "$VERSION" --install-location "$3" "$WORK/$NAME-$2.pkg" >/dev/null
}
component au   au   /Library/Audio/Plug-Ins/Components
component vst3 vst3 /Library/Audio/Plug-Ins/VST3
component app  app  /Applications

cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>$NAME $VERSION</title>
    <welcome file="welcome.html" mime-type="text/html"/>
    <readme file="license-font.txt" mime-type="text/plain"/>
    <options customize="allow" require-scripts="false" hostArchitectures="x86_64,arm64"/>
    <domains enable_localSystem="true"/>
    <os-version min="11.0"/>
    <choices-outline>
        <line choice="au"/>
        <line choice="vst3"/>
        <line choice="app"/>
    </choices-outline>
    <choice id="au"   title="Audio Unit"     description="For Logic, GarageBand, Ableton and other AU hosts."><pkg-ref id="$ID.au"/></choice>
    <choice id="vst3" title="VST3"           description="For Ableton, Reaper, Bitwig, Cubase and other VST3 hosts."><pkg-ref id="$ID.vst3"/></choice>
    <choice id="app"  title="Standalone app" description="Pressura as its own app, in /Applications."><pkg-ref id="$ID.app"/></choice>
    <pkg-ref id="$ID.au"   version="$VERSION">$NAME-au.pkg</pkg-ref>
    <pkg-ref id="$ID.vst3" version="$VERSION">$NAME-vst3.pkg</pkg-ref>
    <pkg-ref id="$ID.app"  version="$VERSION">$NAME-app.pkg</pkg-ref>
</installer-gui-script>
XML

PKG="$OUT/$NAME-$VERSION.pkg"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" \
             --resources "$WORK/resources" ${SIGN_INSTALLER:+--sign "$SIGN_INSTALLER"} "$PKG"

if [ -n "${NOTARY_PROFILE:-}" ]; then
    xcrun notarytool submit "$PKG" --keychain-profile "$NOTARY_PROFILE" --wait
    xcrun stapler staple "$PKG"
fi

rm -rf "$WORK"
echo "Built $PKG"
