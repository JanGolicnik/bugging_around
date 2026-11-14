#define BUGGIN_AUDIO
#include "base.c"

#include <miniaudio.h>

#define MAX_ACTIVE_SOUNDS 128

typedef GenarrHandle SoundHandle;

typedef enum {
    AudioLayer_Sfx,
    AudioLayer_Ui,
    AudioLayer_Music,
    AudioLayer_Count
} AudioLayer;

typedef enum {
    Sound_Click,
    Sound_Music,
    Sound_Count,
} SoundId;

STRUCT(SoundInfo) {
    cstr path;
    AudioLayer layer;
    bool stream;
    bool loop;
};

static SoundInfo sound_info[Sound_Count] = {
    [Sound_Click] = {
        .path = "res/sfx/click.wav",
        .layer = AudioLayer_Ui,
    },
    [Sound_Music] = {
        .path = "res/music/menu.flac",
        .layer = AudioLayer_Music,
        .stream = true,
        .loop = true,
    },
};

STRUCT(Sound) { ma_sound sound; bool loaded; };
STRUCT(ActiveSound) { ma_sound sound; };
STRUCT(ActiveSoundIter) { GENARR_ITER_ALIAS(ActiveSound, sound); };

STRUCT(Audio) {
    ma_engine engine;
    ma_sound_group layers[AudioLayer_Count];
    Sound sounds[Sound_Count];
    GENARR(ActiveSound) active_sounds;
    u32 n_active_sounds;
    bool all_loaded;
};

static Audio audio;

static void ma_log_cb(void* user, ma_uint32 level, const char* msg) {
    mrw_unused user;
    mrw_debug("miniaudio({}) {}", ma_log_level_to_string(level), msg);
}

bool audio_init(Allocator* stable) {
    if (ma_engine_init(nullptr, &audio.engine) != MA_SUCCESS)
    {
        mrw_error("failed to initialize audio");
        return false;
    }
    ma_log_register_callback(ma_engine_get_log(&audio.engine), ma_log_callback_init(ma_log_cb, nullptr));

    for (u32 i = 0; i < AudioLayer_Count; i++) {
        if (ma_sound_group_init(&audio.engine, 0, nullptr, &audio.layers[i]) != MA_SUCCESS)
        {
            mrw_error("failed to initialize audio layer");
            return false;
        }
    }

    genarr_init(audio.active_sounds, MAX_ACTIVE_SOUNDS, stable);

    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_SPATIALIZATION;

    for (u32 i = 0; i < Sound_Count; i++) {
        SoundInfo* info = &sound_info[i];
        if (info->stream) continue;
        if (ma_sound_init_from_file(&audio.engine, info->path, flags, nullptr, nullptr, &audio.sounds[i].sound) != MA_SUCCESS)
            mrw_abort("audio: can't open {}", info->path);
    }

    return true;
}

static void active_sound_free(SoundHandle h, ActiveSound* sound) {
    ma_sound_uninit(&sound->sound);
    genarr_remove(audio.active_sounds, h);
    audio.n_active_sounds--;
}

void audio_shutdown(void) {
    ActiveSoundIter iter = { 0 };
    while (genarr_next_valid(audio.active_sounds, &iter))
        active_sound_free(iter.handle, iter.sound);

    for (u32 i = 0; i < Sound_Count; i++) {
        if (sound_info[i].stream) continue;
        ma_sound_uninit(&audio.sounds[i].sound);
    }

    for (u32 i = 0; i < AudioLayer_Count; i++)
        ma_sound_group_uninit(&audio.layers[i]);

    ma_engine_uninit(&audio.engine);
}

void audio_suspend(void) {
    ma_engine_stop(&audio.engine);
}

void audio_resume(void) {
    ma_engine_start(&audio.engine);
}

