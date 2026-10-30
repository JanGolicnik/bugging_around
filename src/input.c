#include "marrow/genarr.h"
#define BUGGIN_INPUT
#include "base.c"

STRUCT(Finger)
{
    SDL_FingerID id;
    f32 x, y;
    u64 down_at;
};

typedef GENARR_ITER_ALIAS(Finger, f) FingerIter;
typedef GenarrHandle FingerId;

struct {
    GENARR(Finger) fingers;
    vec2s dpad_dir;
} input;

void input_on_finger(u32 id, f32 x, f32 y, bool down, bool motion)
{
    mrw_debug("input on figner {}", id);
    // find existing finger
    Finger* f = nullptr;
    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
    {
        if (iter.f->id != id) continue;
        f = iter.f;
        break;
    }

    // on down add / update already existing
    if (down)
    {
        if (!f)
        {
            FingerId h = genarr_add(input.fingers, (Finger){ .id = id });
            f = genarr_get(input.fingers, h);
        }
        f->down_at = time.tick;
        f->x = x; f->y = y;
        return;
    }

    // if finger doesnt exist then we dont do anything
    if (!f) return;

    if (motion)
    {
        f->x = x;
        f->y = y;
        return;
    }

    mrw_debug("REMOVING FINGER !!!");
    genarr_remove(input.fingers, iter.handle);
}

void finger_consume(FingerId finger)
{
    Finger* f = genarr_get(input.fingers, finger);
    if (!f) return;
    f->down_at--;
}

bool is_finger_down_at(f32 x, f32 y, f32 radius, FingerId* out)
{
    if (out->valid)
    {
        if (!genarr_is(input.fingers, *out)) *out = (FingerId){ 0 };
        return out->valid;
    }

    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
    {
        Finger* f = iter.f;
        if (f->down_at == time.tick && vec2_norm((vec2s){{ f->x - x, f->y - y }}) < radius)
        {
            *out = iter.handle;
            break;
        }
    }
    return out->valid;
}

bool is_finger_down(FingerId id)
{
    return genarr_is(input.fingers, id);
}

vec2s finger_pos(FingerId id)
{
    Finger* f = genarr_get(input.fingers, id);
    if (!f) return VEC2_ZERO;
    return (vec2s){{ f->x, f->y }};
}
