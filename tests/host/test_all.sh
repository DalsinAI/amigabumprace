#!/bin/sh
# Host tests (Linux, gcc, SDL 2, SDL2_image and SDL2_mixer development
# files):
#  1. upstream at the locked commit takes amiga/patches;
#  2. the whole game builds with the host's compiler through
#     amiga/Makefile.amigaos, on amiga/compat (SDL 1.2's calls on SDL 2);
#  3. test_compat: the 5-6-5 screen, the video information, the back
#     buffer, colour keys and alpha, the rectangles copied to the window,
#     the keys by SDL 2 key code, the high score name's characters, and the
#     User drawer;
#  4. the game itself starts with SDL's dummy drivers in a drawer laid out
#     as on the Amiga (PROGDIR:Data and PROGDIR:User are plain directory
#     names here): it must load every picture and sound, write its score
#     file in PROGDIR:User and wait at its title, until it is stopped;
#  5. with Xvfb, libXtst and ImageMagick's import on the host: the game is
#     played on a virtual X display through the keyboard (xkeys.py): title,
#     one player, the first racer, then thrust and steer; it must load the
#     racer and the level, and the racer must move on screen.
# Copyright (c) 2026 AmiCradle Team. GPL-2.0-or-later, like BumpRace.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
. "$ROOT/package/bumprace.lock"
T=${WORK:-"${TMPDIR:-/tmp}/amigabumprace-test"}
mkdir -p "$T"
cd "$ROOT"
ok() { echo "ok: $*"; }

# 1. upstream + patches
BR="$T/bumprace"
if [ "$(git -C "$BR" rev-parse HEAD 2>/dev/null)" != "$BR_COMMIT" ]; then
    rm -rf "$BR"; mkdir -p "$BR"
    git -C "$BR" init -q
    git -C "$BR" fetch -q --depth 1 "$BR_URL" "$BR_COMMIT"
    git -C "$BR" checkout -q FETCH_HEAD
fi
git -C "$BR" checkout -q -- . && git -C "$BR" clean -qfdx
for p in amiga/patches/*.patch; do git -C "$BR" apply --whitespace=nowarn "$ROOT/$p"; done
ok "patches apply to $BR_COMMIT"

# 2. the build
B="$T/build"
make -s -f amiga/Makefile.amigaos -j8 BR="$BR" BUILD="$B" CC=gcc CPU= NOIX= SDL2KIT=/usr OPT=-O1 \
    LIBS="$(sdl2-config --libs) -lSDL2_image -lSDL2_mixer -lm" "$B/BumpRace.debug" > "$T/make.log" 2>&1 \
    || { tail -30 "$T/make.log"; exit 1; }
if grep -E "implicit declaration|incompatible pointer|undefined reference" "$T/make.log"; then
    echo "FAIL: the build is not clean"; exit 1
fi
ok "game builds on the host through amiga/Makefile.amigaos"

# 3. the shim and the User drawer
gcc -O1 -Wall -Iamiga/compat -Iamiga $(sdl2-config --cflags) -o "$T/test_compat" \
    tests/host/test_compat.c amiga/compat/sdl12on2.c amiga/amiga_start.c $(sdl2-config --libs)
SDL_VIDEODRIVER=dummy "$T/test_compat" "$T"

# 4. the game starts, loads everything and waits at its title
R="$T/run"
rm -rf "$R"; mkdir -p "$R/PROGDIR:Data/gfx" "$R/PROGDIR:Data/sound" "$R/PROGDIR:User"
cp "$BR"/data/gfx/*.png "$BR"/data/gfx/*.jpg "$BR"/data/gfx/*.gif "$R/PROGDIR:Data/gfx/"
cp "$BR"/data/sound/*.ogg "$R/PROGDIR:Data/sound/"
set +e
(cd "$R" && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -s KILL 8 stdbuf -o0 "$B/BumpRace.debug" -w) > "$T/run.out" 2>&1
rc=$?
set -e
[ "$rc" = 137 ] || { echo "FAIL: the game stopped by itself (rc $rc)"; cat "$T/run.out"; exit 1; }
grep -q "Main data loaded" "$T/run.out" || { echo "FAIL: data not loaded"; cat "$T/run.out"; exit 1; }
if grep -E "Couldn't|SOUND TURNED OFF" "$T/run.out"; then echo "FAIL: a file or the sound failed"; exit 1; fi
grep -q "PROGDIR:User/bumprace.dat" "$T/run.out" || { echo "FAIL: scores not looked for in PROGDIR:User"; exit 1; }
[ "$(sed -n 3p "$R/PROGDIR:User/bumprace.dat")" = Sam ] || { echo "FAIL: no score file in PROGDIR:User"; exit 1; }
ok "the game loads every picture and sound, writes PROGDIR:User/bumprace.dat and waits at its title"
# 5. played through the keyboard on a virtual display
if command -v Xvfb >/dev/null && command -v import >/dev/null && python3 -c 'import ctypes; ctypes.CDLL("libXtst.so.6")' 2>/dev/null; then
    D=:$((70 + $$ % 20))
    Xvfb "$D" -screen 0 800x600x24 >/dev/null 2>&1 &
    XP=$!
    sleep 2
    rm -f "$R/PROGDIR:User/bumprace.dat"
    (cd "$R" && DISPLAY=$D SDL_AUDIODRIVER=dummy timeout -s KILL 30 stdbuf -o0 "$B/BumpRace.debug" -w) > "$T/play.out" 2>&1 &
    GP=$!
    sleep 4
    python3 "$HERE/xkeys.py" "$D" space sleep:1500 Return sleep:1500 Return sleep:2500 \
        shot:"$T/race0.png" Up:1500 Right:400 Up:800 shot:"$T/race1.png"
    kill -9 "$GP" 2>/dev/null || true; kill "$XP" 2>/dev/null || true
    wait 2>/dev/null || true
    grep -q "Racer data loaded" "$T/play.out" || { echo "FAIL: the race did not start"; cat "$T/play.out"; exit 1; }
    if cmp -s "$T/race0.png" "$T/race1.png"; then echo "FAIL: the racer did not move"; exit 1; fi
    ok "played on $D: title, menu, racer chosen, race started, the racer flies"
else
    echo "skip: no Xvfb, libXtst or import for the played test"
fi
echo "all host tests passed"
