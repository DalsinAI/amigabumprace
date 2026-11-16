/* SDL 1.2's video calls on SDL 2: see sdl12on2.h.
 * The AmiCradle Team's own code, first written for Savage Wheels (MIT);
 * here under BumpRace's licence, GPL-2.0-or-later.
 * Copyright (c) 2026 AmiCradle Team. */
#include <string.h>
#include <SDL.h>

SDL_Window *sdl12on2_window;
SDL_Surface *sdl12on2_screen;
static char sdl12on2_title[128] = "BumpRace";

Uint32 sdl12on2_screen_format(int bpp)
{
    if (bpp == 15)
        return SDL_PIXELFORMAT_RGB555;
    if (bpp == 32 || bpp == 24)
        return SDL_PIXELFORMAT_RGB888;
    return SDL_PIXELFORMAT_RGB565;
}

SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags)
{
    Uint32 fmt = sdl12on2_screen_format(bpp);
    int depth;
    Uint32 r, g, b, a;

    if (sdl12on2_screen) {
        SDL_FreeSurface(sdl12on2_screen);
        sdl12on2_screen = NULL;
    }
    if (!sdl12on2_window) {
        sdl12on2_window = SDL_CreateWindow(sdl12on2_title,
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                (flags & SDL_FULLSCREEN) ? SDL_WINDOW_FULLSCREEN : 0);
        if (!sdl12on2_window)
            return NULL;
    } else {
        SDL_SetWindowSize(sdl12on2_window, w, h);
    }
    if (!SDL_PixelFormatEnumToMasks(fmt, &depth, &r, &g, &b, &a))
        return NULL;
    sdl12on2_screen = SDL_CreateRGBSurface(0, w, h, depth, r, g, b, a);
    return sdl12on2_screen;
}

int SDL_Flip(SDL_Surface *screen)
{
    SDL_Surface *win;

    if (!sdl12on2_window || !screen)
        return -1;
    win = SDL_GetWindowSurface(sdl12on2_window);
    if (!win)
        return -1;
    /* a plain copy: the screen has no colour key or alpha */
    if (SDL_BlitSurface(screen, NULL, win, NULL) < 0)
        return -1;
    return SDL_UpdateWindowSurface(sdl12on2_window);
}

int SDL_WM_ToggleFullScreen(SDL_Surface *screen)
{
    Uint32 now;

    (void)screen;
    if (!sdl12on2_window)
        return 0;
    now = SDL_GetWindowFlags(sdl12on2_window) & SDL_WINDOW_FULLSCREEN;
    return SDL_SetWindowFullscreen(sdl12on2_window, now ? 0 : SDL_WINDOW_FULLSCREEN) == 0;
}

void SDL_WM_SetCaption(const char *title, const char *icon)
{
    (void)icon;
    if (!title)
        return;
    strncpy(sdl12on2_title, title, sizeof sdl12on2_title - 1);
    sdl12on2_title[sizeof sdl12on2_title - 1] = 0;
    if (sdl12on2_window)
        SDL_SetWindowTitle(sdl12on2_window, sdl12on2_title);
}

/* SDL 1.2 converted to the screen's format and kept the colour key and the
 * surface alpha; SDL 2's SDL_ConvertSurface keeps both too. */
SDL_Surface *SDL_DisplayFormat(SDL_Surface *s)
{
    if (!s)
        return NULL;
    if (sdl12on2_screen)
        return SDL_ConvertSurface(s, sdl12on2_screen->format, 0);
    return SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGB565, 0);
}

int SDL_SetAlpha(SDL_Surface *s, Uint32 flags, Uint8 alpha)
{
    if (!s)
        return -1;
    if (flags & SDL_SRCALPHA) {
        SDL_SetSurfaceAlphaMod(s, alpha);
        return SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_BLEND);
    }
    SDL_SetSurfaceAlphaMod(s, 255);
    return SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);
}

