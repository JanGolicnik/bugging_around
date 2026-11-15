#define BUGGIN_GAME
#include "base.c"

#define MAX_SCENES 10

struct {
    bool in_background;

    struct {
        BumpAllocator _frame;
        Allocator* frame;
        Allocator* stable;
    } mem;

    Scene scenes[MAX_SCENES];
    u32 n_scenes;
    Scene* scene;

    EntityHandle anchors[9];

    EntityHandle stinkbug;
    EntityHandle ladybug;
    EntityHandle centipede;

    EntityHandle stump;

    struct
    {
        EntityHandle root;
        EntityHandle bg;
        EntityHandle pad;
        FingerId finger;
    } dpad;
} game;

Scene* game_new_scene(void)
{
    if (game.n_scenes >= MAX_SCENES) mrw_abort("max number of scenes reached");
    Scene *scene = &game.scenes[game.n_scenes++];
    *scene = scene_new(game.mem.stable);
    return scene;
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
    const f32 Radius = 200.0f;
    static FingerId f = { 0 };
    static EntityHandle held = { 0 };
    static EntityHandle last_held = { 0 };
    EntityIter iter = { .include = CT_Bug };
    while (scene_next_entity(game.scene, &iter))
    {
        Entity* e = iter.entity;
        Entity* sprite = entity_first_child_with(e, CT_Sprite);

        if (!held.valid && is_finger_down_at(e->world.pos.x, e->world.pos.y, 100.0f, &f))
        {
            finger_consume(f);
            held = iter.handle;
            last_held = held;
        }

        if (is_finger_up(f))
        {
            held = (EntityHandle){ 0 };
        }

        if (held.i == iter.handle.i)
        {
            e->bug.target = finger_pos(f);
        }

        if (last_held.i == iter.handle.i && vec2_norm2(input.dpad_dir) > 0.001f)
        {
            e->bug.target = vec2_add(e->world.pos, vec2_scale(input.dpad_dir, Radius * 2.0f));
        }

        if (vec2_distance(e->bug.target, e->world.pos) < Radius)
        {
            sprite->local.rot += -sprite->local.rot * (1.0f - exp(-10.0f * timer.dt));
            entity_apply_local(sprite);

            if (e->bug.next_target_at < timer.time)
            {
                e->bug.target = (vec2s){{ mrw_random_f32(-0.4f, 0.4f) * draw_width(), mrw_random_f32(-0.3f, 0.3f) * draw_height() }};
                e->bug.next_target_at = timer.time + mrw_random_f32(1.0f, 5.0f);
            }
        }
        else
        {
            vec2s dir = vec2_normalize(vec2_sub(e->bug.target, e->world.pos));
            e->world.rot = atan2(dir.y, dir.x);
            e->world.pos = vec2_add(e->world.pos, vec2_scale(dir, 200.0f * timer.dt));
            entity_apply_world(e);

            sprite->local.rot = sin(timer.time * 10.0f) * 0.1;
            entity_apply_local(sprite);

            EntityIter iter = { .include = CT_Leg };
            while (scene_next_entity(game.scene, &iter))
            {
                iter.entity->local.rot = sin(timer.time * 30.0f) * 0.1f;
                entity_apply_local(iter.entity);
            }

            audio_play_ex(Sound_Click, 1.0f, mrw_random_f32(0.5f, 1.5f));
        }
    }
}

EntityHandle spawn_stinkbug(vec2s pos)
{
    EntityHandle h = scene_create_entity(game.scene, CT_Transform | CT_Bug | CT_Collision, .name = str("stinkbug"),
        .collision = collision_from_sprite(SPRITE_STINKBUG, 0),
        .world.pos = pos,
        .world.scale = VEC2_ONE
    );
    EntityHandle body = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
        .parent = h,
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
                .local.pos = t->pos,
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
                .local.pos = t->pos,
                .local.rot = t->rot,
                .sprite.id = SPRITE_CANNON1,
                .sprite.layer = Layer_Turrets,
            );
            SpriteTagInfo* t = sprite_get_tag(SPRITE_CANNON1, SpriteTag_turret);
            scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("barrel"),
                .parent = turret,
                .local.pos = t->pos,
                .local.rot = t->rot,
                .sprite.id = SPRITE_CANNON1_BARREL,
                .sprite.layer = Layer_Barrels,
            );
        }
    }
    return h;
}

