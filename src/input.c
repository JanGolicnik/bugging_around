#include "marrow/genarr.h"
#define BUGGIN_INPUT
#include "base.c"

STRUCT(Finger)
{
    SDL_FingerID id;
    f32 x, y;
    u64 down;
};

struct {
    GENARR(Finger) fingers;
    vec2s dpad_dir;
} input;

void input_on_finger(u32 id, f32 x, f32 y, bool down, bool motion)
{
    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
    {
        Finger* f = iter.f;
        if ((down && !f->down) || f->id == id) {
            if (down) f->down = time.tick;
            else if (!motion) f->down = 0;
            f->id = id;
            f->x = x;
            f->y = y;
            break;
        }
    }
}

SDL_FingerID is_finger_down_at(f32 x, f32 y, f32 radius, SDL_FingerID id)
{
    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
    {
        Finger* f = iter.f;
        if (id && (f->id != id && f->down)) continue;
        if (!id && (f->down != time.tick || vec2_norm((vec2s){ .x = f->x - x, .y = f->y - y }) > radius)) continue;
        return f->id;
    }
    return 0;
}

bool is_finger_down(SDL_FingerID id)
{
    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
    {
        Finger* f = iter.f;
        if (f->id == id)
        {
            return f->down != 0;
        }
    }
    return false;
}

vec2s finger_pos(SDL_FingerID id)
{
    GENARR_ITER_ALIAS(Finger, f) iter = { 0 };
    while (genarr_next_valid(input.fingers, &iter))
    {
        Finger* f = iter.f;
        if (f->id == id)
        {
            return (vec2s){ .x = f->x, .y = f->y };
        }
    }
    return VEC2_ZERO;
}
