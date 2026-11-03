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
    SpriteTag_turret,
    SpriteTag_Count
} SpriteTag;

typedef enum {
    SPRITE_BUGGIN____AROUND,
    SPRITE_ARENA,
    SPRITE_CANNON1,
    SPRITE_CANNON1_BARREL,
    SPRITE_CANNON2,
    SPRITE_CANNON2_BARREL,
    SPRITE_CANNON3,
    SPRITE_CANNON3_BARREL,
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
    { SpriteTag_turret, 2.000000, 0.000000, 1.570000 },
    { SpriteTag_turret, 2.000000, -11.000000, 1.570000 },
    { SpriteTag_turret, 2.000000, 11.000000, 1.570000 },
    { SpriteTag_follow, -40.333313, 0.716003, 2.819050 },
    { SpriteTag_spine_small, -31.000000, 1.715973, 1.570003 },
    { SpriteTag_spine_big, -69.000000, 0.715973, 1.570003 },
    { SpriteTag_spine_big, -168.000000, 0.715973, 1.570003 },
    { SpriteTag_spine_big, -272.000000, 0.715973, 1.570003 },
    { SpriteTag_spine_medium, -388.000000, 1.715973, 1.570003 },
    { SpriteTag_spine_tail, -462.000000, 1.715973, 1.570003 },
    { SpriteTag_follow, -117.833336, 0.000000, 2.819046 },
    { SpriteTag_leg_right, -58.500000, 71.000000, 3.140796 },
    { SpriteTag_leg_left, -58.500000, -70.000000, -0.000796 },
    { SpriteTag_follow, -78.333336, -1.000000, 2.819046 },
    { SpriteTag_leg_right, -35.000000, 56.000000, 3.140796 },
    { SpriteTag_leg_left, -35.000000, -57.000000, -0.000796 },
    { SpriteTag_follow, -48.201527, 0.395996, 2.819046 },
    { SpriteTag_leg_right, -21.868164, 62.395996, 3.140799 },
    { SpriteTag_leg_left, -21.868164, -61.603935, -0.000799 },
    { SpriteTag_leg_right, -26.016983, 44.000000, 3.140798 },
    { SpriteTag_leg_left, -26.016983, -45.000000, -0.000798 },
    { SpriteTag_leg_right, 75.843674, -98.552872, 4.460490 },
    { SpriteTag_leg_left, -2.764664, -114.504196, 2.536845 },
    { SpriteTag_leg_left, -96.000992, -100.572266, 1.569997 },
    { SpriteTag_leg_left, 75.843674, 59.759689, -1.320488 },
    { SpriteTag_leg_right, -2.764664, 75.711319, 0.603140 },
    { SpriteTag_leg_right, -96.000992, 61.779007, 1.570000 },
    { SpriteTag_turret, -45.341003, -56.230331, 1.570028 },
    { SpriteTag_turret, -45.341003, 58.769653, 1.570028 },
    { SpriteTag_leg_right, 14.873001, 63.699005, 0.860985 },
    { SpriteTag_leg_right, 38.728683, 60.332001, -0.373246 },
    { SpriteTag_leg_right, -59.236000, 79.641998, 1.570000 },
    { SpriteTag_leg_left, 14.873001, -61.048996, 2.279015 },
    { SpriteTag_leg_left, 38.728683, -57.682007, 3.513247 },
    { SpriteTag_leg_left, -59.236000, -76.991997, 1.570000 },
};

static const SpriteInfo sprites[SPRITE_COUNT] = {
    { 1, 1078, 2, 878, 478, 464, 242, 0.526367, 0.000977, 0.955078, 0.234375, 0, 0 },
    { 0, 875, 2, 1038, 773, 519, 386, 0.427246, 0.000977, 0.934082, 0.378418, 0, 0 },
    { 0, 1889, 1623, 100, 100, 48, 50, 0.922363, 0.792480, 0.971191, 0.841309, 0, 1 },
    { 0, 1955, 1114, 47, 143, 24, 127, 0.954590, 0.543945, 0.977539, 0.613770, 1, 0 },
    { 0, 1889, 1517, 104, 102, 48, 51, 0.922363, 0.740723, 0.973145, 0.790527, 1, 2 },
    { 0, 915, 1731, 143, 34, 15, 17, 0.446777, 0.845215, 0.516602, 0.861816, 3, 0 },
    { 0, 1889, 1391, 124, 122, 116, 61, 0.922363, 0.679199, 0.982910, 0.738770, 3, 0 },
    { 0, 1889, 1727, 111, 98, 17, 49, 0.922363, 0.843262, 0.976562, 0.891113, 3, 0 },
    { 1, 2, 2, 561, 552, 468, 275, 0.000977, 0.000977, 0.274902, 0.270508, 3, 7 },
    { 0, 1803, 957, 148, 169, 132, 82, 0.880371, 0.467285, 0.952637, 0.549805, 10, 3 },
    { 0, 1803, 1130, 108, 141, 92, 69, 0.880371, 0.551758, 0.933105, 0.620605, 13, 3 },
    { 0, 1955, 957, 79, 153, 62, 74, 0.954590, 0.467285, 0.993164, 0.541992, 16, 3 },
    { 0, 1803, 1275, 63, 115, 31, 103, 0.880371, 0.622559, 0.911133, 0.678711, 19, 0 },
    { 0, 652, 1731, 259, 117, 243, 57, 0.318359, 0.845215, 0.444824, 0.902344, 19, 2 },
    { 0, 1803, 779, 174, 174, 87, 87, 0.880371, 0.380371, 0.965332, 0.465332, 21, 0 },
    { 1, 567, 2, 507, 507, 253, 253, 0.276855, 0.000977, 0.524414, 0.248535, 21, 0 },
    { 1, 1078, 484, 865, 476, 432, 238, 0.526367, 0.236328, 0.948730, 0.468750, 21, 0 },
    { 0, 2, 1731, 321, 227, 160, 133, 0.000977, 0.845215, 0.157715, 0.956055, 21, 6 },
    { 0, 1803, 1394, 39, 53, 12, 14, 0.880371, 0.680664, 0.899414, 0.706543, 27, 0 },
    { 0, 2, 1039, 1797, 688, 936, 344, 0.000977, 0.507324, 0.878418, 0.843262, 27, 0 },
    { 0, 1062, 1731, 168, 20, 11, 9, 0.518555, 0.845215, 0.600586, 0.854980, 27, 0 },
    { 0, 327, 1731, 321, 213, 173, 105, 0.159668, 0.845215, 0.316406, 0.949219, 27, 8 },
    { 0, 1915, 1261, 98, 126, 11, 12, 0.935059, 0.615723, 0.982910, 0.677246, 35, 0 },
    { 0, 2, 2, 869, 1033, 434, 548, 0.000977, 0.000977, 0.425293, 0.505371, 35, 0 },
};