EntityHandle spawn_ladybug(vec2s pos)
{
    EntityHandle h = scene_create_entity(game.scene, CT_Transform | CT_Bug | CT_Collision, .name = str("ladybug"),
        .collision = collision_from_sprite(SPRITE_LADYBUG, 0),
        .world.pos = pos,
        .world.scale = VEC2_ONE
    );
    EntityHandle body = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
        .parent = h,
        .sprite.id = SPRITE_LADYBUG,
        .sprite.layer = Layer_BugsBodies
    );
    for (u32 i = 0; i < sprites[SPRITE_LADYBUG].n_tags; i++)
    {
        const SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_LADYBUG].tags_i + i];
        if (t->tag != SpriteTag_leg_left && t->tag != SpriteTag_leg_right) continue;
        EntityHandle joint = scene_create_entity(game.scene, CT_Transform, .name = str("leg_joint"),
            .parent = body,
            .local.pos = t->pos,
            .local.rot = t->rot,
            .local.scale = (vec2s){{ t->tag == SpriteTag_leg_left ? -1.0f : 1.0f, 1.0f }}
        );
        scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Leg, .name = str("leg"),
            .parent = joint,
            .sprite.id = SPRITE_LADYBUG_LEG,
            .sprite.layer = Layer_BugsLegs,
        );
    }
    return h;
}

