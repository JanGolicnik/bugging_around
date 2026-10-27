#define BUGGIN_ENTITY
#include "base.c"

typedef enum {
    CT_Transform = BIT(0),
    CT_IsHidden = BIT(1),
    CT_Sprite = BIT(2),
    CT_Physics = BIT(3),
    CT_Ui = BIT(4),
} ComponentType;

typedef struct Scene Scene;

typedef GenarrHandle EntityHandle;

STRUCT(Transform)
{
    vec2s pos;
    vec2s scale;
    f32   rot;
};

typedef enum {
    Layer_Background,
    Layer_Guys,
    Layer_Ui,
    Layer_Count
} Layer;

STRUCT(Entity) {
    str name;
    Scene* scene;
    EntityHandle parent, first_child, next_sibling;

    ComponentType components;

    Transform local, world;

    struct {
        SpriteId id;
        Layer layer;
    } sprite;

    struct PhysicsC {
        f32 vel_x, vel_y;
    } physics;

    struct UiC {
        bool _;
    } ui;
};

void entity_enable_components(Entity* entity, ComponentType components) {
    FLAG_SET(entity->components, components);
}

void entity_disable_components(Entity* entity, ComponentType components) {
    FLAG_CLEAR(entity->components, components);
}

bool entity_has(Entity* entity, ComponentType components) {
    return FLAG_HAS_ALL(entity->components, components);
}

vec2s sprite_size(SpriteId id)
{
    return (vec2s){{ sprites[id].w, sprites[id].h }};
}

vec2s entity_sprite_size(Entity* e)
{
    return vec2_mul(sprite_size(e->sprite.id), e->world.scale);
}
