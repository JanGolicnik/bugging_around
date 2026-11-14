#define BUGGIN_TIME
#include "base.c"

struct {
    u64 tick;
    f32 time;
    f32 dt;
    f32 dt_accum;
    u32 dt_n_samples;

    f32 avg_fps;
} timer;

void time_update(void)
{
    f32 now = (f32)SDL_GetTicksNS() / 1000000000.0f;
    timer.dt = now - timer.time;
    timer.dt_accum += timer.dt;
    timer.dt_n_samples += 1;
    if (timer.dt_accum > 1.0f) {
        timer.avg_fps = (f32)timer.dt_n_samples / timer.dt_accum;
        timer.dt_accum -= 1.0f;
        timer.dt_n_samples = 0;
        mrw_debug_val(timer.avg_fps);
    }
    timer.time = now;
}
