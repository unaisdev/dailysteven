#!/usr/bin/env bash
# Vuelca el equipo que genera la ROM de PRUEBAS (build-wsl.ps1 -Debug) para varias semillas.
# Se ejecuta DENTRO de WSL. Necesita: mgba-sdl, xvfb (sudo apt install mgba-sdl xvfb).
#
#   bash tests/teams/dump_teams.sh "1 2 3" > equipos.txt
#   python3 tests/teams/verify_teams.py equipos.txt ~/dailysteven-build/pokeemerald-expansion
set -euo pipefail
SEEDS="${1:?uso: dump_teams.sh \"semilla semilla ...\"}"
ROMGZ="${ROMGZ:-/mnt/c/Users/Javier/orca/dailysteven-ronda1/web/game/dailymaximo-debug.gba.gz}"
ELF="${ELF:-$HOME/dailysteven-build/pokeemerald-expansion/pokefirered.elf}"
ADDR=$(arm-none-eabi-nm "$ELF" | awk '$3=="gDailyMaxTeamDump"{print $1}')
[ -n "$ADDR" ] || { echo "ROM sin gDailyMaxTeamDump: compílala con -Debug" >&2; exit 1; }
BASE=$((16#$ADDR))
gzip -dc "$ROMGZ" > /tmp/dm_dump_rom.gba
export SDL_AUDIODRIVER=dummy
for S in $SEEDS; do
  node -e '
const fs=require("fs");const r=fs.readFileSync("/tmp/dm_dump_rom.gba");const m=Buffer.from("DAILYMAX-CONF-V1");
const o=r.indexOf(m)+16;r.writeUInt32LE(Number(process.argv[1])>>>0,o);r.writeUInt16LE(0,o+4);r.writeUInt16LE(0,o+6);r.writeUInt8(1,o+8);
fs.writeFileSync("/tmp/dm_dump_seeded.gba",r)' "$S"
  pkill -x mgba 2>/dev/null || true; pkill Xvfb 2>/dev/null || true; sleep 1; rm -f /tmp/.X79-lock
  Xvfb :79 -screen 0 480x320x24 >/dev/null 2>&1 & XV=$!
  sleep 1; export DISPLAY=:79
  rm -f /tmp/dm_in; mkfifo /tmp/dm_in
  ( exec 3<>/tmp/dm_in; exec mgba -d /tmp/dm_dump_seeded.gba <&3 >/tmp/dm_out.txt 2>&1 ) &
  sleep 2
  exec 4>/tmp/dm_in
  echo c >&4
  sleep 6
  pkill -INT -x mgba; sleep 1
  for i in $(seq 0 101); do printf 'r/4 0x%x\n' $((BASE + i * 4)) >&4; done
  sleep 2
  echo q >&4; sleep 1
  pkill -x mgba 2>/dev/null || true; kill $XV 2>/dev/null || true
  exec 4>&-
  echo "SEED $S"
  grep -E '^ 0x' /tmp/dm_out.txt | tr -d ' ' | tr '\n' ' '
  echo
done
