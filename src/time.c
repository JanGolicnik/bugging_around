#define BUGGIN_TIME
#include "base.c"

struct {
    u64 tick;
    f32 time;
    f32 dt;
    f32 dt_accum;
    u32 dt_n_samples;

    f32 avg_fps;
} time;

void time_update(void)
{
    f32 now = (f32)SDL_GetTicksNS() / 1000000000.0f;
    time.dt = now - time.time;
    time.dt_accum += time.dt;
    time.dt_n_samples += 1;
    if (time.dt_accum > 1.0f) {
        time.avg_fps = (f32)time.dt_n_samples / time.dt_accum;
        time.dt_accum -= 0.3f;
        time.dt_n_samples = 0;
        mrw_debug_val(time.avg_fps);
    }
    time.time = now;
}
