Game of Ice
===========

This is a minor twist on [Conway's Game of Life](
http://en.wikipedia.org/wiki/Conway%27s_Game_of_Life ): entropy is injected at
the edges of the board, and a blue fade effect is used when updating the
screen.

Here's what it looks like in action:
[http://www.youtube.com/watch?v=qOjuTSzlkag](
http://www.youtube.com/watch?v=qOjuTSzlkag )

Or view an [emscripted]( https://github.com/kripken/emscripten ) version here:
<http://gameofice.gmathews.com>

**WARNING**: you're currently on the expiremental emscripten branch, which
*will* be be rebased.

If you want to compile Game of Ice for your browser, checkout [Emscripten](
https://github.com/kripken/emscripten ) and try this command:
`$PATH_TO_EMSCRIPTEN/emcc game_of_life.c -O2 -o Game_of_Life.html`

