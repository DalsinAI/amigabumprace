/* The Amiga port's start: the stack, the version string and PROGDIR:User,
 * where the game keeps its high scores (bumprace.dat), made before main().
 * Install-BumpRace and GFOpenUp keep User when they update the game.
 * Copyright (c) 2026 AmiCradle Team. GPL-2.0-or-later, like BumpRace. */
#include <sys/stat.h>
#include "amiga_start.h"

/* A Shell gives 4 KB of stack; SDL 2 and the game want more. libnix's
 * startup sets this much up. */
unsigned long __stack = 128 * 1024;
static const char version[] __attribute__((used)) =
    "$VER: BumpRace 1.5.8 (16.11.2026)";

int br_make_drawer(const char *path)
{
	struct stat st;

	if (stat(path, &st) == 0)
		return S_ISDIR(st.st_mode) ? 0 : -1;
	return mkdir(path, 0755) == 0 ? 1 : -1;
}

#ifdef __amigaos__
static void __attribute__((constructor)) amiga_user_drawer(void)
{
	br_make_drawer("PROGDIR:User");
}
#endif
