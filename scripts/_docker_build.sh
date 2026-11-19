#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROOT_WIN="$(cd "$ROOT" && (pwd -W 2>/dev/null || pwd))"
cd "$ROOT"

# Strip CRLF from shell scripts (Windows checkouts)
find scripts -name '*.sh' -print0 | xargs -0 sed -i 's/\r$//'

cp -f overlay/src/dailymaximo.c pokeemerald-expansion/src/dailymaximo.c
cp -f overlay/src/dailymaximo_team.c pokeemerald-expansion/src/dailymaximo_team.c
cp -f overlay/include/dailymaximo.h pokeemerald-expansion/include/dailymaximo.h
cp -f overlay/include/constants/dailymaximo.h pokeemerald-expansion/include/constants/dailymaximo.h

docker build -t dailymaximo-build -f scripts/Dockerfile.build .

export MSYS_NO_PATHCONV=1
docker run --rm \
  -v "$ROOT_WIN:/src" \
  -w /src \
  dailymaximo-build \
  bash scripts/build.sh

ls -la web/game/dailymaximo.gba.gz
echo Done.
