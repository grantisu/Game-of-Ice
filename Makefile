SDL_CFLAGS=`sdl-config --cflags`
SDL_LIBS=`sdl-config --libs`

CFLAGS=${SDL_CFLAGS} -std=c99 -march=native -O3 -fms-extensions
LIBS=${SDL_LIBS}

all: game_of_ice

game_of_ice: game_of_ice.c
	${CC} $^ -o $@ ${CFLAGS} ${LIBS}

