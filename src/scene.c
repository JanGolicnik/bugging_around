#define BUGGIN_SCENE
#include "base.c"

STRUCT(Scene) {
    GENARR(Entity) entities;
    EntityHandle player;
    EntityHandle camera;
    EntityHandle planets[2];
};

Entity* scene_get_entity(Scene* scene, EntityHandle handle) {
    return genarr_get((scene)->entities, handle);
}

#define for_each_entity_children(e, val)\
    for (Entity* val = scene_get_entity((e)->scene, (e)->first_child); val; val = scene_get_entity((e)->scene, val->next_sibling))

typedef enum {
    ETU_LOCAL,
    ETU_WORLD,
} EntityTransformUpdate;

void entity_set_hidden(Entity* entity, bool val) {
    if (val) {
        entity_enable_components(entity, CT_IsHidden);
    }
    else {
        entity_disable_components(entity, CT_IsHidden);
    }
}

EntityHandle _scene_create_entity(Scene* scene, EntityHandle handle) {
    Entity* entity = scene_get_entity(scene, handle);
    entity->scene = scene;

    if (entity->parent.valid) {
        Entity* parent = scene_get_entity(scene, entity->parent);
        entity->next_sibling = parent->first_child;
        parent->first_child = handle;
    }

    return handle;
}

#define scene_create_entity(_scene, _comps, ...)\
    _scene_create_entity((_scene), genarr_add((_scene)->entities, (Entity){ .components = (_comps), __VA_ARGS__ }))

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
