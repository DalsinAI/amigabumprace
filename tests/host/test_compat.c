/* Host test of amiga/compat/sdl12on2.c (SDL 1.2's calls on SDL 2, as
 * BumpRace makes them) and of amiga_start.c's br_make_drawer(), with SDL
 * 2's dummy video driver.
 *   test_compat <scratch dir>
 * Copyright (c) 2026 AmiCradle Team. GPL-2.0-or-later, like BumpRace. */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL.h>
#include "amiga_start.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static Uint16 win_px(int x, int y)
{
	SDL_Surface *w = SDL_GetWindowSurface(sdl12on2_window);
	Uint8 r, g, b;
	Uint32 p;

	SDL_LockSurface(w);
	p = *(Uint32 *)((Uint8 *)w->pixels + y * w->pitch + x * w->format->BytesPerPixel);
	SDL_UnlockSurface(w);
	if (w->format->BytesPerPixel == 2)
		p &= 0xFFFF;
	SDL_GetRGB(p, w->format, &r, &g, &b);
	return (Uint16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static void test_video(void)
{
	const SDL_VideoInfo *info;
	SDL_Surface *screen, *back, *pic, *disp;
	SDL_Rect r;
	Uint16 *px;

	/* gfx.c: the desktop's depth is asked first, then bpp 0 */
	info = SDL_GetVideoInfo();
	CHECK(info != NULL && info->vfmt != NULL && info->vfmt->BitsPerPixel == 16);
	screen = SDL_SetVideoMode(800, 600, 0, 0);
	CHECK(screen != NULL);
	if (!screen)
		return;
	CHECK(screen->w == 800 && screen->h == 600 && screen->format->BytesPerPixel == 2);
	CHECK(screen->format->Rmask == 0xF800 && screen->format->Gmask == 0x07E0 && screen->format->Bmask == 0x001F);
	CHECK(!SDL_MUSTLOCK(screen));
	SDL_WM_SetCaption("BumpRace", "BumpRace");
	CHECK(strcmp(SDL_GetWindowTitle(sdl12on2_window), "BumpRace") == 0);

	/* the back buffer, as gfx.c makes it */
	back = SDL_AllocSurface(screen->flags, 800, 600, screen->format->BitsPerPixel,
	                        screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, 0);
	CHECK(back != NULL && back->format->format == screen->format->format);
	SDL_FreeSurface(back);

	/* LoadImage: a picture keyed on white, then SDL_DisplayFormat */
	pic = SDL_CreateRGBSurfaceWithFormat(0, 4, 1, 32, SDL_PIXELFORMAT_ARGB8888);
	CHECK(pic != NULL);
	SDL_FillRect(pic, NULL, SDL_MapRGB(pic->format, 0xFF, 0xFF, 0xFF));
	CHECK(SDL_SetColorKey(pic, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB(pic->format, 0xFF, 0xFF, 0xFF)) == 0);
	disp = SDL_DisplayFormat(pic);
	SDL_FreeSurface(pic);
	CHECK(disp != NULL && disp->format->format == screen->format->format);
	if (disp) {
		Uint32 key = 0;
		CHECK(SDL_GetColorKey(disp, &key) == 0 && key == 0xFFFF);
		SDL_FillRect(screen, NULL, 0x1234);
		SDL_BlitSurface(disp, NULL, screen, NULL);
		px = (Uint16 *)screen->pixels;
		CHECK(px[0] == 0x1234 && px[3] == 0x1234);   /* all keyed: nothing drawn */
		/* the SDL_SetAlpha in bumprace.h: off, then on at 128 */
		SDL_SetAlpha(disp, 0, 0); SDL_SetAlpha(disp, SDL_SRCALPHA, 128);
		{
			Uint8 a = 0;
			SDL_BlendMode m;
			CHECK(SDL_GetSurfaceAlphaMod(disp, &a) == 0 && a == 128);
			CHECK(SDL_GetSurfaceBlendMode(disp, &m) == 0 && m == SDL_BLENDMODE_BLEND);
		}
		SDL_FreeSurface(disp);
	}

	/* the putpixel the game uses at 2 bytes a pixel, then the rectangles
	 * to the window: only what UpdateRects names is copied */
	SDL_FillRect(screen, NULL, 0);
	SDL_Flip(screen);
	px = (Uint16 *)screen->pixels;
	px[10 * screen->pitch / 2 + 10] = 0xF800;
	px[300 * screen->pitch / 2 + 400] = 0x07E0;
	r.x = 0; r.y = 0; r.w = 20; r.h = 20;
	SDL_UpdateRects(screen, 1, &r);
	CHECK(win_px(10, 10) == 0xF800);
	CHECK(win_px(400, 300) == 0);
	SDL_UpdateRect(screen, 390, 290, 20, 20);
	CHECK(win_px(400, 300) == 0x07E0);
	/* 0,0,0,0 is the whole screen, as on 1.2 */
	px[599 * screen->pitch / 2 + 799] = 0x001F;
	SDL_UpdateRect(screen, 0, 0, 0, 0);
	CHECK(win_px(799, 599) == 0x001F);
	/* rectangles off the edge are clipped, not refused */
	r.x = 790; r.y = 590; r.w = 40; r.h = 40;
	SDL_UpdateRects(screen, 1, &r);
	CHECK(win_px(799, 599) == 0x001F);
	CHECK(SDL_EnableUNICODE(1) == 0);
}

static void test_keys(void)
{
	Uint8 state[SDL_NUM_SCANCODES];
	SDLKey k = SDLK_RALT;

	memset(state, 0, sizeof state);
	state[SDL_SCANCODE_UP] = 1;
	state[SDL_SCANCODE_W] = 1;
	state[SDL_SCANCODE_RALT] = 1;
	CHECK(sdl12on2_key_down(state, SDLK_UP));
	CHECK(sdl12on2_key_down(state, SDLK_w));
	CHECK(sdl12on2_key_down(state, k));
	CHECK(!sdl12on2_key_down(state, SDLK_RCTRL));
	CHECK(!sdl12on2_key_down(state, SDLK_RETURN));
	CHECK(!sdl12on2_key_down(state, SDLK_ESCAPE));
}

static Uint16 uni(SDL_Keycode sym, Uint16 mod)
{
	SDL_Keysym k;

	memset(&k, 0, sizeof k);
	k.sym = sym;
	k.mod = mod;
	return sdl12on2_unicode(&k);
}

static void test_unicode(void)
{
	/* the high score name: font.c takes 32-127 and Backspace */
	CHECK(uni(SDLK_a, 0) == 'a');
	CHECK(uni(SDLK_a, KMOD_LSHIFT) == 'A');
	CHECK(uni(SDLK_z, KMOD_CAPS) == 'Z');
	CHECK(uni(SDLK_z, KMOD_CAPS | KMOD_RSHIFT) == 'z');
	CHECK(uni(SDLK_5, 0) == '5');
	CHECK(uni(SDLK_1, KMOD_SHIFT) == '!');
	CHECK(uni(SDLK_SLASH, KMOD_SHIFT) == '?');
	CHECK(uni(SDLK_BACKSLASH, KMOD_SHIFT) == '|');
	CHECK(uni(SDLK_QUOTE, KMOD_SHIFT) == '"');
	CHECK(uni(SDLK_SPACE, 0) == ' ');
	CHECK(uni(SDLK_KP_7, 0) == '7');
	CHECK(uni(SDLK_KP_0, 0) == '0');
	CHECK(uni(SDLK_RETURN, 0) == 13);       /* ends the name (SDLK_RETURN on 1.2) */
	CHECK(uni(SDLK_BACKSPACE, 0) == 8);
	CHECK(uni(SDLK_ESCAPE, 0) == 27);
	CHECK(uni(SDLK_UP, 0) == 0);
	CHECK(uni(SDLK_LSHIFT, KMOD_LSHIFT) == 0);
	CHECK(sdl12on2_unicode(NULL) == 0);
}

static void test_drawer(const char *dir)
{
	char d[512];
	FILE *f;
	struct stat st;

	snprintf(d, sizeof d, "%s/User", dir);
	rmdir(d);
	CHECK(br_make_drawer(d) == 1);
	CHECK(stat(d, &st) == 0 && S_ISDIR(st.st_mode));
	CHECK(br_make_drawer(d) == 0);
	snprintf(d, sizeof d, "%s/afile", dir);
	f = fopen(d, "wb");
	fclose(f);
	CHECK(br_make_drawer(d) == -1);
	remove(d);
}

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("FAIL SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	test_video();
	test_keys();
	test_unicode();
	SDL_Quit();
	test_drawer(argc > 1 ? argv[1] : ".");
	printf("%s: %d failed\n", argv[0], fails);
	return fails != 0;
}
