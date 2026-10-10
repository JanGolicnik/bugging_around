#define BUGGIN_DRAW
#include "base.c"

struct {
    SDL_Window* window;
    Reni* reni;
    ReniSurface surface;
    RippleContext ripple;
} draw;

static void on_reni_init(Reni* reni, void* _)
{
    draw.ripple = ripple_initialize((RippleBackendRendererConfig){ .reni = reni });
    ripple_make_active_context(&draw.ripple);
}

static void on_reni_error(str msg)
{
    mrw_abort("wgpu error: {}", msg);
}

bool draw_init(Allocator* stable, Allocator* frame)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifdef SDL_PLATFORM_APPLE
    flags |= SDL_WINDOW_METAL;
#endif
    draw.window = SDL_CreateWindow("buggin around", 1280, 720, flags);
    if (!draw.window) return false;

    draw.reni = reni_create_reni((ReniConfig){
        .allocator = stable,
        .frame_allocator = frame,
        .init_callback = on_reni_init,
        .error_callback = on_reni_error,
    });

    return true;
}

void draw_scene(Scene* scene)
{
    if (!draw.surface.h.valid)
    {
        draw.surface = reni_create_surface(draw.reni, (ReniSurfaceConfig){ .window = draw.window });
    }

    i32 w, h;
    SDL_GetWindowSizeInPixels(draw.window, &w, &h);
    ReniTexture target = reni_surface_acquire(draw.reni, draw.surface, (ReniSurfaceState){ .width = (u32)w, .height = (u32)h });
    if (!target.h.valid) return;

    reni_begin(draw.reni);
    ReniRenderpass pass = reni_create_renderpass(draw.reni, (ReniRenderpassConfig){
        .name = str("clear"),
        .targets = { { .texture = target, .clear = true, .clear_value = { 0.1, 0.3, 0.6, 1.0 } } },
    });

    mrw_unused scene;

    reni_submit_renderpass(draw.reni, pass);

    ripple_submit(&draw.ripple, w, h, (RippleRenderData) { .reni = draw.reni, .surface = target });
    reni_end(draw.reni);
}

bool draw_is_ready(void)
{
    return reni_is_ready(draw.reni);
}

void draw_destroy(void)
{
    if (draw.surface.h.valid) reni_release_surface(draw.reni, draw.surface);
    SDL_DestroyWindow(draw.window);
}
