#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="$ROOT/pokeemerald-expansion"
cp -f "$ROOT/overlay/test/gen3_surf_earthquake.c" "$GAME/test/battle/gen3_surf_earthquake.c"
cd "$GAME"
apt-get update -qq
apt-get install -y -qq libelf1 >/dev/null
# Force rebuild of this test object + re-link.
rm -f build/firered-test/test/battle/gen3_surf_earthquake.o pokefirered-test.elf pokefirered-test-headless.elf
make -j8 check TESTS='Gen3*' BUILD=firered
