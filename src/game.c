#include "SDL3/SDL_events.h"
#include "ripple/ripple_widgets.h"
#define BUGGIN_GAME
#include "base.c"

struct {
    bool in_background;

    struct {
        BumpAllocator _frame;
        Allocator* frame;
        Allocator* stable;
    } mem;

    u64 frame;
    f32 time;
    f32 dt;
    f32 dt_accum;
    u32 dt_n_samples;

    f32 avg_fps;

    VEKTOR(Scene*) scenes;
    Scene* scene;

    EntityHandle guy;
    EntityHandle dpad;
    EntityHandle dpad_bg;
    EntityHandle dpad_pad;
    SDL_FingerID dpad_finger;
} game;

struct {
    struct { SDL_FingerID id; f32 x, y; u64 down; } fingers[10];
} input;

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
    game.guy = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("gospod 1"),
        .sprite.id = SPRITE_S1
    );

    game.dpad = scene_create_entity(game.scene, CT_Transform, .name = str("dpad"));

    game.dpad_bg = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad_pad"),
        .parent = game.dpad,
        .sprite.id = SPRITE_DPAG_BG
    );

    game.dpad_pad = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad_pad"),
        .parent = game.dpad,
        .sprite.id = SPRITE_DPAD_PAD
    );

    return draw_init(game.mem.stable, game.mem.frame);
}

SDL_FingerID is_pressed_at(f32 x, f32 y, f32 radius, SDL_FingerID id)
{
    if (id)
    {
        array_for_each_i(input.fingers, i) {
            if (input.fingers[i].id == id && input.fingers[i].down)
            {
                return id;
            }
        }
        return 0;
    }

    array_for_each_i(input.fingers, i) {
        if (input.fingers[i].down == game.frame &&
            vec2_norm((vec2s){ .x = input.fingers[i].x - x, .y = input.fingers[i].y - y }) < radius)
        {
            return input.fingers[i].id;
        }
    }
    return 0;
}

bool is_down(SDL_FingerID id)
{
    array_for_each_i(input.fingers, i)
        if (input.fingers[i].id == id)
            return input.fingers[i].down != 0;
    return false;
}

vec2s finger_pos(SDL_FingerID id)
{
    array_for_each_i(input.fingers, i) {
        if (input.fingers[i].id == id)
        {
            return (vec2s){ .x = input.fingers[i].x, .y = input.fingers[i].y };
        }
    }
    return vec2_zero();
}

vec2s entity_sprite_size(Entity* e)
{
    return vec2_scale((vec2s){ .x = sprites[e->sprite.id].w, .y = sprites[e->sprite.id].h }, e->world.scale);
}

#define game_text(f, ...) text(mrw_format(f, game.mem.frame, __VA_ARGS__))

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

        Entity* dpad = scene_get_entity(game.scene, game.dpad);
        vec2s dpad_size = entity_sprite_size(dpad);
        dpad->world.pos.x = dpad_size.x + 100;
        dpad->world.pos.y = draw.height - dpad_size.y - 100;
        entity_apply_world(dpad);

        f32 dpad_radius = entity_sprite_size(scene_get_entity(game.scene, game.dpad_bg)).x * .5f;
        Entity* pad = scene_get_entity(game.scene, game.dpad_pad);
        if ((game.dpad_finger = is_pressed_at(dpad->world.pos.x, dpad->world.pos.y, dpad_radius, game.dpad_finger)))
        {
            pad->world.pos = finger_pos(game.dpad_finger);
            entity_apply_world(pad);
            if (vec2_norm(pad->local.pos) > dpad_radius)
            {
                pad->local.pos = vec2_scale(vec2_normalize(pad->local.pos), dpad_radius);
                entity_apply_local(pad);
            }
        }
        else
        {
            pad->local.pos = (vec2s){ 0 };
            entity_apply_local(pad);
        }

        vec2s dir = vec2_scale(pad->local.pos, 1.0f / dpad_radius);

        f32 speed  = 1000.0f;
        Entity* guy = scene_get_entity(game.scene, game.guy);
        guy->world.pos = vec2_add(guy->world.pos, vec2_scale(dir, speed * game.dt));
        entity_apply_world(guy);

        array_for_each_i(input.fingers, i)
        {
            if (input.fingers[i].down)
            {
                game_text("finger {} x: {} y: {}", input.fingers[i].id, input.fingers[i].x, input.fingers[i].y);
            }
        }
    }

    game.frame++;
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

void on_finger_event(u32 id, f32 x, f32 y, bool down, bool motion)
{
    array_for_each_i(input.fingers, i) {
        if ((down && !input.fingers[i].down) || input.fingers[i].id == id) {
            if (down) input.fingers[i].down = game.frame;
            else if (!motion) input.fingers[i].down = 0;
            input.fingers[i].id = id;
            input.fingers[i].x = x;
            input.fingers[i].y = y;
            break;
        }
    }
}

SDL_AppResult game_on_event(SDL_Event* e)
{
    switch (e->type) {
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

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
            on_finger_event(1, e->motion.x, e->motion.y, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN, e->type == SDL_EVENT_MOUSE_MOTION);
        } break;

        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: {
            i32 w, h; SDL_GetWindowSizeInPixels(draw.window, &w, &h);
            on_finger_event(e->tfinger.fingerID, e->tfinger.x * w, e->tfinger.y * h, e->type == SDL_EVENT_FINGER_DOWN, e->type == SDL_EVENT_FINGER_MOTION);
        } break;

        default:
            break;
    }
    return SDL_APP_CONTINUE;
}
