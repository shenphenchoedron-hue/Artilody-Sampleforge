#!/bin/bash
# Builds Artilody Sampleforge from source on Debian / Ubuntu and creates a .deb.
# Run:  bash Build-Linux.sh
set -euo pipefail
cd "$(dirname "$0")"
trap 'printf "\nBuild failed / Build fejlede. Copy the error message above.\n"' ERR

if [[ "$(uname -s)" != "Linux" ]] || ! command -v apt-get >/dev/null 2>&1; then
  echo "This script is for Debian / Ubuntu Linux (apt)."
  exit 1
fi

packages=(build-essential cmake pkg-config git curl ca-certificates dpkg-dev
  libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev
  libxrandr-dev libxrender-dev libgl1-mesa-dev)
missing=()
for p in "${packages[@]}"; do
  dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q "install ok installed" || missing+=("$p")
done
if (( ${#missing[@]} )); then
  echo "Installing build dependencies: ${missing[*]}"
  sudo apt-get update
  sudo apt-get install -y "${missing[@]}"
fi

# JUCE 8 needs CMake >= 3.24 (Debian 12 / Ubuntu 24.04 ship that). Older
# systems get a verified private CMake copy in .tools/.
cmake_bin="$(command -v cmake)"
version="$("$cmake_bin" --version | head -1 | awk '{print $3}')"
major="${version%%.*}"; rest="${version#*.}"; minor="${rest%%.*}"
if [[ "$major" -lt 3 || ( "$major" -eq 3 && "$minor" -lt 24 ) ]]; then
  arch="$(uname -m)"
  name="cmake-3.31.6-linux-$arch"
  case "$arch" in
    x86_64|aarch64) ;;
    *) echo "Unsupported CPU architecture: $arch"; exit 1 ;;
  esac
  cmake_bin="$PWD/.tools/$name/bin/cmake"
  if [[ ! -x "$cmake_bin" ]]; then
    mkdir -p .tools
    echo "Downloading CMake 3.31.6 into .tools/ ..."
    curl --fail --location --retry 2 \
      "https://github.com/Kitware/CMake/releases/download/v3.31.6/$name.tar.gz" \
      --output ".tools/$name.tar.gz"
    curl --fail --location --retry 2 \
      "https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6-SHA-256.txt" \
      --output ".tools/cmake-SHA-256.txt"
    (cd .tools && grep " $name.tar.gz\$" cmake-SHA-256.txt | sha256sum -c -)
    tar -xzf ".tools/$name.tar.gz" -C .tools
  fi
fi

build_dir="$PWD/build-linux"
echo "Building Artilody Sampleforge (Release) - first build downloads JUCE 8.0.4."
"$cmake_bin" -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
"$cmake_bin" --build "$build_dir" --parallel "$(nproc)"
"$(dirname "$cmake_bin")/ctest" --test-dir "$build_dir" --output-on-failure

app="$build_dir/Sampleforge_artefacts/Release/Artilody Sampleforge"
[[ -x "$app" ]] || { echo "Binary not found: $app"; exit 1; }
version="$(sed -n 's/^project(ArtilodySampleforge VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
bash packaging/linux/make-deb.sh "$app" "$version" dist
echo
echo "Done. Install with:"
echo "  sudo apt install ./dist/artilody-sampleforge_${version}_$(dpkg --print-architecture).deb"
echo "Or run directly:  \"$app\""
