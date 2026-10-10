#ifndef FLOS_BASE
#define FLOS_BASE

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <marrow/marrow.h>

#define RENI_BACKEND_SDL3
#define RENI_IMPLEMENTATION
#include <reni/reni.h>

#define RIPPLE_IMPLEMENTATION
#define RIPPLE_BACKEND RIPPLE_RENI
#include <ripple/ripple.h>
#include <ripple/ripple_widgets.h>

#ifndef FLOS_RENDERER
#include "renderer.c"

#ifndef FLOS_GAME
#include "game.c"

#endif
#endif

#endif // FLOS_BASE
