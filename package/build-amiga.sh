#!/bin/sh
# The release archive and the GFOpenUp part, for Cradle's stove_build
# (target package):
#  1. upstream BumpRace (code and data), fetched at the commit
#     package/bumprace.lock names (it is never kept in this repository),
#     with amiga/patches applied;
#  2. the game built by amiga/Makefile.amigaos;
#  3. the drawer, its icon and the Installer script beside it packed as
#     NAME-VERSION-68040.lha (tools/pack_dist.py), with its .sha256 and a
#     short .readme;
#  4. OUT/Parts/BumpRace, a GFOpenUp part: its Part file and the drawer's
#     files for SYS:Games/BumpRace, installed with
#       GFOpenUpTool INSTALL Parts/BumpRace
#     It needs the GFOpenGPU part, whose SDL 2 modules draw and play it.
# Nothing is published from here.
# Copyright (c) 2026 AmiCradle Team. GPL-2.0-or-later, like BumpRace.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
. "$HERE/package.env"
. "$HERE/bumprace.lock"
OUT=${OUT:-"$ROOT/build-amiga-package"}
mkdir -p "$OUT"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
STOVE=${STOVE:-"$HOME/AmiCradle/kitchen/stoves/os32-gcc16-open6"}
# outside the checkout: Cradle cleans it (git clean -ffdx) before every job
WORK=${WORK:-"$HOME/.cache/amigabumprace"}
mkdir -p "$WORK"
cd "$ROOT"

# 1. upstream at the locked commit, patched afresh every time
BR="$WORK/bumprace"
if [ "$(git -C "$BR" rev-parse HEAD 2>/dev/null)" != "$BR_COMMIT" ]; then
    rm -rf "$BR"; mkdir -p "$BR"
    git -C "$BR" init -q
    git -C "$BR" fetch -q --depth 1 "$BR_URL" "$BR_COMMIT"
    git -C "$BR" checkout -q FETCH_HEAD
fi
git -C "$BR" checkout -q -- . && git -C "$BR" clean -qfdx
for p in "$ROOT"/amiga/patches/*.patch; do
    git -C "$BR" apply --whitespace=nowarn "$p"
done

# 2. the game (a stove's compiler is in its prefix/, or STOVE is the prefix)
if [ -x "$STOVE/prefix/bin/m68k-amigaos-gcc" ]; then PREFIX="$STOVE/prefix"; else PREFIX="$STOVE"; fi
rm -rf "$DIST"
make -f amiga/Makefile.amigaos BR="$BR" STOVE="$STOVE" PREFIX="$PREFIX" -j4 $TARGET > "$OUT/build-output.txt" 2>&1 || { tail -n 60 "$OUT/build-output.txt"; exit 1; }

# 3. the archive
python3 "$ROOT/tools/pack_dist.py" --dist "$DIST" --name "$NAME" --out "$OUT"

# 4. the GFOpenUp part
sh "$ROOT/package/write-part.sh" "$DIST/$NAME" "$OUT/Parts/$NAME" "$NAME" "$PORT_VERSION" \
    "BumpRace: race through 2D gravity caves against the clock, 1 or 2 players"
echo "part: $(grep -c '^copy' "$OUT/Parts/$NAME/Part") files, $(du -sk "$OUT/Parts/$NAME" | cut -f1) KB"
ls -l "$DIST/$NAME/$NAME"
