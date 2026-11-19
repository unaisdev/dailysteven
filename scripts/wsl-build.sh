#!/usr/bin/env bash
# Parte "de dentro" de scripts/build-wsl.ps1. Se ejecuta DENTRO de WSL (Ubuntu).
#
#   1. Copia el worktree de Windows a un disco rápido de WSL (~/dailysteven-build) con finales LF.
#   2. Si el árbol de pokeemerald-expansion no existe o está desfasado respecto a los scripts, la
#      traducción, el parche o los NOMBRES de dailymax_texts.h, lo regenera desde cero
#      (get-expansion.sh). Si no, solo copia el overlay (rápido).
#   3. Compila (scripts/build.sh).
#   4. Deja la ROM en  <worktree>/web/game/dailymaximo.gba.gz  y descarga web/vendor si falta.
#   5. Escribe web/game/BUILD-INFO.txt (hora, versión de las fuentes y huella de la ROM).
#
# Uso (normalmente lo llama el .ps1):  bash scripts/wsl-build.sh /mnt/c/ruta/al/worktree [--clean]
set -euo pipefail

SRC="${1:?uso: wsl-build.sh RUTA_WORKTREE_EN_WSL [--clean] [--debug]}"
CLEAN=""
DEBUG=""
for a in "${@:2}"; do
  case "$a" in --clean) CLEAN="--clean";; --debug) DEBUG="--debug";; esac
done
DST="${DAILYSTEVEN_WSL_DIR:-$HOME/dailysteven-build}"

echo "== [1/5] Sincronizando $SRC -> $DST"
mkdir -p "$DST"
rsync -a --delete \
  --exclude .git --exclude pokeemerald-expansion --exclude web/vendor \
  --exclude 'web/game/*.gba.gz' --exclude 'web/game/BUILD-INFO*.txt' \
  --exclude .cache --exclude .toolchain \
  "$SRC"/ "$DST"/
cd "$DST"
find scripts overlay -type f \( -name '*.sh' -o -name '*.py' -o -name '*.c' -o -name '*.h' -o -name '*.inc' -o -name '*.mjs' \) \
  -exec sed -i 's/\r$//' {} +
for f in scripts/*.sh; do bash -n "$f" || { echo "ERROR de sintaxis en $f"; exit 1; }; done

# Huella de lo que condiciona el árbol de la expansion (si cambia, hay que regenerarlo).
tree_hash() {
  {
    for f in scripts/get-expansion.sh scripts/apply-dailymaximo.sh scripts/patch-dailymaximo-hooks.py \
             scripts/apply-locale.sh scripts/apply_spanish_locale.py scripts/translate_battle_strings_table.py \
             scripts/normalize-binary-patch.py scripts/locale_guard.py overlay/data/dailymax_rivals.party \
             patches/hall-of-legends.patch; do
      tr -d '\r' < "$f"
    done
    find locale/es -type f | LC_ALL=C sort | while read -r f; do tr -d '\r' < "$f"; done
    grep -h '#define DAILYMAX_NAME_' overlay/include/dailymax_texts.h | tr -d '\r'
  } | sha256sum | cut -c1-16
}
# Huella de TODAS las fuentes (solo informativa, para BUILD-INFO).
source_hash() {
  find scripts overlay locale content web -type f ! -path 'web/vendor/*' ! -name '*.gz' ! -name BUILD-INFO.txt 2>/dev/null \
    | LC_ALL=C sort | while read -r f; do tr -d '\r' < "$f"; done | sha256sum | cut -c1-16
}
WANT="$(tree_hash)"
HAVE=""
[ -f pokeemerald-expansion/.dailymax-tree-hash ] && HAVE="$(cat pokeemerald-expansion/.dailymax-tree-hash)"

echo "== [2/5] Árbol de la expansion"
if [ ! -d pokeemerald-expansion ] || [ "$HAVE" != "$WANT" ] || [ "$CLEAN" = "--clean" ]; then
  echo "   (re)generando desde cero: huella $HAVE -> $WANT"
  rm -rf pokeemerald-expansion
  HOL_PLAY=1 bash scripts/get-expansion.sh
  echo "$WANT" > pokeemerald-expansion/.dailymax-tree-hash
else
  echo "   árbol al día ($WANT); solo se copia el overlay"
  bash scripts/apply-dailymaximo.sh --overlay-only
fi

if [ -n "$DEBUG" ]; then
  echo "   *** ROM DE PRUEBAS (-Debug): semillas 900001 = derrota, 900002 = victoria ***"
  sed -i 's/^#define DAILYMAX_DEBUG 0/#define DAILYMAX_DEBUG 1/' pokeemerald-expansion/include/constants/dailymaximo.h
  grep -q '^#define DAILYMAX_DEBUG 1' pokeemerald-expansion/include/constants/dailymaximo.h || { echo "ERROR: no se pudo activar DAILYMAX_DEBUG"; exit 1; }
fi

echo "== [3/5] Compilando"
HOL_PLAY=1 bash scripts/build.sh

echo "== [4/5] Copiando la ROM y el emulador a $SRC/web"
mkdir -p "$SRC/web/game" "$SRC/web/vendor"
if [ -n "$DEBUG" ]; then
  OUTNAME=dailymaximo-debug.gba.gz   # la ROM de pruebas NUNCA pisa a la normal
  INFONAME=BUILD-INFO-debug.txt
else
  OUTNAME=dailymaximo.gba.gz
  INFONAME=BUILD-INFO.txt
fi
cp -f web/game/dailymaximo.gba.gz "$SRC/web/game/$OUTNAME"
if [ ! -f "$SRC/web/vendor/nostalgist.js" ]; then
  bash scripts/fetch-vendor.sh
  rsync -a web/vendor/ "$SRC/web/vendor/"
fi

echo "== [5/5] BUILD-INFO"
ROM_SHA="$(gzip -dc web/game/dailymaximo.gba.gz | sha256sum | cut -c1-16)"
GZ_SHA="$(sha256sum "$SRC/web/game/$OUTNAME" | cut -c1-16)"
GIT_INFO="$(git -c safe.directory='*' -C "$SRC" rev-parse --short HEAD 2>/dev/null || echo '?')"
{
  echo "compilada:        $(date '+%Y-%m-%d %H:%M:%S')"
  echo "commit base:      $GIT_INFO (+ cambios sin confirmar si los hay)"
  echo "huella fuentes:   $(source_hash)"
  echo "huella arbol:     $WANT"
  echo "huella ROM:       $ROM_SHA"
  echo "huella .gba.gz:   $GZ_SHA"
  echo "tamano .gba.gz:   $(stat -c %s "$SRC/web/game/$OUTNAME") bytes"
  if [ -n "$DEBUG" ]; then echo "tipo:             PRUEBAS (-Debug)"; else echo "tipo:             NORMAL"; fi
} | tee "$SRC/web/game/$INFONAME"
echo "LISTO."
