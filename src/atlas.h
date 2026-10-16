#pragma once

#include <marrow/marrow.h>

#define ATLAS_PAGE_COUNT 1
#define ATLAS_PAGE_W 2048
#define ATLAS_PAGE_H 2048

typedef enum {
    SpriteTag_None,
    SpriteTag_origin,
    SpriteTag_attach,
    SpriteTag_Count
} SpriteTag;

typedef enum {
    SPRITE_S1,
    SPRITE_S2,
    SPRITE_COUNT
} SpriteId;

static struct {i32 page, x, y, w, h; f64 u0, v0, u1, v1; struct { SpriteTag tag; f32 x, y; } tags[128]; } sprites[SPRITE_COUNT] = {
    {0, 249, 2, 364, 278, 0.121582, 0.000977, 0.299316, 0.136719, { { .tag = SpriteTag_origin, .x = 12.500000, .y = 12.000000 }, { .tag = SpriteTag_attach, .x = 351.500000, .y = 204.000000 },  }},
    {0, 2, 2, 243, 291, 0.000977, 0.000977, 0.119629, 0.143066, { { .tag = SpriteTag_origin, .x = 84.774902, .y = 195.126999 },  }},
};

