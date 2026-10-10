#ifndef BUGGIN_BASE
#define BUGGIN_BASE

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

#ifndef BUGGIN_ENTITY
#include "entity.c"

#ifndef BUGGIN_SCENE
#include "scene.c"

#ifndef BUGGIN_DRAW
#include "draw.c"

#ifndef BUGGIN_GAME
#include "game.c"

#endif
#endif
#endif
#endif

#endif // BUGGIN_BASE
