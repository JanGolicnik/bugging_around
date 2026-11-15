#define BUGGIN_DRAW
#include "base.c"

#define FONT_FIRST_CHAR 32
#define N_FONT_CHARS 96

cstr common_includes[] = {
    "./res/shaders/common.wgsl"
};

STRUCT(Instance)
{
    f32 x, y;
    f32 sx, sy;
    f32 rot;
    SpriteId sprite;
    f32 tint[4];
};

typedef GenarrHandle SpriteHandle;

STRUCT(Glyph)
{
    f32 uv0[2];
    f32 uv1[2];
    f32 size_px[2];
    f32 offset_px[2];
    f32 advance;
};

struct {
    SDL_Window* window;
    i32 width, height;
    Reni* reni;
    ReniSurface surface;

    VEKTOR(Instance) instances;
    ReniBuffer instance_buffer;

    f32 scaling;

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

    struct {
        Glyph glyphs[N_FONT_CHARS];
        f32 ascent, descent, line_gap;
    } text;

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
    u32 page;
    f32 _pad;
};

static void draw_bake_font(ReniTexture texture, u32 layer)
{
    u8Slice file = read_file("./res/roboto.ttf", draw._mem.frame);

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, file.start, stbtt_GetFontOffsetForIndex(file.start, 0)))
        mrw_abort("failed loading font");

    f32 scale = stbtt_ScaleForPixelHeight(&font, 64.0f);

    i32 ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
    draw.text.ascent = ascent * scale;
    draw.text.descent = descent * scale;
    draw.text.line_gap = line_gap * scale;

    u8* sdfs[N_FONT_CHARS] = { 0 };
    stbrp_rect rects[N_FONT_CHARS] = { 0 };
    i32 n_rects = 0;
    for (i32 i = 0; i < N_FONT_CHARS; i++)
    {
        i32 cp = FONT_FIRST_CHAR + i;
        Glyph* g = &draw.text.glyphs[i];
        *g = (Glyph){ 0 };

        i32 advance, lsb;
        stbtt_GetCodepointHMetrics(&font, cp, &advance, &lsb);
        g->advance = advance * scale;

        i32 w, h, ox, oy;
        sdfs[i] = stbtt_GetCodepointSDF(&font, scale, cp, 8, 128, (f32)128 / 8, &w, &h, &ox, &oy);
        if (!sdfs[i]) continue;

        g->size_px[0] = (f32)w;
        g->size_px[1] = (f32)h;
        g->offset_px[0] = (f32)ox;
        g->offset_px[1] = (f32)oy;

        rects[n_rects++] = (stbrp_rect){ .id = i, .w = w + 1, .h = h + 1 };
    }

    stbrp_node* nodes = mrw_alloc_n(draw._mem.frame, stbrp_node, ATLAS_SIZE);
    stbrp_context ctx;
    stbrp_init_target(&ctx, ATLAS_SIZE, ATLAS_SIZE, nodes, ATLAS_SIZE);
    if (!stbrp_pack_rects(&ctx, rects, n_rects))
        mrw_abort("font does not fit into atlas");

    usize layer_size = (usize)ATLAS_SIZE * ATLAS_SIZE * 4;
    u8* data = mrw_alloc_n(draw._mem.frame, u8, layer_size);
    memset(data, 0, layer_size);

    for (i32 r = 0; r < n_rects; r++)
    {
        const stbrp_rect* rect = &rects[r];
        Glyph* g = &draw.text.glyphs[rect->id];
        const u8* sdf = sdfs[rect->id];
        i32 w = (i32)g->size_px[0];
        i32 h = (i32)g->size_px[1];

        for (i32 y = 0; y < h; y++)
        {
            for (i32 x = 0; x < w; x++)
            {
                u8 d = sdf[y * w + x];
                u8* px = &data[((usize)(rect->y + y) * ATLAS_SIZE + rect->x + x) * 4];
                px[0] = px[1] = px[2] = px[3] = d;
            }
        }

        g->uv0[0] = (f32)rect->x / ATLAS_SIZE;
        g->uv0[1] = (f32)rect->y / ATLAS_SIZE;
        g->uv1[0] = (f32)(rect->x + w) / ATLAS_SIZE;
        g->uv1[1] = (f32)(rect->y + h) / ATLAS_SIZE;
    }

    for (i32 i = 0; i < N_FONT_CHARS; i++)
        stbtt_FreeSDF(sdfs[i], font.userdata);

    reni_texture_write_layer(draw.reni, texture, (u8Slice)slice_to(data, layer_size), layer);
}

