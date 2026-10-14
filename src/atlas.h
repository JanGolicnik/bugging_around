#pragma once

#include <marrow/marrow.h>

#define ATLAS_PAGE_COUNT 1
#define ATLAS_PAGE_W 2048
#define ATLAS_PAGE_H 2048

typedef enum {
    SPRITE_S1,
    SPRITE_S2,
    SPRITE_COUNT
} SpriteId;

static struct {i32 page, x, y, w, h; f64 u0, v0, u1, v1, ox, oy; } sprites[SPRITE_COUNT] = {
{0,249, 2, 364, 278,0.121582, 0.000977, 0.299316, 0.136719,12.500000, 12.000000},
{0,2, 2, 243, 291,0.000977, 0.000977, 0.119629, 0.143066,84.774902, 195.126999},
};

