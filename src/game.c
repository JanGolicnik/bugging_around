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

    EntityHandle stinkbug;
    EntityHandle ladybug;
    EntityHandle centipede;

    struct
    {
        EntityHandle root;
        EntityHandle bg;
        EntityHandle pad;
        FingerId finger;
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
    const SpriteInfo* s = &sprites[id];
    for (u32 i = 0; s->n_tags; i++)
    {
        const SpriteTagInfo* t = &sprite_tags[s->tags_i + i];
        if (t->tag == tag) return (vec2s){{ t->x, t->y }};
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

void game_update_turrets(void)
{
    EntityIter iter = { .include = CT_Turret };
    while (scene_next_entity(game.scene, &iter))
    {
        Entity* e = iter.entity;
        FingerIter finger_iter = { 0 };
        while (genarr_next_valid(input.fingers, &finger_iter))
        {
            Finger* f = finger_iter.f;
            vec2s dir = vec2_normalize(vec2_sub((vec2s){{ f->x, f->y }}, e->world.pos));
            e->world.rot = atan2(dir.y, dir.x);
            entity_apply_world(e);
            break;
        }
    }
}

void game_update_bugs(void)
{
    static FingerId f = { 0 };
    static EntityHandle held = { 0 };
    EntityIter iter = { .include = CT_Bug };
    while (scene_next_entity(game.scene, &iter))
    {
        Entity* e = iter.entity;
        Entity* sprite = entity_first_child_with(e, CT_Sprite);

        if (!held.valid && is_finger_down_at(e->world.pos.x, e->world.pos.y, 100.0f, &f))
        {
            finger_consume(f);
            held = iter.handle;
        }

        if (is_finger_up(f))
        {
            held = (EntityHandle){ 0 };
        }

        if (held.i == iter.handle.i)
        {
            e->bug.target = finger_pos(f);
        }

        if (vec2_distance(e->bug.target, e->world.pos) < 200.0f)
        {
            sprite->local.rot += -sprite->local.rot * (1.0f - exp(-10.0f * time.dt));
            entity_apply_local(sprite);

            if (e->bug.next_target_at < time.time)
            {
                e->bug.target = (vec2s){{ mrw_random_f32(-0.4f, 0.4f) * draw.width, mrw_random_f32(-0.3f, 0.3f) * draw.height }};
                e->bug.next_target_at = time.time + mrw_random_f32(1.0f, 5.0f);
            }
        }
        else
        {
            vec2s dir = vec2_normalize(vec2_sub(e->bug.target, e->world.pos));
            e->world.rot = atan2(dir.y, dir.x);
            e->world.pos = vec2_add(e->world.pos, vec2_scale(dir, 200.0f * time.dt));
            entity_apply_world(e);

            sprite->local.rot = sin(time.time * 10.0f) * 0.1;
            entity_apply_local(sprite);

            EntityIter iter = { .include = CT_Leg };
            while (scene_next_entity(game.scene, &iter))
            {
                iter.entity->local.rot = sin(time.time * 30.0f) * 0.1f;
                entity_apply_local(iter.entity);
            }
        }
    }
}

bool game_init(void)
{
    game.mem.stable = nullptr;
    game.mem._frame = (BumpAllocator){ MRW_BUMP_IMPL };
    game.mem.frame = (Allocator*)&game.mem._frame;

    game.scene = game_new_scene();

    array_for_each_i(_anchors, i) {
        game.anchors[i] = scene_create_entity(game.scene, CT_Transform, .name = _anchor_names[i]);
    }

    { // stinkbug
        game.stinkbug = scene_create_entity(game.scene, CT_Transform | CT_Bug, .name = str("stinkbug"));
        EntityHandle body = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
            .parent = game.stinkbug,
            .sprite.id = SPRITE_STINKBUG,
            .sprite.layer = Layer_BugsBodies
        );
        for (u32 i = 0; i < sprites[SPRITE_STINKBUG].n_tags; i++)
        {
            const SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_STINKBUG].tags_i + i];
            if (t->tag == SpriteTag_leg_left || t->tag == SpriteTag_leg_right)
            {
                EntityHandle joint = scene_create_entity(game.scene, CT_Transform, .name = str("leg_joint"),
                    .parent = body,
                    .local.pos = (vec2s){{ t->x, t->y }},
                    .local.rot = t->rot,
                    .local.scale = (vec2s){{ t->tag == SpriteTag_leg_left ? -1.0f : 1.0f, 1.0f }}
                );
                scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Leg, .name = str("leg"),
                    .parent = joint,
                    .sprite.id = SPRITE_STINKBUG_LEG,
                    .sprite.layer = Layer_BugsLegs,
                );
            }
            else if (t->tag == SpriteTag_turret)
            {
                EntityHandle turret = scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Turret, .name = str("cannon !!"),
                    .parent = body,
                    .local.pos = (vec2s){{ t-> x, t->y }},
                    .local.rot = t->rot,
                    .sprite.id = SPRITE_CANNON1,
                    .sprite.layer = Layer_Turrets,
                );
                SpriteTagInfo* t = sprite_get_tag(SPRITE_CANNON1, SpriteTag_turret);
                scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("barrel"),
                    .parent = turret,
                    .local.pos = (vec2s){{ t-> x, t->y }},
                    .local.rot = t->rot,
                    .sprite.id = SPRITE_CANNON1_BARREL,
                    .sprite.layer = Layer_Barrels,
                );
            }
        }
    }

    { // ladybug
        game.ladybug = scene_create_entity(game.scene, CT_Transform | CT_Bug, .name = str("ladybug"));
        EntityHandle body = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
            .parent = game.ladybug,
            .sprite.id = SPRITE_LADYBUG,
            .sprite.layer = Layer_BugsBodies
        );
        for (u32 i = 0; i < sprites[SPRITE_LADYBUG].n_tags; i++)
        {
            const SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_LADYBUG].tags_i + i];
            if (t->tag != SpriteTag_leg_left && t->tag != SpriteTag_leg_right) continue;
            EntityHandle joint = scene_create_entity(game.scene, CT_Transform, .name = str("leg_joint"),
                .parent = body,
                .local.pos = (vec2s){{ t->x, t->y }},
                .local.rot = t->rot,
                .local.scale = (vec2s){{ t->tag == SpriteTag_leg_left ? -1.0f : 1.0f, 1.0f }}
            );
            scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Leg, .name = str("leg"),
                .parent = joint,
                .sprite.id = SPRITE_LADYBUG_LEG,
                .sprite.layer = Layer_BugsLegs,
            );
        }
    }

    { // centipede
        game.centipede = scene_create_entity(game.scene, CT_Transform | CT_Bug, .name = str("centipede"), .sprite.id = SPRITE_CENTIPEDE);
        scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
            .parent = game.centipede,
            .sprite.id = SPRITE_CENTIPEDE,
            .sprite.layer = Layer_BugsBodies
        );

        u32 n_segments = 0;
        EntityHandle segments[10] = { 0 };
        for (i32 i = sprites[SPRITE_CENTIPEDE].n_tags; i >= 0; i--)
        {
            const SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_CENTIPEDE].tags_i + i];
            SpriteId sprite = SPRITE_COUNT;
            if (t->tag == SpriteTag_spine_small) sprite = SPRITE_CENTIPEDE_BODY_SMALL;
            else if (t->tag == SpriteTag_spine_big) sprite = SPRITE_CENTIPEDE_BODY_BIG;
            else if (t->tag == SpriteTag_spine_medium) sprite = SPRITE_CENTIPEDE_BODY_MEDIUM;
            else if (t->tag == SpriteTag_spine_tail) sprite = SPRITE_CENTIPEDE_TAIL;
            else continue;

            EntityHandle body = segments[n_segments++] = scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Link, .name = str("body"),
                .sprite.id = sprite,
                .sprite.layer = Layer_BugsBodies,
            );

            for (u32 j = 0; j < sprites[sprite].n_tags; j++)
            {
                const SpriteTagInfo* t = &sprite_tags[sprites[sprite].tags_i + j];
                if (t->tag != SpriteTag_leg_left && t->tag != SpriteTag_leg_right) continue;
                EntityHandle joint = scene_create_entity(game.scene, CT_Transform, .name = str("leg_joint"),
                    .parent = body,
                    .local.pos = (vec2s){{ t->x, t->y }},
                    .local.rot = t->rot,
                    .local.scale = (vec2s){{ t->tag == SpriteTag_leg_left ? -1.0f : 1.0f, 1.0f }}
                );
                scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Leg, .name = str("leg"),
                    .parent = joint,
                    .sprite.id = SPRITE_CENTIPEDE_LEG,
                    .sprite.layer = Layer_BugsLegs,
                );
            }
        }

        for (u32 i = 0; i < n_segments; i++)
        {
            Entity* e = scene_get_entity(game.scene, segments[i]);
            e->link.follow = i == (n_segments - 1) ? game.centipede : segments[i + 1];
            Entity* follow = scene_get_entity(game.scene, e->link.follow);
            e->link.dist =  vec2_norm(sprite_get_tag_pos(follow->sprite.id, SpriteTag_follow)) * 0.9f;
        }
    }

    game.centipede = scene_create_entity(game.scene, CT_Transform | CT_Bug, .name = str("centipede"));
    scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
        .parent = game.centipede,
        .sprite.id = SPRITE_CENTIPEDE,
    );

    game.dpad.root = scene_create_entity(game.scene, CT_Transform, .name = str("dpad"),
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
    if (is_finger_down_at(pad->world.pos.x, pad->world.pos.y, dpad_radius, &game.dpad.finger))
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

void game_update_links(void)
{
    EntityIter iter = { .include = CT_Link };
    while (scene_next_entity(game.scene, &iter))
    {
        Entity* e = iter.entity;
        vec2s target = scene_get_entity(game.scene, e->link.follow)->world.pos;
        vec2s diff = vec2_sub(e->world.pos, target);
        f32 len = vec2_len(diff);

        if (len > e->link.dist)
        {
            float k = e->link.dist / len;
            e->world.pos.x = target.x + diff.x * k;
            e->world.pos.y = target.y + diff.y * k;
            e->world.rot   = atan2f(-diff.y, -diff.x);
        }

        entity_apply_world(e);
    }
}

void game_update(void)
{
    game_text("hello everynyan");
    game_text("fps: {}", time.avg_fps);

    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
        game_text("finger {} x: {} y: {}", iter.f->id, iter.f->x, iter.f->y);

    game_update_ui();

    game_update_bugs();

    game_update_turrets();

    game_update_links();

//     scene_print_tree(scene_root(game.scene), 0);
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
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        case SDL_EVENT_WILL_ENTER_BACKGROUND: {
            game.in_background = true;
            if (draw.surface.h.valid)
                reni_release_surface(draw.reni, draw.surface);
            draw.surface = (ReniSurface){ 0 };
        } break;
        case SDL_EVENT_DID_ENTER_FOREGROUND: {
            game.in_background = false;
        } break;

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

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {

        } break;

        case SDL_EVENT_WINDOW_RESIZED: {
            array_for_each_i(game.anchors, i)
            {
                Entity* anchor = scene_get_entity(game.scene, game.anchors[i]);
                anchor->world.pos = (vec2s){{ (float)e->window.data1 * _anchors[i].x, (float)e->window.data2 * _anchors[i].y }};
                entity_apply_world(anchor);
            }
        } break;

        default: break;
    }
    return SDL_APP_CONTINUE;
}
