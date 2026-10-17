#!/bin/bash
# Builds a Debian/Ubuntu .deb package from an already compiled Sampleforge binary.
# Usage: packaging/linux/make-deb.sh <path-to-binary> <version> [output-dir]
set -euo pipefail
binary="$1"
version="$2"
out_dir="${3:-dist}"
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
arch="$(dpkg --print-architecture)"
pkg=artilody-sampleforge
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT

install -Dm755 "$binary" "$stage/opt/$pkg/artilody-sampleforge"
mkdir -p "$stage/usr/bin"
ln -s "/opt/$pkg/artilody-sampleforge" "$stage/usr/bin/artilody-sampleforge"
install -Dm644 "$here/artilody-sampleforge.desktop" \
  "$stage/usr/share/applications/artilody-sampleforge.desktop"
for size in 48 64 128 256 512; do
  install -Dm644 "$here/icons/$size.png" \
    "$stage/usr/share/icons/hicolor/${size}x${size}/apps/artilody-sampleforge.png"
done
install -Dm644 "$root/Assets/icon.svg" \
  "$stage/usr/share/icons/hicolor/scalable/apps/artilody-sampleforge.svg"
doc="$stage/usr/share/doc/$pkg"
install -Dm644 "$root/LICENSE" "$doc/copyright"
install -Dm644 "$root/JUCE-LICENSE.md" "$doc/JUCE-LICENSE.md"
install -Dm644 "$root/INSTRUMENT-FORMAT.md" "$doc/INSTRUMENT-FORMAT.md"
for manual in "$root"/docs/*.pdf; do
  [[ -f "$manual" ]] && install -Dm644 "$manual" "$doc/$(basename "$manual")"
done

# Shared libraries linked directly (ALSA, FreeType, fontconfig, libc ...).
shlib_tmp="$(mktemp -d)"
mkdir -p "$shlib_tmp/debian"
printf 'Source: %s\n\nPackage: %s\nArchitecture: any\n' "$pkg" "$pkg" > "$shlib_tmp/debian/control"
linked="$(cd "$shlib_tmp" && dpkg-shlibdeps -O "$stage/opt/$pkg/artilody-sampleforge" 2>/dev/null \
  | sed -n 's/^shlibs:Depends=//p')"
rm -rf "$shlib_tmp"
# JUCE loads the X11 libraries at runtime with dlopen, so list them explicitly.
x11="libx11-6, libxext6, libxinerama1, libxrandr2, libxcursor1, libxrender1, libxcomposite1"
depends="${linked:+$linked, }$x11"
size_kb="$(du -sk "$stage" | cut -f1)"

mkdir -p "$stage/DEBIAN"
cat > "$stage/DEBIAN/control" <<EOF
Package: $pkg
Version: $version
Section: sound
Priority: optional
Architecture: $arch
Installed-Size: $size_kb
Depends: $depends
Recommends: fonts-dejavu-core
Maintainer: Artilody <noreply@github.com>
Homepage: https://github.com/shenphenchoedron-hue/Artilody-Sampleforge
Description: Chromatic sample splitter, tuner and SFZ exporter
 Artilody Sampleforge loads a recording of notes played one by one in
 ascending chromatic order, splits it into individual samples, measures
 their pitch, offers fine-tuning and exports a standard SFZ 1.0 instrument
 with 24-bit WAV samples.
EOF
cat > "$stage/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database -q || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
exit 0
EOF
cp "$stage/DEBIAN/postinst" "$stage/DEBIAN/postrm"
chmod 755 "$stage/DEBIAN/postinst" "$stage/DEBIAN/postrm"

mkdir -p "$out_dir"
deb="$out_dir/${pkg}_${version}_${arch}.deb"
dpkg-deb --build --root-owner-group "$stage" "$deb"
echo "Created $deb"
