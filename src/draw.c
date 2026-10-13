#include "atlas.h"
#include "marrow/vektor.h"
#include "reni/reni.h"
#define BUGGIN_DRAW
#include "base.c"

cstr common_includes[] = {
    "./res/shaders/common.wgsl"
};

STRUCT(Sprite)
{
    u32 index;
};

typedef GenarrHandle SpriteHandle;

struct {
    SDL_Window* window;
    i32 width, height;
    Reni* reni;
    ReniSurface surface;
    RippleContext ripple;

    GENARR(Sprite) sprites;
    VEKTOR(TransformC) instances;
    ReniBuffer instance_buffer;

    struct {
        ReniBindingLayout layout;
        ReniBinding binding;
        ReniBuffer buffer;
        struct {
            f32 width, height;
            f32 time;
        } data;
    } shader_data;

    ReniShader shader;

    struct {
        Allocator* frame;
        Allocator* stable;
    } _mem;
} draw;

SpriteHandle create_sprite(str path)
{
    mrw_unused sprites[SPRITE_S1].x;
    return (SpriteHandle){ 0 };
}

void draw_init_shaders(void)
{
    draw.shader_data.layout = reni_create_binding_layout(draw.reni, (ReniBindingLayoutConfig) { .entries[0] = {
        .visibility = ReniShaderStage_Vertex | ReniShaderStage_Fragment,
        .buffer.type = ReniBufferBindingType_Uniform,
    }});

    draw.shader_data.buffer = reni_create_buffer(draw.reni, (ReniBufferConfig){
        .name = sstr("shader data buffer"),
        .usage = ReniBufferUsage_CopyDst | ReniBufferUsage_Uniform
    });

    draw.shader_data.binding = reni_create_binding(draw.reni, (ReniBindingConfig) {
        .name = sstr("shader data"),
        .layout = draw.shader_data.layout,
        .entries[0].buffer.buffer = draw.shader_data.buffer
    });

    draw.shader = reni_create_shader(draw.reni, (ReniShaderConfig){
        .name = sstr("sprite shader"),
        .source.file = {
            .path = "./res/shaders/sprite.wgsl",
            .includes = array_slice(common_includes)
        },
        .layouts[0] = draw.shader_data.layout,
        .vertex = {
            .entry = sstr("vs_main"),
            .buffers[0] = {
                .instance = true,
                .stride = sizeof(TransformC),
                .attributes[0] = {
                    .location = 0,
                    .offset = offsetof(TransformC, x),
                    .format = ReniVertexFormat_Float32x2,
                },
                .attributes[1] = {
                    .location = 1,
                    .offset = offsetof(TransformC, w),
                    .format = ReniVertexFormat_Float32x2,
                }
            },
        },
        .fragment = {
            .entry = sstr("fs_main"),
            .targets[0] = {
                .format = reni_surface_get_format(draw.reni, draw.surface),
                .blend_state = {
                    .color = RENI_BLEND_STATE_BLEND,
                    .alpha = RENI_BLEND_STATE_OVERWRITE
                }
            }
        }
    });
}

static void on_reni_init(Reni* reni, void* _)
{
    draw.reni = reni;
    draw.surface = reni_create_surface(draw.reni, (ReniSurfaceConfig){ .window = draw.window });

    draw_init_shaders();

    draw.instance_buffer = reni_create_buffer(draw.reni, (ReniBufferConfig) {  .usage = ReniBufferUsage_CopyDst | ReniBufferUsage_Vertex }),

    draw.ripple = ripple_initialize(draw._mem.frame, draw._mem.stable, (RippleBackendRendererConfig){ .reni = draw.reni });
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
    if (!draw.window)  mrw_abort("failed to create window");

    draw.reni = reni_create_reni((ReniConfig){
        .allocator = stable,
        .frame_allocator = frame,
        .init_callback = on_reni_init,
        .init_callback_data = nullptr,
        .error_callback = on_reni_error,
    });

    vektor_init(draw.instances, 10, stable);

    return true;
}

void draw_scene(Scene* scene, f32 time)
{
    if (!draw.surface.h.valid)
    {
        draw.surface = reni_create_surface(draw.reni, (ReniSurfaceConfig){ .window = draw.window });
    }

    SDL_GetWindowSizeInPixels(draw.window, &draw.width, &draw.height);
    ReniTexture target = reni_surface_acquire(draw.reni, draw.surface, (ReniSurfaceState){ .width = (u32)draw.width, .height = (u32)draw.height });
    if (!target.h.valid) return;

    draw.shader_data.data.width = (f32)draw.width;
    draw.shader_data.data.height = (f32)draw.height;
    draw.shader_data.data.time = time;
    reni_buffer_write(draw.reni, draw.shader_data.buffer, slice_u8_one(&draw.shader_data.data), 0);

    reni_begin(draw.reni);

    ReniRenderpass pass = reni_create_renderpass(draw.reni, (ReniRenderpassConfig){
        .name = str("sprite pass"),
        .targets[0] = {
            .texture = target,
            .clear = true,
            .clear_value = { 0.1, 0.3, 0.6, 1.0 }
        },
    });

    { // draw sprites and shit
        vektor_clear(draw.instances);

        EntityIter iter = { .include = CT_Sprite };
        while (scene_next_entity(scene, &iter)) vektor_add(draw.instances, iter.entity->transform);
        reni_buffer_write(draw.reni, draw.instance_buffer, slice_vektor_u8(draw.instances), 0);

        reni_renderpass_set_shader(draw.reni, pass, draw.shader);
        reni_renderpass_set_binding(draw.reni, pass, 0, draw.shader_data.binding);
        reni_renderpass_draw(draw.reni, pass, (ReniDrawConfig) {
            .n_vertices = 6,
            .n_instances = draw.instances.n_items,
            .vertices = draw.instance_buffer
        });
    }

    reni_submit_renderpass(draw.reni, pass);
    ripple_submit(&draw.ripple, draw.width, draw.height, (RippleRenderData) { .reni = draw.reni, .surface = target });
    reni_end(draw.reni);
}

bool draw_is_ready(void)
{
    return draw.reni && reni_is_ready(draw.reni);
}

void draw_destroy(void)
{
    if (draw.surface.h.valid) reni_release_surface(draw.reni, draw.surface);
    SDL_DestroyWindow(draw.window);
}