EntityHandle spawn_centipede(vec2s pos)
{
    EntityHandle h = scene_create_entity(game.scene, CT_Transform | CT_Bug, .name = str("centipede"), .sprite.id = SPRITE_CENTIPEDE);
    scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("sprite"),
        .parent = h,
        .sprite.id = SPRITE_CENTIPEDE,
        .sprite.layer = Layer_BugsHead
    );

    u32 n_segments = 0;
    EntityHandle segments[10] = { 0 };
    for (i32 i = sprites[SPRITE_CENTIPEDE].n_tags - 1; i >= 0; i--)
    {
        const SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_CENTIPEDE].tags_i + i];
        SpriteId sprite = SPRITE_COUNT;
        if (t->tag == SpriteTag_spine_small) sprite = SPRITE_CENTIPEDE_BODY_SMALL;
        else if (t->tag == SpriteTag_spine_big) sprite = SPRITE_CENTIPEDE_BODY_BIG;
        else if (t->tag == SpriteTag_spine_medium) sprite = SPRITE_CENTIPEDE_BODY_MEDIUM;
        else if (t->tag == SpriteTag_spine_tail) sprite = SPRITE_CENTIPEDE_TAIL;
        else continue;

        EntityHandle body = segments[n_segments++] =
            scene_create_entity(game.scene, CT_Transform | CT_Sprite | CT_Link | CT_Collision, .name = str("body"),
            .sprite.id = sprite,
            .sprite.layer = Layer_BugsBodies,
        );

        for (u32 j = 0; j < sprites[sprite].n_tags; j++)
        {
            const SpriteTagInfo* t = &sprite_tags[sprites[sprite].tags_i + j];
            if (t->tag != SpriteTag_leg_left && t->tag != SpriteTag_leg_right) continue;
            EntityHandle joint = scene_create_entity(game.scene, CT_Transform, .name = str("leg_joint"),
                .parent = body,
                .local.pos = t->pos,
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
        e->link.follow = i == (n_segments - 1) ? h : segments[i + 1];
        Entity* follow = scene_get_entity(game.scene, e->link.follow);
        e->link.dist = vec2_norm(sprite_get_tag(follow->sprite.id, SpriteTag_follow)->pos) * 0.85f;
        e->collision = collision_from_sprite(e->sprite.id, CollisionGroup_Centipede);
    }
    return h;
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

    game.stinkbug = spawn_stinkbug(VEC2_ZERO);
    game.ladybug = spawn_ladybug(VEC2_ZERO);
    game.centipede = spawn_centipede(VEC2_ZERO);

    EntityHandle leaves = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("leaves"),
        .parent = game.anchors[Anchor_mt],
        .sprite.id = SPRITE_LEAVES,
        .sprite.layer = Layer_Debug
    );
    for (u32 i = 0; i < sprites[SPRITE_LEAVES].n_tags; i++)
    {
        SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_LEAVES].tags_i + i];
        if (t->tag != SpriteTag_leg_left && t->tag != SpriteTag_leg_right) continue;
        scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("leaf"),
            .parent = leaves,
            .sprite.id = SPRITE_LEAF,
            .local.pos = t->pos,
            .local.rot = t->rot,
            .local.scale = (vec2s){{ t->tag == SpriteTag_leg_left ? 1.0f : -1.0f, 1.0f }}
        );
    }

    game.stump = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("stump"),
        .parent = game.anchors[Anchor_mm],
        .sprite.id = SPRITE_STUMP,
        .sprite.layer = Layer_Background2
    );
    for (u32 i = 0; i < sprites[SPRITE_STUMP].n_tags; i++)
    {
        SpriteTagInfo* t = &sprite_tags[sprites[SPRITE_STUMP].tags_i + i];
        if (t->tag != SpriteTag_leg) continue;
        scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("stump_leg"),
            .parent = game.stump,
            .sprite.id = SPRITE_STUMP_LEG,
            .local.pos = t->pos,
            .local.rot = t->rot,
            .sprite.layer = Layer_Background1
        );
    }

    game.dpad.root = scene_create_entity(game.scene, CT_Transform, .name = str("dpad"),
        .parent = game.anchors[Anchor_lb],
        .local.pos = vec2_adds(vec2_scale(sprite_size(SPRITE_DPAD_BG), 0.5f), 50.0f),
        .sprite.layer = Layer_Ui
    );

    game.dpad.bg = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad_bg"),
        .parent = game.dpad.root,
        .sprite.id = SPRITE_DPAD_BG
    );
    game.dpad.pad = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("dpad_pad"),
        .parent = game.dpad.root,
        .sprite.id = SPRITE_DPAD_PAD
    );

    scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("woah"),
        .sprite.text = str("HELLO"),
        .sprite.layer = Layer_Debug,
        .sprite.text_size = 128.0f
    );

    scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = str("woah2"),
        .sprite.text = str("jaz\nsem\njan"),
        .sprite.layer = Layer_Debug,
        .world.pos.y = 256.0f,
        .world.scale = VEC2_ONE
    );

    return true;
}

#define game_text(...) text(mrw_format(game.mem.frame, __VA_ARGS__))

void game_update_collisions(void)
{
    EntityIter iter = { .include = CT_Collision | CT_Transform };
    while (scene_next_entity(game.scene, &iter))
    {
        Entity* a = iter.entity;
        vec2s a_pos = entity_local_to_world(a, a->collision.pos);
        EntityIter iter2 = iter;
        while (scene_next_entity(game.scene, &iter2))
        {
            Entity* b = iter2.entity;
            if (a->collision.fixed && b->collision.fixed) continue;
            if (a->collision.group != 0 && b->collision.group != 0 && (a->collision.group & b->collision.group)) continue;

            vec2s b_pos = entity_local_to_world(b, b->collision.pos);

            f32 radius = a->collision.radius + b->collision.radius;
            vec2s aTob = vec2_sub(b_pos, a_pos);
            f32 dist2 = vec2_norm2(aTob);
            if (dist2 >= radius * radius) continue;

            f32 dist = sqrtf(dist2);
            vec2s dir = (dist > 1e-6f) ? vec2_scale(aTob, 1.0f / dist) : (vec2s){ .x = 1.0f, .y = 0.0f };
            f32 delta = radius - dist;
            f32 t = a->collision.fixed ? 0.0f : (b->collision.fixed ? 1.0f : 0.5f);
            a->world.pos = vec2_sub(a->world.pos, vec2_scale(dir, delta * t));
            b->world.pos = vec2_add(b->world.pos, vec2_scale(dir, delta * (1.0f - t)));
            entity_apply_world(a);
            entity_apply_world(b);
        }
    }
}

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

