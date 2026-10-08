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

static struct {
    SDL_Window* window;
    Reni* reni;
    ReniSurface surface;
    bool paused;

    BumpAllocator _frame;
    Allocator* frame;
    Allocator* stable;

    bool ripple_initialized;
    RippleContext ripple_context;

    f32 prev_time;
    f32 dt;
    f32 dt_accum;
    f32 avg_fps;
    u32 dt_n_samples;
} g;

static void on_reni_init(Reni* reni, void* _)
{
}

static void on_reni_error(str msg)
{
    mrw_abort("wgpu error: {}", msg);
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    g.stable = nullptr;
    g._frame = (BumpAllocator){ MRW_BUMP_IMPL };
    g.frame = (Allocator*)&g._frame;

    if (!SDL_Init(SDL_INIT_VIDEO)) return SDL_APP_FAILURE;

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifdef SDL_PLATFORM_APPLE
    flags |= SDL_WINDOW_METAL;
#endif
    g.window = SDL_CreateWindow("buggin around", 1280, 720, flags);
    if (!g.window) return SDL_APP_FAILURE;

    g.reni = reni_create_reni((ReniConfig){
        .allocator = g.stable,
        .frame_allocator = g.frame,
        .init_callback = on_reni_init,
        .error_callback = on_reni_error,
    });

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* e)
{
    switch (e->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            g.paused = true;
            if (g.surface.h.valid) reni_surface_suspend(g.reni, g.surface);
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            g.paused = false;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    if (g.paused || !reni_is_ready(g.reni)) return SDL_APP_CONTINUE;

    if (!g.surface.h.valid)
    {
        g.surface = reni_create_surface(g.reni, (ReniSurfaceConfig){ .window = g.window });

        if (!g.ripple_initialized)
        {
            g.ripple_initialized = true;
            g.ripple_context = ripple_initialize((RippleBackendRendererConfig){
                .reni = g.reni,
                .surface_format = reni_surface_get_format(g.reni, g.surface)
            });
            ripple_make_active_context(&g.ripple_context);
        }
    }

    int w, h;
    SDL_GetWindowSizeInPixels(g.window, &w, &h);
    reni_surface_update(g.reni, g.surface, (ReniSurfaceState){ .width = (u32)w, .height = (u32)h });

    f32 time = (f32)SDL_GetTicksNS() / 1000000000.0f;
    g.dt = time - g.prev_time;
    g.dt_accum += g.dt;
    g.dt_n_samples += 1;
    if (g.dt_accum > 1.0f) {
        g.avg_fps = (f32)g.dt_n_samples / g.dt_accum;
        g.dt_accum -= 0.3f;
        g.dt_n_samples = 0;
        mrw_debug_val(g.avg_fps);
    }
    g.prev_time = time;

    RIPPLE(FORM(.width = PERCENT(1.0f, SVT_RELATIVE_CHILD), .height = PERCENT(1.0f, SVT_RELATIVE_CHILD)),
        RECTANGLE(.color = RIPPLE_RGBA(0x2e2e2ebf), .radiusBR = .15f))
    {
        text(str("hello everynyan"));
        text(mrw_format("fps: {}", g.frame, g.avg_fps));
    }

    reni_begin(g.reni);

    ReniTexture target = reni_surface_acquire(g.reni, g.surface);
    if (target.h.valid) {
        ReniRenderpass pass = reni_create_renderpass(g.reni, (ReniRenderpassConfig){
            .name = str("clear"),
            .targets = { { .texture = target, .clear = true, .clear_value = { 0.1, 0.3, 0.6, 1.0 } } },
        });
        reni_submit_renderpass(g.reni, pass);
    }

    ripple_submit(&g.ripple_context, w, h, (RippleRenderData) { .reni = g.reni, .surface = target });

    reni_end(g.reni);

    mrw_bump_reset(&g._frame);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    if (g.surface.h.valid) reni_release_surface(g.reni, g.surface);
    SDL_DestroyWindow(g.window);
}
