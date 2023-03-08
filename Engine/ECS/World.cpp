#include "World.h"

void World::init()
{
    entities.logical.alloc = &allocator;
    entities.spatial.alloc = &allocator;
    components.alloc       = &allocator;
    registry.component_types.alloc = &allocator;
}

void World::register_component_type(ComponentType& type, size_t storage_size) {
    ComponentMetadata metadata = {
        .type = type,
        .size = storage_size,
    };
    registry.component_types.add(metadata);
}


EntityBase* create_entity(bool logical) {

    return nullptr;
}

void *create_component(const ComponentType& type) {

}

