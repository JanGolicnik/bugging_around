#define BUGGIN_SCENE
#include "base.c"

STRUCT(Scene) {
    GENARR(Entity) entities;
    EntityHandle root;
};

Entity* scene_get_entity(Scene* scene, EntityHandle handle) {
    return genarr_get(scene->entities, handle);
}

Entity* scene_root(Scene* scene) {
    return genarr_get(scene->entities, scene->root);
}

#define for_each_entity_children(e, val)\
    for (Entity* val = scene_get_entity((e)->scene, (e)->first_child); val; val = scene_get_entity((e)->scene, val->next_sibling))

void entity_set_hidden(Entity* entity, bool val) {
    if (val) entity_enable_components(entity, CT_IsHidden);
    else     entity_disable_components(entity, CT_IsHidden);
}

STRUCT(EntityIter) {
    GENARR_ITER_ALIAS(Entity, entity);
    ComponentType include;
    ComponentType any;
    ComponentType exclude;
};

bool scene_next_entity(Scene* scene, EntityIter* iter) {
    FLAG_SET(iter->exclude, CT_IsHidden);
    FLAG_CLEAR(iter->exclude, iter->include);
    while (genarr_next_valid(scene->entities, iter)) {
        ComponentType comps = iter->entity->components;
        if (FLAG_HAS_ANY(comps, iter->exclude)) continue;
        if (iter->any && FLAG_HAS_ANY(comps, iter->any)) return true;
        if (!FLAG_HAS_ALL(comps, iter->include)) continue;
        return true;
    }
    return false;
}

typedef enum {
    ETU_LOCAL,
    ETU_WORLD,
} EntityTransformUpdate;

Transform transform_calculate_world(Transform parent, Transform local) {
    return (Transform) {
      .pos = vec2_add(parent.pos, vec2_mul(vec2_rotate(local.pos, parent.rot), parent.scale)),
      .rot = parent.rot + local.rot,
      .scale = vec2_mul(parent.scale, local.scale),
    };
}

Transform transform_calculate_local(Transform parent, Transform world) {
    return (Transform){
      .pos = vec2_sub(vec2_div(vec2_rotate(world.pos, -parent.rot), parent.scale), parent.pos),
      .rot = world.rot - parent.rot,
      .scale = vec2_div(world.scale, parent.scale),
    };
}

void entity_transform_apply(Entity* entity, Entity* parent, EntityTransformUpdate update) {
    bool apply_parent = parent && entity_has(parent, CT_Transform);
    if (update == ETU_LOCAL)
    {
        entity->world = apply_parent ? transform_calculate_world(parent->world, entity->local) : entity->local;
        // entity->_matrix = mat4_from_transform(&entity->world);
    }
    else
    {
        entity->local = apply_parent ? transform_calculate_local(parent->world, entity->world) : entity->world;
    }

    for_each_entity_children(entity, child) {
        if (entity_has(child, CT_IsHidden)) continue;
        entity_transform_apply(child, entity, ETU_LOCAL);
    }
}

void entity_apply_local(Entity* entity) {
    entity_transform_apply(entity, scene_get_entity(entity->scene, entity->parent), ETU_LOCAL);
}

void entity_apply_world(Entity* entity) {
    entity_transform_apply(entity, scene_get_entity(entity->scene, entity->parent), ETU_WORLD);
}

EntityHandle _scene_create_entity(Scene* scene, EntityHandle handle) {
    Entity* entity = scene_get_entity(scene, handle);
    entity->scene = scene;

    if (!entity->parent.valid) entity->parent = scene->root;

    if (entity->parent.valid)
    {
        Entity* parent = scene_get_entity(scene, entity->parent);
        entity->next_sibling = parent->first_child;
        parent->first_child = handle;
    }

    if (entity_has(entity, CT_Transform))
    {
        if (vec2_norm2(entity->world.scale) == 0.0f)
        {
            if (vec2_norm2(entity->local.scale) == 0.0f) entity->local.scale = VEC2_ONE;
            entity_apply_local(entity);
        }
        else
        {
            if (vec2_norm2(entity->world.scale) == 0.0f) entity->world.scale = VEC2_ONE;
            entity_apply_world(entity);
        }
    }

    return handle;
}

#define scene_create_entity(_scene, _comps, ...)\
    _scene_create_entity((_scene), genarr_add((_scene)->entities, (Entity){ .components = (_comps), __VA_ARGS__ }))
