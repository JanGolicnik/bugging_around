#define FLOS_GAME
#include "base.c"

static struct {
    bool in_background;

    SDL_Window* window;
    Reni* reni;
    ReniSurface surface;
    RippleContext ripple;

    struct {
        BumpAllocator _frame;
        Allocator* frame;
        Allocator* stable;
    } mem;

    f32 prev_time;
    f32 dt;
    f32 dt_accum;
    u32 dt_n_samples;

    f32 avg_fps;
} game;

static void on_reni_init(Reni* reni, void* _)
{
    game.surface = reni_create_surface(reni, (ReniSurfaceConfig){ .window = game.window });
    game.ripple = ripple_initialize((RippleBackendRendererConfig){ .reni = reni });
    ripple_make_active_context(&game.ripple);
}

static void on_reni_error(str msg)
{
    mrw_abort("wgpu error: {}", msg);
}

bool game_init(void)
{
    game.mem.stable = nullptr;
    game.mem._frame = (BumpAllocator){ MRW_BUMP_IMPL };
    game.mem.frame = (Allocator*)&game.mem._frame;

    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifdef SDL_PLATFORM_APPLE
    flags |= SDL_WINDOW_METAL;
#endif
    game.window = SDL_CreateWindow("buggin around", 1280, 720, flags);
    if (!game.window) return false;

    game.reni = reni_create_reni((ReniConfig){
        .allocator = game.mem.stable,
        .frame_allocator = game.mem.frame,
        .init_callback = on_reni_init,
        .error_callback = on_reni_error,
    });

    return true;
}

SDL_AppResult game_on_event(SDL_Event* event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            game.in_background = true;
            if (game.surface.h.valid) reni_surface_suspend(game.reni, game.surface);
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            game.in_background = false;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

void game_update(void)
{
    f32 time = (f32)SDL_GetTicksNS() / 1000000000.0f;
    game.dt = time - game.prev_time;
    game.dt_accum += game.dt;
    game.dt_n_samples += 1;
    if (game.dt_accum > 1.0f) {
        game.avg_fps = (f32)game.dt_n_samples / game.dt_accum;
        game.dt_accum -= 0.3f;
        game.dt_n_samples = 0;
        mrw_debug_val(game.avg_fps);
    }
    game.prev_time = time;

    RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD), RECT( .color = RIPPLE_RGBA(0x2e2e2ebf), .radiusBR = .15f ) )
    {
        text(str("hello everynyan"));
        text(mrw_format("fps: {}", game.mem.frame, game.avg_fps));
    }
}

void game_render(void)
{
    i32 w, h;
    SDL_GetWindowSizeInPixels(game.window, &w, &h);
    ReniTexture target = reni_surface_acquire(game.reni, game.surface, (ReniSurfaceState){ .width = (u32)w, .height = (u32)h });
    if (!target.h.valid) return;

    reni_begin(game.reni);

    reni_submit_renderpass(game.reni, reni_create_renderpass(game.reni, (ReniRenderpassConfig){
        .name = str("clear"),
        .targets = { { .texture = target, .clear = true, .clear_value = { 0.1, 0.3, 0.6, 1.0 } } },
    }));

    ripple_submit(&game.ripple, w, h, (RippleRenderData) { .reni = game.reni, .surface = target });

    reni_end(game.reni);
}

void game_tick(void)
{
    if (game.in_background || !reni_is_ready(game.reni)) return;

    game_update();

    game_render();

    mrw_bump_reset(&game.mem._frame);
}

void game_quit(void)
{
    if (game.surface.h.valid) reni_release_surface(game.reni, game.surface);
    SDL_DestroyWindow(game.window);
}
