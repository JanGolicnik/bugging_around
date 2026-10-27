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

#define CGLM_OMIT_NS_FROM_STRUCT_API
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct.h>

#define VEC2_ONE  GLMS_VEC2_ONE
#define VEC2_ZERO GLMS_VEC2_ZERO

static u8Slice read_file(cstr path, Allocator* a) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END); long n = ftell(f);
    rewind(f); u8* buf = mrw_alloc_n(a, u8, n + 1);
    fread(buf, 1, (size_t)n, f);
    fclose(f); buf[n] = 0;
    return (u8Slice)slice_to(buf, n);
}

#include "atlas.h"

#ifndef BUGGIN_TIME
#include "time.c"

#ifndef BUGGIN_INPUT
#include "input.c"

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
#endif
#endif

#endif // BUGGIN_BASE
