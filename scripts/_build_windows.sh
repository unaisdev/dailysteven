#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ARM_BIN="/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin"
export PATH="$ARM_BIN:$PATH"

if ! command -v arm-none-eabi-gcc >/dev/null; then
  echo "arm-none-eabi-gcc not found in $ARM_BIN"
  exit 1
fi

# Git Bash often has no `make`; try several locations or download a portable make.
if ! command -v make >/dev/null; then
  if [ -x "$ROOT/.toolchain/make/bin/make" ]; then
    export PATH="$ROOT/.toolchain/make/bin:$PATH"
  else
    echo "Installing portable GNU make into .toolchain/make ..."
    mkdir -p "$ROOT/.toolchain"
    TMP="$(mktemp -d)"
    curl -fsSL -o "$TMP/make.zip" "https://sourceforge.net/projects/ezwinports/files/make-4.4.1-without-guile-w32-bin.zip/download"
    mkdir -p "$ROOT/.toolchain/make"
    unzip -oq "$TMP/make.zip" -d "$ROOT/.toolchain/make"
    rm -rf "$TMP"
    export PATH="$ROOT/.toolchain/make/bin:$PATH"
  fi
fi

# libpng is needed by gbagfx — try choco/system or skip if already present.
if ! pkg-config --exists libpng 2>/dev/null; then
  echo "Note: libpng via pkg-config not found; expansion tools may still build if headers are available."
fi

echo "gcc: $(arm-none-eabi-gcc --version | head -1)"
echo "make: $(make --version | head -1)"
cd "$ROOT"
# Re-copy overlay sources in case we edited them after apply
cp -f overlay/src/dailymaximo.c pokeemerald-expansion/src/dailymaximo.c
cp -f overlay/src/dailymaximo_team.c pokeemerald-expansion/src/dailymaximo_team.c
cp -f overlay/include/dailymaximo.h pokeemerald-expansion/include/dailymaximo.h
bash scripts/build.sh
