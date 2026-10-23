#define BUGGIN_INPUT
#include "base.c"

struct {
    struct
    {
        SDL_FingerID id;
        f32 x, y;
        u64 down;
    } fingers[10];

    vec2s dpad_dir;
} input;

void input_on_finger(u32 id, f32 x, f32 y, bool down, bool motion)
{
    array_for_each_i(input.fingers, i) {
        if ((down && !input.fingers[i].down) || input.fingers[i].id == id) {
            if (down) input.fingers[i].down = time.tick;
            else if (!motion) input.fingers[i].down = 0;
            input.fingers[i].id = id;
            input.fingers[i].x = x;
            input.fingers[i].y = y;
            break;
        }
    }
}

SDL_FingerID is_finger_down_at(f32 x, f32 y, f32 radius, SDL_FingerID id)
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
        if (input.fingers[i].down == time.tick &&
            vec2_norm((vec2s){ .x = input.fingers[i].x - x, .y = input.fingers[i].y - y }) < radius)
        {
            return input.fingers[i].id;
        }
    }
    return 0;
}

bool is_finger_down(SDL_FingerID id)
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