bool audio_is_ready(void) {
    if (audio.all_loaded) return true;

    audio.all_loaded = true;
    for (u32 i = 0; i < Sound_Count; i++) {
        SoundInfo* info = &sound_info[i];
        if (info->stream || audio.sounds[i].loaded) continue;

        ma_result r = ma_resource_manager_data_source_result((ma_resource_manager_data_source*)ma_sound_get_data_source(&audio.sounds[i].sound));
        if (r == MA_BUSY) {
            audio.all_loaded = false;
            continue;
        }
        if (r != MA_SUCCESS)
            mrw_abort("audio: failed to load {}: {}", sound_info[i].path, ma_result_description(r));
        if (r != MA_SUCCESS)
            mrw_abort("audio: failed to decode {}", info->path);

        mrw_debug("loaded sound {}", info->path);
        audio.sounds[i].loaded = true;
    }
    if (audio.all_loaded)
        mrw_debug("all sounds loaded!!");
    return audio.all_loaded;
}

SoundHandle audio_play_ex(SoundId id, f32 volume, f32 pitch) {
    if ((u32)id >= Sound_Count || audio.n_active_sounds >= MAX_ACTIVE_SOUNDS)
        return (SoundHandle){ 0 };

    SoundInfo* info = &sound_info[id];
    if (!info->stream && !audio.sounds[id].loaded)
        return (SoundHandle){ 0 };

    SoundHandle h = genarr_add(audio.active_sounds, (ActiveSound){ 0 });
    ActiveSound* sound = genarr_get(audio.active_sounds, h);
    ma_sound_group* group = &audio.layers[info->layer];
    ma_uint32 flags = MA_SOUND_FLAG_NO_SPATIALIZATION;

    ma_result r = info->stream
        ? ma_sound_init_from_file(&audio.engine, info->path, flags | MA_SOUND_FLAG_STREAM, group, nullptr, &sound->sound)
        : ma_sound_init_copy(&audio.engine, &audio.sounds[id].sound, flags, group, &sound->sound);

    if (r != MA_SUCCESS) {
        mrw_error("failed to play sound {}", (i32)id);
        genarr_remove(audio.active_sounds, h);
        return (SoundHandle){ 0 };
    }
    audio.n_active_sounds++;

    ma_sound_set_looping(&sound->sound, info->loop);
    ma_sound_set_volume(&sound->sound, volume);
    ma_sound_set_pitch(&sound->sound, pitch);
    ma_sound_start(&sound->sound);
    return h;
}

#define audio_play(id) audio_play_ex((id), 1.0f, 1.0f)

void audio_stop(SoundHandle h) {
    ActiveSound* sound = genarr_get(audio.active_sounds, h);
    if (sound) active_sound_free(h, sound);
}

bool audio_alive(SoundHandle h) {
    return genarr_get(audio.active_sounds, h) != nullptr;
}

void audio_set_volume(SoundHandle h, f32 volume) {
    ActiveSound* sound = genarr_get(audio.active_sounds, h);
    if (sound) ma_sound_set_volume(&sound->sound, volume);
}

void audio_set_pitch(SoundHandle h, f32 pitch) {
    ActiveSound* sound = genarr_get(audio.active_sounds, h);
    if (sound) ma_sound_set_pitch(&sound->sound, pitch);
}

void audio_tick(void) {
    ActiveSoundIter iter = { 0 };
    while (genarr_next_valid(audio.active_sounds, &iter)) {
        if (ma_sound_at_end(&iter.sound->sound))
            active_sound_free(iter.handle, iter.sound);
    }
}

void audio_layer_set_paused(AudioLayer l, bool paused) {
    if (paused) ma_sound_group_stop(&audio.layers[l]);
    else        ma_sound_group_start(&audio.layers[l]);
}

void audio_layer_set_volume(AudioLayer l, f32 volume) {
    ma_sound_group_set_volume(&audio.layers[l], volume);
}

void audio_set_master_volume(f32 volume) {
    ma_engine_set_volume(&audio.engine, volume);
}
