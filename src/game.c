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

    VEKTOR(Scene*) scenes;
    Scene* scene;

    EntityHandle anchors[9];

    EntityHandle guy;

    struct
    {
        EntityHandle root;
        EntityHandle bg;
        EntityHandle pad;
        SDL_FingerID finger;
    } dpad;
} game;

Scene* game_new_scene(void) {
    Scene* scene = mrw_alloc(game.mem.stable, Scene);
    vektor_add(game.scenes, scene);
    *scene = (Scene) { 0 };
    genarr_init(scene->entities, 123, game.mem.stable);
    scene->root = scene_create_entity(scene, 0, .name = str("root"));
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

#define XAnchors \
X(Anchor_lt, -0.5f, 0.5f),  X(Anchor_mt , 0.0f, 0.5f),  X(Anchor_rt , 0.5f, 0.5f),\
X(Anchor_lm, -0.5f, 0.0f),  X(Anchor_mm , 0.0f, 0.0f),  X(Anchor_rm , 0.5f, 0.0f),\
X(Anchor_lb, -0.5f, -0.5f),  X(Anchor_mb , 0.0f, -0.5f),  X(Anchor_rb , 0.5f, -0.5f),

#define X(name, ...) {{ __VA_ARGS__ }}
vec2s _anchors[9] = { XAnchors };
#undef X

#define X(name, ...) sstr(#name)
str _anchor_names[9] = { XAnchors };
#undef X

#define X(name, ...) name
typedef enum { XAnchors } Anchor;
#undef X

bool game_init(void)
{
    game.mem.stable = nullptr;
    game.mem._frame = (BumpAllocator){ MRW_BUMP_IMPL };
    game.mem.frame = (Allocator*)&game.mem._frame;

    game.scene = game_new_scene();

    array_for_each_i(_anchors, i) {
        game.anchors[i] = scene_create_entity(game.scene, CT_Transform, .name = _anchor_names[i]);
    }

    game.guy = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("gospod 1"),
        .sprite.id = SPRITE_S1,
    );

    game.dpad.root = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad"),
        .parent = game.anchors[Anchor_lb],
        .local.pos = vec2_adds(sprite_size(SPRITE_DPAG_BG), 100.0f),
    );

    game.dpad.bg = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad_bg"),
        .parent = game.dpad.root,
        .sprite.id = SPRITE_DPAG_BG
    );
    game.dpad.pad = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad_pad"),
        .parent = game.dpad.root,
        .sprite.id = SPRITE_DPAD_PAD
    );

    return draw_init(game.mem.stable, game.mem.frame);
}

#define game_text(...) text(mrw_format(game.mem.frame, __VA_ARGS__))

void game_update_ui(void)
{
    f32 dpad_radius = entity_sprite_size(scene_get_entity(game.scene, game.dpad.bg)).x * .5f;
    Entity* pad = scene_get_entity(game.scene, game.dpad.pad);
    if ((game.dpad.finger = is_finger_down_at(pad->world.pos.x, pad->world.pos.y, dpad_radius, game.dpad.finger)))
    {
        pad->world.pos = finger_pos(game.dpad.finger);
        entity_apply_world(pad);
        if (vec2_norm(pad->local.pos) > dpad_radius)
        {
            pad->local.pos = vec2_scale(vec2_normalize(pad->local.pos), dpad_radius);
            entity_apply_local(pad);
        }
    }
    else
    {
        pad->local.pos = VEC2_ZERO;
        entity_apply_local(pad);
    }

    input.dpad_dir = vec2_normalize(pad->local.pos);
}

void game_update(void)
{
    game_text("hello everynyan");
    game_text("fps: {}", time.avg_fps);

    array_for_each_i(input.fingers, i)
        if (input.fingers[i].down)
            game_text("finger {} x: {} y: {}", input.fingers[i].id, input.fingers[i].x, input.fingers[i].y);

    game_update_ui();

    f32 speed  = 1000.0f;
    Entity* guy = scene_get_entity(game.scene, game.guy);
    guy->world.pos = vec2_add(guy->world.pos, vec2_scale(input.dpad_dir, speed * time.dt));
    entity_apply_world(guy);
}

cstr indents[] = { "   ", "        ", "            ", "                ", "                    ", "                        ", "                                ", "                                    ", "                                    " };
void game_print_tree(Entity* e, u32 indent)
{
    printout("{}{}", indents[indent], e->name);
    if (entity_has(e, CT_Transform))
    {
        printout("(x:{.1f} y:{.1f} w:{.1f} h:{.1f}\n", e->world.pos.x, e->world.pos.y, e->world.scale.x, e->world.scale.y);
    }

    for_each_entity_children(e, child)
    {
        game_print_tree(child, indent + 1);
    }
}

void game_tick(void)
{
    if (game.in_background || !draw_is_ready()) return;

    mrw_bump_reset(&game.mem._frame);

    time_update();

    RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD), RECT( .color = RIPPLE_RGBA(0x2e2e2ebf), .radiusBR = .15f ) )
    {
        game_update();
    }

    // game_print_tree(scene_get_entity(game.scene, game.scene->root), 0);

    draw_scene(game.scene, time.time);

    time.tick++;
}

void game_destroy(void)
{
    return;
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
            i32 w, h; SDL_GetWindowSizeInPixels(draw.window, &w, &h);
            input_on_finger(1, e->motion.x - w * 0.5f, h * 0.5f - e->motion.y, e->type == SDL_EVENT_MOUSE_BUTTON_DOWN, e->type == SDL_EVENT_MOUSE_MOTION);
        } break;

        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: {
            i32 w, h; SDL_GetWindowSizeInPixels(draw.window, &w, &h);
            input_on_finger(e->tfinger.fingerID, (e->tfinger.x - 0.5f) * w, (e->tfinger.y - 0.5f) * h, e->type == SDL_EVENT_FINGER_DOWN, e->type == SDL_EVENT_FINGER_MOTION);
        } break;

        case SDL_EVENT_WINDOW_RESIZED:
            array_for_each_i(game.anchors, i)
            {
                Entity* anchor = scene_get_entity(game.scene, game.anchors[i]);
                anchor->world.pos = (vec2s){{ (float)e->window.data1 * _anchors[i].x, (float)e->window.data2 * _anchors[i].y }};
                entity_apply_world(anchor);
            }
            break;

        default:
            break;
    }
    return SDL_APP_CONTINUE;
}
