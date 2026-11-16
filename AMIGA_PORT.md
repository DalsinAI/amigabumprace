BumpRace for GFOS
=================

A port of BumpRace, Karl Bartel's 2D racer, from
https://github.com/karlb/bumprace at commit
b7330a591b0664fdd9befb2697834439eac206f8 (BumpRace 1.5.8), for GFOS:
AmigaOS 3.2 with GFOpenUp, on a 68040 or 68060 with an FPU, on GFOpenUp's
SDL 2.

Fly a little racer through cave tracks under gravity, from the start to
the blue ring, against the clock, without touching the walls. Pick one of
four racers (best steering, extra time, a speed boost, or sticking to
walls), race alone, against a friend, or as a team of two, and make the
high score table.

Status: host tests pass (played through the keyboard on a virtual X
display); built in Cradle's os32-gcc16-open6 stove; not yet run in a lab.


What you need
-------------

- GFOS: AmigaOS 3.2 with GFOpenUp 0.8.26 or later, which installs
  LIBS:GFOpenGPU/SDL2.module, SDL2_image.module and SDL2_mixer.module.
- A 68040 or 68060 with an FPU (not an LC040 or EC040), an RTG screen
  mode of 800 x 600 (or a screen with room for an 800 x 600 window), and
  about 16 MB of free memory.
- Nothing else: the pictures, music and sounds are in the drawer.


How to play
-----------

Install it one of three ways:

- GFOpenUp: `GFOpenUpTool INSTALL Parts/BumpRace` puts it in
  SYS:Games/BumpRace.
- Run Install-BumpRace from the archive (it offers Work:Games).
- Or copy the BumpRace drawer where you want it.

Then double-click BumpRace. It opens its own 800 x 600 screen; from a
Shell, `BumpRace -w` plays in a window instead.

The menus are played with the cursor keys and Return. Player one flies
with the cursor keys (Up is thrust) and uses the racer's special ability
with Right Alt (an Amiga keyboard has no right Ctrl, which the racer
picture names); player two flies with W, A, D and uses Ctrl. Esc ends
the game.

The high scores (bumprace.dat) are kept in the drawer's User drawer.
Install-BumpRace and GFOpenUp keep User when they update the game.


Building
--------

    sh package/build-amiga.sh        (STOVE=/path/to/os32-gcc16-open6)

or, in Cradle, stove_build with the os32-gcc16-open6 stove and target
package. It fetches upstream at the commit package/bumprace.lock names,
applies amiga/patches, builds with amiga/Makefile.amigaos and packs
BumpRace-VERSION-68040.lha and the GFOpenUp part Parts/BumpRace.

Host tests: `sh tests/host/test_all.sh` (Linux, gcc, SDL 2, SDL2_image
and SDL2_mixer development files) checks that the patch applies; builds
the whole game with the host's compiler through the same Makefile; tests
the SDL 1.2 calls on SDL 2 (the 5-6-5 screen and its rectangles, the
video information, colour keys, the keys by SDL 2 key code and the
characters typed for the high score name) and the User drawer; and starts
the game with SDL's dummy drivers in a drawer laid out as on the Amiga,
where it must load every picture and sound, write its score file in
PROGDIR:User and wait at its title.


What was changed
----------------

- amiga/compat/: BumpRace is written for SDL 1.2. sdl12on2.c gives it the
  1.2 calls it makes, on SDL 2 (our own code, first written for Savage
  Wheels): SDL_SetVideoMode makes the window and a 16-bit 5-6-5 screen
  surface in memory, SDL_UpdateRect and SDL_UpdateRects copy the
  rectangles the game names to the window, SDL_Flip the whole screen, and
  SDL_GetVideoInfo, SDL_DisplayFormat, SDL_SetAlpha, SDL_AllocSurface and
  the caption map to SDL 2's. GFOpenGPU's own SDL 1.2 (sdl12-compat) has
  no SDL_image or SDL_mixer for 1.2, which the game needs, so the port
  uses SDL 2's, through this shim.
- amiga/patches/0001-amigaos-drawers-keys-sound.patch:
  - keys are read from SDL 2's keyboard state by SDL 2 key code, and the
    high score name's characters through sdl12on2_unicode() (SDL 2 has no
    keysym.unicode);
  - the scores are kept in PROGDIR:User/bumprace.dat;
  - player one's special key is Right Alt;
  - sound at 22050 Hz, half upstream's rate, to spare the 68040.
- The data are read from PROGDIR:Data (DATADIR, set by the Makefile).
- amiga/amiga_start.c: 128 KB of stack, the $VER string, and PROGDIR:User
  made at start.
- amiga/: the Makefile, the GlowIcons (our own art), the Installer script
  and its spec. package/: the build, the release archive and the GFOpenUp
  part.

The AmiCradle Team.
