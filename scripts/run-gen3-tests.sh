#!/usr/bin/env bash
# Pruebas automáticas de mecánicas de 3.ª gen (sistema de pruebas de la expansion, mGBA sin pantalla).
#   bash scripts/run-gen3-tests.sh [PREFIJO]      (por defecto «Gen3»; se ejecuta DENTRO de WSL/Linux, tras compilar)
# Copia overlay/test/*.c a pokeemerald-expansion/test/battle/ y lanza `make check`. Cada prueba comprueba el comportamiento
# real de la ROM (animaciones, barras de vida, estados), no la configuración. Salida: PASSED/FAILED por prueba.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${GAME_DIR:-$ROOT/pokeemerald-expansion}"
PREFIX="${1:-Gen3}"
mkdir -p "$GAME/test/battle"
cp -f "$ROOT"/overlay/test/*.c "$GAME/test/battle/"
cd "$GAME"
rm -f pokefirered-test.elf pokefirered-test-headless.elf
HOL_PLAY=1 make -j"$(nproc)" check TESTS="${PREFIX}*" BUILD=firered
