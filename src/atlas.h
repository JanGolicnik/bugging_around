#pragma once

#include <marrow/marrow.h>

#define ATLAS_PAGE_COUNT 2
#define ATLAS_SIZE 2048
typedef enum {
    SpriteTag_None,
    SpriteTag_follow,
    SpriteTag_leg_left,
    SpriteTag_leg_right,
    SpriteTag_spine_big,
    SpriteTag_spine_medium,
    SpriteTag_spine_small,
    SpriteTag_spine_tail,
    SpriteTag_Count
} SpriteTag;

typedef enum {
    SPRITE_BUGGIN____AROUND,
    SPRITE_ARENA,
    SPRITE_CENTIPEDE,
    SPRITE_CENTIPEDE_BODY_BIG,
    SPRITE_CENTIPEDE_BODY_MEDIUM,
    SPRITE_CENTIPEDE_BODY_SMALL,
    SPRITE_CENTIPEDE_LEG,
    SPRITE_CENTIPEDE_TAIL,
    SPRITE_DPAD_PAD,
    SPRITE_DPAG_BG,
    SPRITE_ENTER,
    SPRITE_LADYBUG,
    SPRITE_LADYBUG_LEG,
    SPRITE_LEAVES,
    SPRITE_LINE,
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
    { SpriteTag_follow, -40.333313, 0.716003, 2.819050 },
    { SpriteTag_spine_small, -31.000000, 1.715973, 1.570003 },
    { SpriteTag_spine_big, -69.000000, 0.715973, 1.570003 },
    { SpriteTag_spine_big, -168.000000, 0.715973, 1.570003 },
    { SpriteTag_spine_big, -272.000000, 0.715973, 1.570003 },
    { SpriteTag_spine_medium, -388.000000, 1.715973, 1.570003 },
    { SpriteTag_spine_tail, -462.000000, 1.715973, 1.570003 },
    { SpriteTag_follow, -118.833336, 0.000000, 2.819046 },
    { SpriteTag_leg_left, -57.499901, -61.000000, -0.000796 },
    { SpriteTag_leg_right, -57.499901, 61.000000, 3.140796 },
    { SpriteTag_follow, -78.333336, -1.000000, 2.819046 },
    { SpriteTag_leg_left, -36.401367, -62.000000, -0.000799 },
    { SpriteTag_leg_right, -36.401367, 60.000000, 3.140799 },
    { SpriteTag_follow, -48.201328, 0.000000, 2.819046 },
    { SpriteTag_leg_left, -23.269398, -61.000000, -0.000797 },
    { SpriteTag_leg_right, -23.269398, 61.000000, 3.140797 },
    { SpriteTag_leg_left, -31.418655, -47.000000, -0.000826 },
    { SpriteTag_leg_right, -31.418655, 47.000000, 3.140826 },
    { SpriteTag_leg_right, 75.843674, -98.552872, 4.460490 },
    { SpriteTag_leg_left, -2.764664, -114.504196, 2.536845 },
    { SpriteTag_leg_left, -96.000992, -100.572266, 1.569997 },
    { SpriteTag_leg_left, 75.843674, 59.759689, -1.320488 },
    { SpriteTag_leg_right, -2.764664, 75.711319, 0.603140 },
    { SpriteTag_leg_right, -96.000992, 61.779007, 1.570000 },
    { SpriteTag_leg_right, 14.873001, 63.699005, 0.860985 },
    { SpriteTag_leg_right, 38.729004, 60.332306, -0.373262 },
    { SpriteTag_leg_right, -59.235008, 79.641998, 1.570000 },
    { SpriteTag_leg_left, 14.873001, -61.048996, 2.279015 },
    { SpriteTag_leg_left, 38.729004, -57.682007, 3.513271 },
    { SpriteTag_leg_left, -59.235008, -76.991997, 1.570000 },
};

static const SpriteInfo sprites[SPRITE_COUNT] = {
    { 1, 1078, 2, 878, 478, 464, 242, 0.526367, 0.000977, 0.955078, 0.234375, 0, 0 },
    { 0, 875, 2, 1038, 773, 519, 386, 0.427246, 0.000977, 0.934082, 0.378418, 0, 0 },
    { 1, 2, 2, 561, 552, 468, 275, 0.000977, 0.000977, 0.274902, 0.270508, 0, 7 },
    { 0, 1803, 957, 148, 155, 132, 73, 0.880371, 0.467285, 0.952637, 0.542969, 7, 3 },
    { 0, 1803, 1116, 108, 146, 92, 74, 0.880371, 0.544922, 0.933105, 0.616211, 10, 3 },
    { 0, 1955, 957, 79, 147, 62, 73, 0.954590, 0.467285, 0.993164, 0.539062, 13, 3 },
    { 0, 1915, 1238, 115, 63, 10, 34, 0.935059, 0.604492, 0.991211, 0.635254, 16, 0 },
    { 0, 653, 1731, 259, 119, 243, 59, 0.318848, 0.845215, 0.445312, 0.903320, 16, 2 },
    { 0, 1803, 779, 174, 174, 87, 87, 0.880371, 0.380371, 0.965332, 0.465332, 18, 0 },
    { 1, 567, 2, 507, 507, 253, 253, 0.276855, 0.000977, 0.524414, 0.248535, 18, 0 },
    { 1, 1078, 484, 865, 476, 432, 238, 0.526367, 0.236328, 0.948730, 0.468750, 18, 0 },
    { 0, 2, 1731, 321, 227, 160, 133, 0.000977, 0.845215, 0.157715, 0.956055, 18, 6 },
    { 0, 1803, 1266, 39, 53, 12, 14, 0.880371, 0.618164, 0.899414, 0.644043, 24, 0 },
    { 0, 2, 1039, 1797, 688, 936, 344, 0.000977, 0.507324, 0.878418, 0.843262, 24, 0 },
    { 0, 1862, 1305, 168, 20, 11, 9, 0.909180, 0.637207, 0.991211, 0.646973, 24, 0 },
    { 0, 327, 1731, 322, 213, 173, 105, 0.159668, 0.845215, 0.316895, 0.949219, 24, 6 },
    { 0, 1915, 1116, 99, 118, 11, 12, 0.935059, 0.544922, 0.983398, 0.602539, 30, 0 },
    { 0, 2, 2, 869, 1033, 434, 548, 0.000977, 0.000977, 0.425293, 0.505371, 30, 0 },
};
