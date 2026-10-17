#!/bin/bash
# Creates a drag-to-Applications DMG from a built "Artilody Sampleforge.app".
# Usage: packaging/macos/make-dmg.sh <path-to-app> <version> [output-dir]
set -euo pipefail
app="$1"
version="$2"
out_dir="${3:-dist}"
root="$(cd "$(dirname "$0")/../.." && pwd)"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
ditto "$app" "$stage/Artilody Sampleforge.app"
ln -s /Applications "$stage/Applications"
for manual in "$root"/docs/*.pdf; do
  [[ -f "$manual" ]] && cp "$manual" "$stage/"
done
mkdir -p "$out_dir"
dmg="$out_dir/Artilody-Sampleforge-$version-macOS.dmg"
rm -f "$dmg"
hdiutil create -volname "Artilody Sampleforge $version" -srcfolder "$stage" \
  -fs HFS+ -format UDZO -imagekey zlib-level=9 "$dmg"
echo "Created $dmg"
