#include <omp.h>
#include "SDL.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PARR_SZ 1280
#define QARR_SZ 720

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

        SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
        SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
        SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
        SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
        SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
        SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
        SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);

        return screen;
}

inline void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
        ((Uint32 *)surface->pixels)[y * surface->w + x] = pixel;
}

inline void addpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
        px_t ap = *((px_t *)&pixel);
        px_t *bp = &(((px_t *)surface->pixels)[y * surface->w + x]);

        bp->a = (255 - ap.a) < bp->a ? 255 : ap.a + bp->a;
        bp->r = (255 - ap.r) < bp->r ? 255 : ap.r + bp->r;
        bp->g = (255 - ap.g) < bp->g ? 255 : ap.g + bp->g;
        bp->b = (255 - ap.b) < bp->b ? 255 : ap.b + bp->b;
}

inline void subpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
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

inline void draw_state(SDL_Surface *s, Uint8 *a)
{
        int i, j;

        #pragma omp for 
        for(j = 0; j < QARR_SZ; j++)
        for(i = 0; i < PARR_SZ; i++)
        {
                subpixel(s, i, j, 0x030201);
                addpixel(s, i, j, a[i + j*PARR_SZ] ? 0xffffff : 0);
        }
}

void seed_gamestate(Uint8 *a)
{
        int i, j;

        #pragma omp for 
        for(j=0; j < QARR_SZ; j++)
        {
                a[j*PARR_SZ] = lrand48() & 0x1;
                a[PARR_SZ + j*PARR_SZ - 1] = lrand48() & 0x1;
        }

        #pragma omp for 
        for(i=0; i < PARR_SZ; i++)
        {
                a[i] = lrand48() & 0x1;
                a[i + (QARR_SZ-1)*PARR_SZ] = lrand48() & 0x1;
        }

}

void run_game(const Uint8 *restrict a, Uint8 *restrict b)
{
        int i, j, o, t;


        #pragma omp for 
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

int main(int argc, char **argv)
{
        SDL_Surface *s;
        SDL_Event event;
        int skip_count = 0;

        Uint8 *a, *b;

        srand48(0);
        s = init_display();
        SDL_FillRect(s, NULL, 0);

        a = new_gamestate(PARR_SZ, QARR_SZ);
        b = new_gamestate(PARR_SZ, QARR_SZ);

        while(1)
        {       SDL_PollEvent(&event);
                if(event.type == SDL_KEYDOWN \
                        && event.key.keysym.sym == SDLK_q) {
                        break;
                }

                #pragma omp parallel shared(a, b)
                {
                        seed_gamestate(a);
                        run_game(a, b);
                        seed_gamestate(b);
                        run_game(b, a);
                }

                skip_count++;

                if(skip_count % 2 == 0)
                {
                        draw_state(s, a);
                        SDL_Flip(s);
                        skip_count = 0;
                } else if(skip_count % 1 == 0) {
                        draw_state(s, b);
                        SDL_Flip(s);
                }

        }

        free(a);
        free(b);

        return 0;
}

