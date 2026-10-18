#include <unistd.h>
#include "src/base.c"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    const char* base = SDL_GetBasePath();
    if (base && chdir(base) != 0) mrw_debug("change base dir failed");

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    bool success = true;
    if (success) success |= game_init();
    return success ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* e)
{
    return game_on_event(e);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    game_tick();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    draw_destroy();
    game_destroy();
}
