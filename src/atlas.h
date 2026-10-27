#pragma once

#include <marrow/marrow.h>

#define ATLAS_PAGE_COUNT 2
#define ATLAS_SIZE 2048
typedef enum {
    SpriteTag_None,
    SpriteTag_attach,
    SpriteTag_leg,
    SpriteTag_leg_2,
    SpriteTag_leg_3,
    SpriteTag_leg_4,
    SpriteTag_leg_5,
    SpriteTag_leg_6,
    SpriteTag_Count
} SpriteTag;

typedef enum {
    SPRITE_BUGGIN____AROUND,
    SPRITE_ARENA,
    SPRITE_CENTIPEDE,
    SPRITE_DPAD_PAD,
    SPRITE_DPAG_BG,
    SPRITE_ENTER,
    SPRITE_LADYBUG,
    SPRITE_LEAVES,
    SPRITE_S1,
    SPRITE_S2,
    SPRITE_STINKBUG,
    SPRITE_STINKBUG_LEG,
    SPRITE_THE_METAMORPH,
    SPRITE_COUNT
} SpriteId;

typedef struct { SpriteTag tag; f32 x, y; } SpriteTagInfo;
typedef struct {
    i32 page;
    i32 x, y, w, h;
    i32 ox, oy;
    f64 u0, v0, u1, v1;
    u32 tags_i, n_tags;
} SpriteInfo;

static const SpriteTagInfo sprite_tags[] = {
    { SpriteTag_attach, 339.500000, 180.000000 },
    { SpriteTag_leg, 6.265015, -82.500000 },
    { SpriteTag_leg_2, -50.234993, -92.000000 },
    { SpriteTag_leg_3, -51.234985, 39.000000 },
    { SpriteTag_leg_4, 5.265015, 29.500000 },
    { SpriteTag_leg_5, 42.764999, 35.000000 },
    { SpriteTag_leg_6, 43.265015, -87.500000 },
};

static const SpriteInfo sprites[SPRITE_COUNT] = {
    { 1, 853, 513, 878, 478, 464, 237, 0.416504, 0.250488, 0.845215, 0.483887, 0, 0 },
    { 0, 875, 2, 1038, 773, 519, 386, 0.427246, 0.000977, 0.934082, 0.378418, 0, 0 },
    { 1, 2, 2, 847, 552, 429, 276, 0.000977, 0.000977, 0.414551, 0.270508, 0, 0 },
    { 0, 1803, 1074, 174, 174, 87, 87, 0.880371, 0.524414, 0.965332, 0.609375, 0, 0 },
    { 1, 853, 2, 507, 507, 253, 253, 0.416504, 0.000977, 0.664062, 0.248535, 0, 0 },
    { 1, 853, 995, 865, 476, 432, 238, 0.416504, 0.485840, 0.838867, 0.718262, 0, 0 },
    { 0, 2, 1731, 321, 283, 160, 141, 0.000977, 0.845215, 0.157715, 0.983398, 0, 0 },
    { 0, 2, 1039, 1797, 688, 899, 344, 0.000977, 0.507324, 0.878418, 0.843262, 0, 0 },
    { 0, 327, 1731, 364, 278, 12, 12, 0.159668, 0.845215, 0.337402, 0.980957, 0, 1 },
    { 0, 1803, 779, 243, 291, 84, 195, 0.880371, 0.380371, 0.999023, 0.522461, 1, 0 },
    { 0, 695, 1731, 321, 213, 161, 103, 0.339355, 0.845215, 0.496094, 0.949219, 1, 6 },
    { 0, 1803, 1252, 99, 118, 21, 11, 0.880371, 0.611328, 0.928711, 0.668945, 7, 0 },
    { 0, 2, 2, 869, 1033, 434, 493, 0.000977, 0.000977, 0.425293, 0.505371, 7, 0 },
};
