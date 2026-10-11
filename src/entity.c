#define BUGGIN_ENTITY
#include "base.c"

typedef enum {
    CT_Transform = BIT(0),
    CT_IsHidden = BIT(1),
    CT_Sprite = BIT(2),
    CT_Physics = BIT(3),
} ComponentType;

typedef struct Scene Scene;

typedef GenarrHandle EntityHandle;

STRUCT(TransformC)
{
    f32 x, y, w, h;
};

STRUCT(Entity) {
    str name;
    Scene* scene;
    EntityHandle parent, first_child, next_sibling;

    ComponentType components;

    TransformC transform;

    struct SpriteC {
        u32 sprite;
    } sprite;

    struct PhysicsC {
        f32 vel_x, vel_y;
    } physics;
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
