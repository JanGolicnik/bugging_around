#define BUGGIN_GAME
#include "base.c"

struct {
    bool in_background;

    struct {
        BumpAllocator _frame;
        Allocator* frame;
        Allocator* stable;
    } mem;

    f32 prev_time;
    f32 dt;
    f32 dt_accum;
    u32 dt_n_samples;

    f32 avg_fps;

    VEKTOR(Scene*) scenes;
    Scene* scene;
} game;

Scene* game_new_scene(void) {
    Scene* scene = mrw_alloc(game.mem.stable, Scene);
    vektor_add(game.scenes, scene);
    *scene = (Scene) { 0 };
    genarr_init(scene->entities, 123, game.mem.stable);
    return scene;
}

bool game_init(void)
{
    game.mem.stable = nullptr;
    game.mem._frame = (BumpAllocator){ MRW_BUMP_IMPL };
    game.mem.frame = (Allocator*)&game.mem._frame;

    game.scene = game_new_scene();
    scene_create_entity(game.scene, CT_Transform, .name = str("gospod1"), .transform.x = 100.0f, .transform.y = 200.0f);
    scene_create_entity(game.scene, CT_Transform, .name = str("gospod2"), .transform.x = 200.0f, .transform.y = 200.0f);
    scene_create_entity(game.scene, CT_Transform, .name = str("gospod3"), .transform.x = 300.0f, .transform.y = 200.0f);
    scene_create_entity(game.scene, CT_Transform, .name = str("gospod4"), .transform.x = 400.0f, .transform.y = 200.0f);

    return draw_init(game.mem.stable, game.mem.frame);
}

void game_update(void)
{
    f32 time = (f32)SDL_GetTicksNS() / 1000000000.0f;
    game.dt = time - game.prev_time;
    game.dt_accum += game.dt;
    game.dt_n_samples += 1;
    if (game.dt_accum > 1.0f) {
        game.avg_fps = (f32)game.dt_n_samples / game.dt_accum;
        game.dt_accum -= 0.3f;
        game.dt_n_samples = 0;
        mrw_debug_val(game.avg_fps);
    }
    game.prev_time = time;

    RIPPLE( FORM( .width = FROM_CHILD, .height = FROM_CHILD), RECT( .color = RIPPLE_RGBA(0x2e2e2ebf), .radiusBR = .15f ) )
    {
        text(str("hello everynyan"));
        text(mrw_format("fps: {}", game.mem.frame, game.avg_fps));

        EntityIter iter = { .include = CT_Transform };
        while(scene_next_entity(game.scene, &iter))
        {
            text(mrw_format("entity {} exists", game.mem.frame, iter.entity->name));
            text(mrw_format("at {} {}", game.mem.frame, iter.entity->transform.x, iter.entity->transform.y));
        }
    }
}

void game_tick(void)
{
    if (game.in_background || !draw_is_ready()) return;

    mrw_bump_reset(&game.mem._frame);

    game_update();

    draw_scene(game.scene);
}

void game_destroy(void)
{
    return;
}

SDL_AppResult game_on_event(SDL_Event* event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            game.in_background = true;
            reni_release_surface(draw.reni, draw.surface);
            draw.surface = (ReniSurface){ 0 };
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            game.in_background = false;
            break;
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}
