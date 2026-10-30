#pragma once

#include <marrow/marrow.h>

#define ATLAS_PAGE_COUNT 2
#define ATLAS_SIZE 2048
typedef enum {
    SpriteTag_None,
    SpriteTag_attach,
    SpriteTag_left,
    SpriteTag_leg_left,
    SpriteTag_leg_right,
    SpriteTag_right,
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
    SPRITE_LINE,
    SPRITE_S1,
    SPRITE_S2,
    SPRITE_STINKBUG,
    SPRITE_STINKBUG_LEG,
    SPRITE_THE_METAMORPH,
    SPRITE_COUNT
} SpriteId;

typedef struct { SpriteTag tag; f32 x, y, rot; } SpriteTagInfo;
typedef struct {
    i32 page;
    i32 x, y, w, h;
    i32 ox, oy;
    f64 u0, v0, u1, v1;
    u32 tags_i, n_tags;
} SpriteInfo;

static const SpriteTagInfo sprite_tags[] = {
    { SpriteTag_attach, 351.500000, 207.333344, 3.422125 },
    { SpriteTag_leg_right, 108.431686, 169.000000, 1.890953 },
    { SpriteTag_leg_right, 165.098358, 159.500000, 1.890952 },
    { SpriteTag_leg_right, 202.764999, 165.000000, 1.890954 },
    { SpriteTag_leg_left, 109.431671, 38.000000, 1.890955 },
    { SpriteTag_leg_left, 166.098358, 47.500000, 1.890952 },
    { SpriteTag_leg_left, 203.386520, 43.716553, 5.334968 },
    { SpriteTag_right, 285.248352, 184.361664, 2.471505 },
    { SpriteTag_left, 278.764343, 25.000267, 3.731310 },
};

static const SpriteInfo sprites[SPRITE_COUNT] = {
    { 1, 853, 513, 878, 478, 464, 242, 0.416504, 0.250488, 0.845215, 0.483887, 0, 0 },
    { 0, 875, 2, 1038, 773, 519, 386, 0.427246, 0.000977, 0.934082, 0.378418, 0, 0 },
    { 1, 2, 2, 847, 552, 429, 276, 0.000977, 0.000977, 0.414551, 0.270508, 0, 0 },
    { 0, 1803, 1074, 174, 174, 87, 87, 0.880371, 0.524414, 0.965332, 0.609375, 0, 0 },
    { 1, 853, 2, 507, 507, 253, 253, 0.416504, 0.000977, 0.664062, 0.248535, 0, 0 },
    { 1, 853, 995, 865, 476, 432, 238, 0.416504, 0.485840, 0.838867, 0.718262, 0, 0 },
    { 0, 2, 1731, 321, 283, 160, 141, 0.000977, 0.845215, 0.157715, 0.983398, 0, 0 },
    { 0, 2, 1039, 1797, 688, 936, 344, 0.000977, 0.507324, 0.878418, 0.843262, 0, 0 },
    { 0, 1981, 1074, 20, 168, 9, 157, 0.967285, 0.524414, 0.977051, 0.606445, 0, 0 },
    { 0, 327, 1731, 364, 278, 12, 15, 0.159668, 0.845215, 0.337402, 0.980957, 0, 1 },
    { 0, 1803, 779, 243, 291, 84, 198, 0.880371, 0.380371, 0.999023, 0.522461, 1, 0 },
    { 0, 695, 1731, 321, 213, 173, 105, 0.339355, 0.845215, 0.496094, 0.949219, 1, 8 },
    { 0, 1803, 1252, 99, 118, 11, 12, 0.880371, 0.611328, 0.928711, 0.668945, 9, 0 },
    { 0, 2, 2, 869, 1033, 434, 548, 0.000977, 0.000977, 0.425293, 0.505371, 9, 0 },
};