void draw_load_sprites(void)
{
    draw.atlas.tex = reni_create_texture(draw.reni, (ReniTextureConfig){
        .name = str("atlas"),
        .width = ATLAS_SIZE,
        .height = ATLAS_SIZE,
        .layers = ATLAS_PAGE_COUNT + 1,
        .format = ReniTextureFormat_RGBA8Unorm,
        .usage = ReniTextureUsage_CopyDst | ReniTextureUsage_TextureBinding
    });

    for (i32 i = 0; i < ATLAS_PAGE_COUNT; i++)
    {
        str path = mrw_format(draw._mem.frame, "./res/atlas/atlas_{}.png\0", i);
        u8Slice buf = read_file(path.start, draw._mem.frame);
        i32 w, h, c;
        stbi_uc* img = stbi_load_from_memory(buf.start, slice_size(buf), &w, &h, &c, 4);
        reni_texture_write_layer(draw.reni, draw.atlas.tex, (u8Slice)slice_to((u8*)img, w * h * 4), i);
        stbi_image_free(img);
    }

    draw_bake_font(draw.atlas.tex, ATLAS_PAGE_COUNT);

    draw.atlas.layout = reni_create_binding_layout(draw.reni, (ReniBindingLayoutConfig) {
        .entries[0] = {
            .visibility = ReniShaderStage_Fragment,
            .sampler.type = ReniSamplerType_Filtering,
        },
        .entries[1] = {
            .visibility = ReniShaderStage_Fragment,
            .texture.type = ReniSampleType_Float,
            .texture.array = true
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

    GPUSprite sprites_buffer[SPRITE_COUNT + N_FONT_CHARS] = { 0 };
    array_for_each_i(sprites, i)
    {
        const SpriteInfo* s = &sprites[i];
        vec2s size = {{ s->w, s->h }};
        vec2s origin = vec2_sub(vec2_scale(size, 0.5f), (vec2s){{ s->ox, s->oy }});
        GPUSprite out = {
            .uv0 = { s->u0, s->v0 },
            .uv1 = { s->u1, s->v1 },
            .size_px = { size.x, size.y },
            .origin_px = { origin.x, origin.y },
            .page = s->page,
        };
        sprites_buffer[i] = out;
    }

    for (i32 i = 0; i < N_FONT_CHARS; i++)
    {
        const Glyph* g = &draw.text.glyphs[i];
        sprites_buffer[SPRITE_COUNT + i] = (GPUSprite){
            .uv0 = { g->uv0[0], g->uv1[1] },
            .uv1 = { g->uv1[0], g->uv0[1] },
            .size_px = { g->size_px[0], g->size_px[1] },
            .origin_px = {
                g->size_px[0] * 0.5f + g->offset_px[0],
                -(g->size_px[1] * 0.5f + g->offset_px[1])
            },
            .page = ATLAS_PAGE_COUNT,
        };
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
                    .offset = offsetof(Instance, sx),
                    .format = ReniVertexFormat_Float32x2,
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
                },
                .attributes[4] = {
                    .location = 4,
                    .offset = offsetof(Instance, tint),
                    .format = ReniVertexFormat_Float32x4,
                },
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

    draw.instance_buffer = reni_create_buffer(draw.reni, (ReniBufferConfig) {  .usage = ReniBufferUsage_CopyDst | ReniBufferUsage_Vertex });

    input.ripple = ripple_initialize(draw._mem.frame, draw._mem.stable, (RippleBackendRendererConfig){ .reni = draw.reni });
    ripple_make_active_context(&input.ripple);
}

static void on_reni_error(str msg)
{
    mrw_abort("wgpu error: {}", msg);
}

bool draw_init(Allocator* stable, Allocator* frame)
{
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifdef SDL_PLATFORM_APPLE
    flags |= SDL_WINDOW_METAL;
#endif
    draw._mem.frame = frame;
    draw._mem.stable = stable;

    draw.window = SDL_CreateWindow("buggin around", 1280, 720, flags);
    if (!draw.window)  mrw_abort("failed to create window");

    draw.reni = reni_create_reni((ReniConfig){
        .allocator = stable,
        .frame_allocator = frame,
        .init_callback = on_reni_init,
        .init_callback_data = nullptr,
        .error_callback = on_reni_error,
    });

    vektor_init(draw.instances, 1024, stable);

    draw.scaling = 1.0f;

    return true;
}

void draw_set_fullscreen(void)
{
    SDL_SetWindowFullscreen(draw.window, true);
}

void draw_set_size(u32 w, u32 h)
{
    SDL_SetWindowFullscreen(draw.window, false);
    SDL_SetWindowSize(draw.window, w, h);
}

f32 draw_width(void) { return draw.width * draw.scaling; }
f32 draw_height(void) { return draw.height * draw.scaling; }
vec2s draw_size(void) { return (vec2s){{ draw_width(), draw_height() }}; }

bool draw_is_char_drawable(u8 c)
{
    if (c < FONT_FIRST_CHAR || c >= FONT_FIRST_CHAR + N_FONT_CHARS) return false;
    return draw.text.glyphs[c - FONT_FIRST_CHAR].size_px[0] > 0.0f;
}

u32 draw_text_daw_len(str text)
{
    u32 n = 0;
    slice_for_each(text, c, char) n += draw_is_char_drawable((u8)*c);
    return n;
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

    draw.shader_data.data.width = (f32)draw.width * draw.scaling;
    draw.shader_data.data.height = (f32)draw.height * draw.scaling;
    draw.shader_data.data.time = time;
    reni_buffer_write(draw.reni, draw.shader_data.buffer, slice_u8_one(&draw.shader_data.data), 0);

    reni_begin(draw.reni);

    ReniRenderpass pass = reni_create_renderpass(draw.reni, (ReniRenderpassConfig){
        .name = str("sprite pass"),
        .targets[0] = {
            .texture = target,
            .clear = true,
            .clear_value = { 233.0f / 255.0f, 173.0f / 255.0f, 120.0f / 255.0f, 1.0 }
        },
    });

    { // draw sprites and shit
        vektor_clear(draw.instances);

        u32 n_sprites_per_layer[Layer_Count] = { 0 };
        {
            EntityIter iter = { .include = CT_Sprite };
            while (scene_next_entity(scene, &iter))
            {
                Entity* e = iter.entity;
                n_sprites_per_layer[e->sprite.layer] += e->sprite.text.start ? draw_text_daw_len(e->sprite.text) : 1;
            }
        }
        u32 layer_offset[Layer_Count] = { 0 };
        u32 offset = 0;
        for (u32 i = 0; i < Layer_Count; i++)
        {
            layer_offset[i] = offset;
            offset += n_sprites_per_layer[i];
        }
        buf_set(n_sprites_per_layer, 0, array_size(n_sprites_per_layer));
        vektor_ensure(draw.instances, offset);
        EntityIter iter = { .include = CT_Sprite };
        while (scene_next_entity(scene, &iter))
        {
            Entity* e = iter.entity;
            if (e->sprite.text.start)
            {
                f32 scale = e->sprite.text_size / 64.0f;
                f32 line_height = (draw.text.ascent - draw.text.descent + draw.text.line_gap) * scale;
                vec2s pen = {{ 0.0f, 0.0f }};

                slice_for_each(e->sprite.text, c, char)
                {
                    u8 ch = (u8)*c;
                    if (ch == '\n')
                    {
                        pen.x = 0.0f;
                        pen.y -= line_height;
                        continue;
                    }

                    if (ch < FONT_FIRST_CHAR || ch >= FONT_FIRST_CHAR + N_FONT_CHARS)
                        continue;

                    if (draw_is_char_drawable(ch))
                    {
                        vec2s p = entity_local_to_world(e, pen);
                        u32 i = layer_offset[e->sprite.layer] + n_sprites_per_layer[e->sprite.layer]++;
                        draw.instances.n_items++;
                        draw.instances.items[i] = (Instance){
                            .x = p.x, .y = p.y,
                            .sx = e->world.scale.x * scale,
                            .sy = e->world.scale.y * scale,
                            .rot = e->world.rot,
                            .sprite = (SpriteId)(SPRITE_COUNT + (ch - FONT_FIRST_CHAR))
                        };
                        memcpy(draw.instances.items[i].tint, (void*)&e->sprite.tint, sizeof(e->sprite.tint));
                    }
                    pen.x += draw.text.glyphs[ch - FONT_FIRST_CHAR].advance * scale;
                }
            }
            else
            {
                u32 i = layer_offset[e->sprite.layer] + n_sprites_per_layer[e->sprite.layer]++;
                draw.instances.n_items++;
                draw.instances.items[i] = (Instance) {
                    .x = e->world.pos.x,
                    .y = e->world.pos.y,
                    .sx = e->world.scale.x,
                    .sy = e->world.scale.y,
                    .rot = e->world.rot,
                    .sprite = e->sprite.id
                };
                memcpy(draw.instances.items[i].tint, (void*)&e->sprite.tint, sizeof(e->sprite.tint));
            }
        }
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
    ripple_submit(&input.ripple, draw.width, draw.height, (RippleRenderData) { .reni = draw.reni, .surface = target });
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
