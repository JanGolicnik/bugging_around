#define BUGGIN_TWIN
#include "base.c"

#define MAX_TWEENS 1024

typedef enum {
    Tween_Playing = BIT(0),
    Tween_From = BIT(1),
    Tween_Relative = BIT(2),
    Tween_Loop = BIT(3),
    Tween_Delete = BIT(4),
    Tween_Local = BIT(5),
    Tween_World = BIT(6),
} TweenFlags;

typedef struct {
    EntityHandle target;
    u16 offset;
    u16 count;
    Easing ease;
    TweenFlags flags;
    f32 delay, time, duration;
    f32 from[4], to[4];
} Tween;

struct {
    Tween tweens[MAX_TWEENS];
    u32 n_tweens;
} tweens;

void tweens_update(Scene* scene, f32 dt)
{
    for (i32 i = 0; i < (i32)tweens.n_tweens; i++)
    {
        Tween *tween = &tweens.tweens[i];
        Entity *e = scene_get_entity(scene, tween->target);
        if (!e)
        {
            tweens.tweens[i] = tweens.tweens[--tweens.n_tweens];
            i--;
            continue;
        }

        f32 step = dt;
        if (tween->delay > 0.0f) {
            tween->delay -= step;
            if (tween->delay > 0.0f) {
                continue;
            }
            step = -tween->delay;
        }

        f32* val = (f32*)((u8*)e + tween->offset);
        if (!FLAG_HAS(tween->flags, Tween_Playing)) {
            FLAG_SET(tween->flags, Tween_Playing);
            if (!FLAG_HAS(tween->flags, Tween_From)) buf_copy(tween->from, val, tween->count * sizeof(*val));
            if (FLAG_HAS(tween->flags, Tween_Relative))
                for (i32 j = 0; j < tween->count; j++)
                    tween->to[j] += tween->from[j];
        }

        tween->time += step;
        f32 progress = clamp(tween->time / tween->duration, 0.0f, 1.0f);
        f32 t = mrw_ease(progress, tween->ease);
        for (i32 j = 0; j < tween->count; j++)
            val[j] = tween->from[j] + (tween->to[j] - tween->from[j]) * t;

        if (FLAG_HAS(tween->flags, Tween_World)) {
            entity_apply_world(e);
        }
        if (FLAG_HAS(tween->flags, Tween_Local)) {
            entity_apply_local(e);
        }

        if (progress >= 1.0f)
        {
            if (FLAG_HAS(tween->flags, Tween_Loop)) {
                for (i32 j = 0; j < tween->count; j++)
                {
                    f32 tmp = tween->from[j];
                    tween->from[j] = tween->to[j];
                    tween->to[j] = tmp;
                }
                tween->time = tween->duration - tween->time;
                continue;
            }

            if (FLAG_HAS(tween->flags, Tween_Delete)) scene_delete_entity(scene, tween->target);
            tweens.tweens[i] = tweens.tweens[--tweens.n_tweens];
            i--;
        }
    }
}

void tween_add(Tween tween, f32* to) {
    if (tweens.n_tweens >= MAX_TWEENS) return;
    Tween* t = &tweens.tweens[tweens.n_tweens++];
    *t = tween;
    buf_copy(t->to, to, t->count * sizeof(f32));
}

#define tween(handle, field, to_, duration_, ease_, ...) do {\
    typeof(((Entity*)0)->field) to__ = to_;\
    tween_add((Tween){\
        .target = (handle), .offset = offsetof(Entity, field), .count = sizeof(((Entity*)0)->field) / sizeof(f32), .duration = (duration_), .ease = (ease_),\
        __VA_ARGS__ \
    }, (f32*)&to__);\
} while (0)
