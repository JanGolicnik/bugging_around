#include "src/base.c"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
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
    if (game.surface.h.valid) reni_release_surface(game.reni, game.surface);
    SDL_DestroyWindow(game.window);
}
