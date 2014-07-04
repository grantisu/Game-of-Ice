#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "SDL.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PARR_SZ 1280
#define QARR_SZ 720

unsigned int _lame_val = 1001;
static inline int lame_rand(void) {
	_lame_val = _lame_val * 1664525 + 1013904223;
	return _lame_val >> 8;
}

typedef union {
        Uint32 c;
        struct {
                Uint8 a, r, g, b;
        };
} px_t;

SDL_Surface *init_display(void)
{
        SDL_Surface *screen;

        if( SDL_Init(SDL_INIT_VIDEO) == -1 ) {
                fprintf(stderr, "SDL init error: %s\n", SDL_GetError());
                exit(-1);
        }

        atexit(SDL_Quit);

        screen = SDL_SetVideoMode(PARR_SZ, QARR_SZ, 32, SDL_SWSURFACE | SDL_DOUBLEBUF);
        if( screen == NULL ) {
                fprintf(stderr, "SDL video error: %s\n", SDL_GetError());
                exit(-1);
        }

        return screen;
}

static inline void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
        ((Uint32 *)surface->pixels)[y * surface->w + x] = pixel;
}

static inline void addpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
        px_t ap = *((px_t *)&pixel);
        px_t *bp = &(((px_t *)surface->pixels)[y * surface->w + x]);

        bp->a = (255 - ap.a) < bp->a ? 255 : ap.a + bp->a;
        bp->r = (255 - ap.r) < bp->r ? 255 : ap.r + bp->r;
        bp->g = (255 - ap.g) < bp->g ? 255 : ap.g + bp->g;
        bp->b = (255 - ap.b) < bp->b ? 255 : ap.b + bp->b;
}

static inline void subpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
        px_t ap = *((px_t *)&pixel);
        px_t *bp = &(((px_t *)surface->pixels)[y * surface->w + x]);

        bp->a = ap.a > bp->a ? 0 : bp->a - ap.a;
        bp->r = ap.r > bp->r ? 0 : bp->r - ap.r;
        bp->g = ap.g > bp->g ? 0 : bp->g - ap.g;
        bp->b = ap.b > bp->b ? 0 : bp->b - ap.b;
}

Uint8 *new_gamestate(int a_sz, int b_sz)
{
        int i, j;
        Uint8 *a;


        a = malloc(a_sz * b_sz);
        if(a == NULL) {
                fprintf(stderr, "Can't init gamestate.\n");
                exit(-1);
        }


        for(j=0; j < b_sz; j++)
        for(i=0; i < a_sz; i++)
        {
                a[i + j*(a_sz)] = 0;
        }

        return a;
}

static inline void draw_state(SDL_Surface *s, Uint8 *a)
{
        int i, j;
	Uint32 red_color, white_color;

	red_color = SDL_MapRGB(s->format, 3, 2, 1);
	white_color = SDL_MapRGB(s->format, 255, 255, 255);

        for(j = 0; j < QARR_SZ; j++)
        for(i = 0; i < PARR_SZ; i++)
        {
                subpixel(s, i, j, red_color);
                addpixel(s, i, j, a[i + j*PARR_SZ] ? white_color : 0);
        }
}

void seed_gamestate(Uint8 *a)
{
        int i, j;

        for(j=0; j < QARR_SZ; j++)
        {
                a[j*PARR_SZ] = lame_rand() & 0x1;
                a[PARR_SZ + j*PARR_SZ - 1] = lame_rand() & 0x1;
        }

        for(i=0; i < PARR_SZ; i++)
        {
                a[i] = lame_rand() & 0x1;
                a[i + (QARR_SZ-1)*PARR_SZ] = lame_rand() & 0x1;
        }

}

void run_game(const Uint8 *restrict a, Uint8 *restrict b)
{
        int i, j, o, t;


        for(j=1; j < QARR_SZ-1; j++)
        for(i=1; i < PARR_SZ-1; i++)
        {
                o = i + j*PARR_SZ;

                // Edge cases are random noise, thus ignored
                t = 0;
                t += a[o - 1 - PARR_SZ];
                t += a[o     - PARR_SZ];
                t += a[o + 1 - PARR_SZ];
                t += a[o - 1];
                t += a[o + 1];
                t += a[o - 1 + PARR_SZ];
                t += a[o     + PARR_SZ];
                t += a[o + 1 + PARR_SZ];

                b[o] = a[o] * (t == 2 ? 1 : 0);
                b[o] |= (t == 3) ? 1 : 0;
        }

}

SDL_Surface *s_global;
SDL_Event event;
int skip_count = 0;
Uint8 *a_global, *b_global;

void game_iter(void) {
	SDL_PollEvent(&event);
	if(event.type == SDL_KEYDOWN \
	&& event.key.keysym.sym == SDLK_q) {
		SDL_Quit();
	}

	seed_gamestate(a_global);
	run_game(a_global, b_global);
	seed_gamestate(b_global);
	run_game(b_global, a_global);

	skip_count++;

	SDL_LockSurface(s_global);
	if(skip_count % 2 == 0)
	{
		draw_state(s_global, a_global);
		SDL_Flip(s_global);
		skip_count = 0;
	} else if(skip_count % 1 == 0) {
		draw_state(s_global, b_global);
		SDL_Flip(s_global);
	}
	SDL_UnlockSurface(s_global);
}

int main(int argc, char **argv)
{
        s_global = init_display();
        SDL_FillRect(s_global, NULL, 0);

        a_global = new_gamestate(PARR_SZ, QARR_SZ);
        b_global = new_gamestate(PARR_SZ, QARR_SZ);

#ifdef EMSCRIPTEN
	emscripten_set_main_loop(game_iter, 0, 1);
#else
        while(1) { game_iter(); }
#endif

        free(a_global);
        free(b_global);

        return 0;
}

