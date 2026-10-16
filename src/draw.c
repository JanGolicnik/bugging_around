#define BUGGIN_DRAW
#include "base.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

cstr common_includes[] = {
    "./res/shaders/common.wgsl"
};

STRUCT(Instance)
{
    f32 x, y;
    f32 scale, rot;
    SpriteId sprite;
};

typedef GenarrHandle SpriteHandle;

struct {
    SDL_Window* window;
    i32 width, height;
    Reni* reni;
    ReniSurface surface;
    RippleContext ripple;

    VEKTOR(Instance) instances;
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

    struct {
        ReniTexture tex;
        ReniSampler sampler;
        ReniBuffer sprites_buffer;
        ReniBindingLayout layout;
        ReniBinding binding;
    } atlas;

    ReniShader shader;

    struct {
        Allocator* frame;
        Allocator* stable;
    } _mem;
} draw;

STRUCT(GPUSprite) {
    f32 uv0[2];
    f32 uv1[2];
    f32 size_px[2];
    f32 origin_px[2];
};

void draw_load_sprites(void)
{
    cstr path = "./res/atlas/atlas_0.png";
    u8Slice buf = read_file(path, draw._mem.frame);
    i32 w, h, c;
    stbi_uc* img = stbi_load_from_memory(buf.start, slice_size(buf), &w, &h, &c, 4);
    draw.atlas.tex = reni_create_texture(draw.reni, (ReniTextureConfig){
        .name = str(path),
        .width = w,
        .height = h,
        .format = ReniTextureFormat_RGBA8UnormSrgb,
        .usage = ReniTextureUsage_CopyDst | ReniTextureUsage_TextureBinding
    });
    reni_texture_write(draw.reni, draw.atlas.tex, (u8Slice)slice_to((u8*)img, w * h * c));
    stbi_image_free(img);

    draw.atlas.layout = reni_create_binding_layout(draw.reni, (ReniBindingLayoutConfig) {
        .entries[0] = {
            .visibility = ReniShaderStage_Fragment,
            .sampler.type = ReniSamplerType_Filtering,
        },
        .entries[1] = {
            .visibility = ReniShaderStage_Fragment,
            .texture.type = ReniSampleType_Float,
        },
        .entries[2] = {
            .visibility = ReniShaderStage_Vertex,
            .buffer.type = ReniBufferBindingType_ReadOnlyStorage,
        }
    });

    draw.atlas.sampler = reni_create_sampler(draw.reni, (ReniSamplerConfig){
        .name = str("atlas sampler"),
        .address = ReniSamplerAddressMode_ClampToEdge,
        .filter = ReniSamplerFilterMode_Linear
    });

    GPUSprite sprites_buffer[SPRITE_COUNT] = { 0 };
    for (u32 i = 0; i < SPRITE_COUNT; i++)
    {
        vec2s size = {{ sprites[i].w, sprites[i].h }};
        GPUSprite out = {
            .uv0 = { sprites[i].u0, sprites[i].v0 },
            .uv1 = { sprites[i].u1, sprites[i].v1 },
            .size_px = { size.x, size.y },
        };
        for (u32 j = 0; sprites[i].tags[j].tag != SpriteTag_None; j++)
        {
            if (sprites[i].tags[j].tag == SpriteTag_origin)
            {
                vec2s origin = vec2_sub(vec2_scale(size, 0.5f), (vec2s){{ sprites[i].tags[j].x, sprites[i].tags[j].y }});
                out.origin_px[0] = origin.x;
                out.origin_px[1] = origin.y;
            }
        }
        sprites_buffer[i] = out;
    }

    draw.atlas.sprites_buffer = reni_create_buffer(draw.reni, (ReniBufferConfig){
        .name = str("sprites buffer"),
        .usage = ReniBufferUsage_CopyDst | ReniBufferUsage_Storage,
        .data = slice_to((u8*)sprites_buffer, sizeof(sprites_buffer))
    });

    draw.atlas.binding = reni_create_binding(draw.reni, (ReniBindingConfig) {
        .name = sstr("atlas bind group"),
        .layout = draw.atlas.layout,
        .entries[0].sampler = draw.atlas.sampler,
        .entries[1].texture = draw.atlas.tex,
        .entries[2].buffer.buffer = draw.atlas.sprites_buffer,
    });
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
        .layouts = { draw.shader_data.layout, draw.atlas.layout },
        .vertex = {
            .entry = sstr("vs_main"),
            .buffers[0] = {
                .instance = true,
                .stride = sizeof(Instance),
                .attributes[0] = {
                    .location = 0,
                    .offset = offsetof(Instance, x),
                    .format = ReniVertexFormat_Float32x2,
                },
                .attributes[1] = {
                    .location = 1,
                    .offset = offsetof(Instance, scale),
                    .format = ReniVertexFormat_Float32,
                },
                .attributes[2] = {
                    .location = 2,
                    .offset = offsetof(Instance, rot),
                    .format = ReniVertexFormat_Float32,
                },
                .attributes[3] = {
                    .location = 3,
                    .offset = offsetof(Instance, sprite),
                    .format = ReniVertexFormat_Uint32,
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

    draw_load_sprites();
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
        while (scene_next_entity(scene, &iter))
            vektor_add(draw.instances, (Instance){
                .x = iter.entity->world.pos.x,
                .y = iter.entity->world.pos.y,
                .scale = iter.entity->world.scale,
                .rot = iter.entity->world.rot,
                .sprite = iter.entity->sprite
            });
        reni_buffer_write(draw.reni, draw.instance_buffer, slice_vektor_u8(draw.instances), 0);

        reni_renderpass_set_shader(draw.reni, pass, draw.shader);
        reni_renderpass_set_binding(draw.reni, pass, 0, draw.shader_data.binding);
        reni_renderpass_set_binding(draw.reni, pass, 1, draw.atlas.binding);
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