const SDL_VideoInfo *SDL_GetVideoInfo(void)
{
    static SDL_VideoInfo info;
    static SDL_PixelFormat *fmt;
    SDL_DisplayMode m;

    memset(&info, 0, sizeof info);
    /* SDL 1.2 named the desktop's format; the screen here is 5-6-5 */
    if (!fmt)
        fmt = SDL_AllocFormat(sdl12on2_screen ? sdl12on2_screen->format->format
                                              : SDL_PIXELFORMAT_RGB565);
    info.vfmt = fmt;
    info.wm_available = 1;
    info.blit_sw = info.blit_sw_CC = info.blit_sw_A = 1;
    if (SDL_GetDesktopDisplayMode(0, &m) == 0) {
        info.current_w = m.w;
        info.current_h = m.h;
    }
    return &info;
}

char *SDL_VideoDriverName(char *buf, int len)
{
    const char *d = SDL_GetCurrentVideoDriver();

    if (!buf || len <= 0 || !d)
        return NULL;
    strncpy(buf, d, (size_t)len - 1);
    buf[len - 1] = 0;
    return buf;
}

SDL_Rect **SDL_ListModes(const SDL_PixelFormat *fmt, Uint32 flags)
{
    (void)fmt; (void)flags;
    return (SDL_Rect **)-1;          /* any size: it is a window */
}

int SDL_VideoModeOK(int w, int h, int bpp, Uint32 flags)
{
    (void)w; (void)h; (void)flags;
    return bpp == 15 ? 15 : bpp == 32 ? 32 : 16;
}

int sdl12on2_key_down(const Uint8 *keys, int key)
{
    SDL_Scancode sc;

    if (!keys)
        return 0;
    sc = SDL_GetScancodeFromKey((SDL_Keycode)key);
    return sc != SDL_SCANCODE_UNKNOWN && keys[sc];
}

/* the screen's rectangles to the window: a plain copy, clipped to both */
static void sdl12on2_copy(SDL_Surface *screen, SDL_Surface *win, SDL_Rect *r)
{
    SDL_Rect d = *r;

    SDL_BlitSurface(screen, r, win, &d);
}

void SDL_UpdateRects(SDL_Surface *screen, int n, SDL_Rect *rects)
{
    SDL_Surface *win;
    int i;

    if (!sdl12on2_window || !screen || n <= 0 || !rects)
        return;
    win = SDL_GetWindowSurface(sdl12on2_window);
    if (!win)
        return;
    for (i = 0; i < n; i++)
        sdl12on2_copy(screen, win, &rects[i]);
    SDL_UpdateWindowSurfaceRects(sdl12on2_window, rects, n);
}

void SDL_UpdateRect(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{
    SDL_Rect r;

    if (!screen)
        return;
    if (x == 0 && y == 0 && w == 0 && h == 0) {   /* 1.2: the whole screen */
        SDL_Flip(screen);
        return;
    }
    r.x = x; r.y = y; r.w = (int)w; r.h = (int)h;
    SDL_UpdateRects(screen, 1, &r);
}

Uint16 sdl12on2_unicode(const SDL_Keysym *k)
{
    static const char from[] = "1234567890-=[];'\\,./`";
    static const char to[]   = "!@#$%^&*()_+{}:\"|<>?~";
    SDL_Keycode s;
    const char *p;
    int shift;

    if (!k)
        return 0;
    s = k->sym;
    shift = (k->mod & KMOD_SHIFT) != 0;
    if (s == SDLK_BACKSPACE) return 8;
    if (s == SDLK_RETURN || s == SDLK_KP_ENTER) return 13;
    if (s == SDLK_ESCAPE) return 27;
    if (s >= SDLK_a && s <= SDLK_z)
        return (Uint16)((shift ^ ((k->mod & KMOD_CAPS) != 0)) ? s - 32 : s);
    if (s >= 32 && s < 127) {
        if (shift && (p = strchr(from, (int)s)) != NULL)
            return (Uint16)(unsigned char)to[p - from];
        return (Uint16)s;
    }
    if (s >= SDLK_KP_1 && s <= SDLK_KP_9)
        return (Uint16)('1' + (s - SDLK_KP_1));
    if (s == SDLK_KP_0)
        return '0';
    return 0;
}