SoundHandle music = { 0 };
f32 music_volume = 0.0f;
void game_update(void)
{
    game_text("hello everynyan");
    game_text("fps: {}", timer.avg_fps);

    if (slider("music volume", &music_volume, 0.0f, 1.0f, game.mem.frame))
    {
        if (!music.valid) {
            music = audio_play(Sound_Music);
        }
        audio_set_volume(music, music_volume);
    }

    slider("scaling", &draw.scaling, 0.1f, 10.0f, game.mem.frame);
    if (button(str("ifon")))
    {
        draw_set_size(1179 / 2, 2556 /2);
        draw.scaling = draw.scaling == 1.0f ? 2.0f : 1.0f;
    }

    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
        game_text("finger {} x: {} y: {}", iter.f->id, iter.f->x, iter.f->y);

    tweens_update(game.scene, timer.dt);

    FingerId finger = { 0 };
    if (is_finger_pressed_at(0.0f, 0.0f, 100.0f, &finger))
    {
        EntityHandle e = scene_create_entity(game.scene, CT_Transform | CT_Sprite, .name = sstr("test"),
            .sprite.id = SPRITE_DPAD_PAD,
            .sprite.layer = Layer_Debug,
            .world.pos = finger_pos(finger),
            .world.scale = {{ 1.0f, 1.0f }}
        );
        tween(e, world.pos.x, 100.0f, 2.0f, EASE_OUT_ELASTIC, .flags = Tween_Relative | Tween_World);
        tween(e, world.scale, VEC2_ZERO, 1.0f, EASE_OUT, .delay = 1.0f, .flags = Tween_Delete);
    }

    game_update_ui();

    game_update_bugs();

    game_update_turrets();

    game_update_links();

    game_update_collisions();

//     scene_print_tree(scene_root(game.scene), 0);
}

void game_tick(void)
{
    if (game.in_background || !draw_is_ready()  || !audio_is_ready()) return;

    mrw_bump_reset(&game.mem._frame);

    draw.scaling = 2.0f;
    draw_set_size(1179 / 2, 2556 /2);

    time_update();

    RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD), RECT( .color = RIPPLE_RGBA(0x2e2e2ebf), .radiusBR = .15f ) )
    {
        game_update();
    }

    // game_print_tree(scene_get_entity(game.scene, game.scene->root), 0);

    draw_scene(game.scene, timer.time);

    timer.tick++;
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
            audio_suspend();
        } break;
        case SDL_EVENT_DID_ENTER_FOREGROUND: {
            game.in_background = false;
            audio_resume();
        } break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
            vec2s size = draw_size();

            bool down = e->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            bool motion = e->type == SDL_EVENT_MOUSE_MOTION;
            ripple_on_mouse_event(&input.ripple, (RippleMouseEvent) {
                .type = motion ? RippleEventType_MouseMove : (down ? RippleEventType_MousePress : RippleEventType_MouseRelease), .button = 0,
                .x = e->motion.x, .y = e->motion.y
            });

            input_on_finger(1, e->motion.x * draw.scaling - size.x * 0.5f, size.y * 0.5f - e->motion.y * draw.scaling, down, motion);
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
            f32 w = (f32)e->window.data1 * draw.scaling;
            f32 h = (f32)e->window.data2 * draw.scaling;
            array_for_each_i(game.anchors, i)
            {
                Entity* anchor = scene_get_entity(game.scene, game.anchors[i]);
                anchor->world.pos = (vec2s){{ w * _anchors[i].x, h * _anchors[i].y }};
                entity_apply_world(anchor);
            }
        } break;

        default: break;
    }
    return SDL_APP_CONTINUE;
}
