/* The Amiga port's start (amiga_start.c).
 * Copyright (c) 2026 AmiCradle Team. GPL-2.0-or-later, like BumpRace. */
#ifndef BR_AMIGA_START_H
#define BR_AMIGA_START_H
/* Makes a drawer if it is not there: 1 made, 0 already there, -1 failed
 * (or a file of that name is in the way). */
int br_make_drawer(const char *path);
#endif
