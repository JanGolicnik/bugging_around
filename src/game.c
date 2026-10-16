#include "atlas.h"
#include "marrow/marrow.h"
#define BUGGIN_GAME
#include "base.c"

struct {
    bool in_background;

    struct {
        BumpAllocator _frame;
        Allocator* frame;
        Allocator* stable;
    } mem;

    f32 time;
    f32 dt;
    f32 dt_accum;
    u32 dt_n_samples;

    f32 avg_fps;

    VEKTOR(Scene*) scenes;
    Scene* scene;

    EntityHandle entity_guy;
    EntityHandle other_guy;
} game;

Scene* game_new_scene(void) {
    Scene* scene = mrw_alloc(game.mem.stable, Scene);
    vektor_add(game.scenes, scene);
    *scene = (Scene) { 0 };
    genarr_init(scene->entities, 123, game.mem.stable);
    return scene;
}

vec2s sprite_get_tag_pos(SpriteId id, SpriteTag tag)
{
    for (u32 i = 0; sprites[id].tags[i].tag; i++)
    {
        if (sprites[id].tags[i].tag == tag)
            return (vec2s){ .x = sprites[id].tags[i].x, .y = sprites[id].tags[i].y };
    }
    return (vec2s){ 0 };
}

bool game_init(void)
{
    game.mem.stable = nullptr;
    game.mem._frame = (BumpAllocator){ MRW_BUMP_IMPL };
    game.mem.frame = (Allocator*)&game.mem._frame;

    game.scene = game_new_scene();
    game.entity_guy = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("gospod 1"),
        .sprite.id = SPRITE_S1
    );
    EntityHandle parent = game.entity_guy;
    vec2s local_pos = sprite_get_tag_pos(SPRITE_S1, SpriteTag_attach);
    for (int i = 0; i < 10000; i++)
    {
        parent = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = mrw_format("stvor{}", game.mem.stable, i),
            .parent = parent,
            .local = { .pos = local_pos, .scale = 0.9f },
            .sprite.id = SPRITE_S1
        );
    }
    game.other_guy = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("gospod2"),
        .sprite.id = SPRITE_S2
    );

    return draw_init(game.mem.stable, game.mem.frame);
}

void game_update(void)
{
    f32 time = (f32)SDL_GetTicksNS() / 1000000000.0f;
    game.dt = time - game.time;
    game.dt_accum += game.dt;
    game.dt_n_samples += 1;
    if (game.dt_accum > 1.0f) {
        game.avg_fps = (f32)game.dt_n_samples / game.dt_accum;
        game.dt_accum -= 0.3f;
        game.dt_n_samples = 0;
        mrw_debug_val(game.avg_fps);
    }
    game.time = time;

    RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD), RECT( .color = RIPPLE_RGBA(0x2e2e2ebf), .radiusBR = .15f ) )
    {
        text(str("hello everynyan"));
        text(mrw_format("fps: {}", game.mem.frame, game.avg_fps));

        EntityIter s1_iter = { .include = CT_Sprite };
        while (scene_next_entity(game.scene, &s1_iter))
        {
            Entity* e = s1_iter.entity;
            if (e->sprite.id != SPRITE_S1) continue;
            e->local.rot = game.time;
        }

        Entity* guy = scene_get_entity(game.scene, game.entity_guy);
        guy->world.pos.x = draw.width * 0.5 + sin(game.time) * 100.0f;
        guy->world.pos.y = draw.height * 0.5;
        guy->world.rot = -game.time;
        entity_apply_world(guy);

        Entity* other_guy = scene_get_entity(game.scene, game.other_guy);
        other_guy->world.pos.x = draw.width * 0.5 + sin(game.time * 4.0f) * 100.0f;
        other_guy->world.pos.y = draw.height * 0.3;
        entity_apply_world(other_guy);
    }
}

void game_tick(void)
{
    if (game.in_background || !draw_is_ready()) return;

    mrw_bump_reset(&game.mem._frame);

    game_update();

    draw_scene(game.scene, game.time);
}

void game_destroy(void)
{
    return;
}

SDL_AppResult game_on_event(SDL_Event* event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            game.in_background = true;
            if (draw.surface.h.valid)
                reni_release_surface(draw.reni, draw.surface);
            draw.surface = (ReniSurface){ 0 };
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            game.in_background = false;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}
