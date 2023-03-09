#include "World.h"

void World::init()
{
    entities.logical.alloc         = &allocator;
    entities.spatial.alloc         = &allocator;
    components.alloc               = &allocator;
    registry.component_types.alloc = &allocator;
}

void World::register_component_type(ComponentType& type)
{
    ComponentMetadata metadata = {
        .type = type,
    };
    registry.component_types.add(metadata);
}

EntityBase* World::create_entity(bool logical)
{
    // @todo: add to staging buffer first
    EntityBase* result = 0;
    if (logical) {
        result = entities.logical.add();
    } else {
        result = entities.spatial.add();
    }
    result->pending_deletion = false;
    result->components       = TArray<ComponentReference>(&allocator);
    return result;
}

ComponentNativePtr World::allocate_component(const ComponentType& type)
{
    // @todo: add to staging buffer first
    ComponentMetadata* metadata = find_component_metadata(type);
    ASSERT(
        metadata &&
        "Component metadata must exist before any call to create_component");
    ComponentContainer* container = find_or_create_component_container(type);

    ComponentContainer::List::Ptr ptr = container->list.allocate();

    return ComponentNativePtr{
        .ptr = container->list.ptr_to_data_ptr(ptr),
        .ref = ComponentReference{ptr.index},
    };
}

void World::attach_component_to_entity(
    ComponentNativePtr component, EntityBase* entity)
{
    entity->components.add(component.ref);
}

ComponentMetadata* World::find_component_metadata(const ComponentType& type)
{
    for (auto& metadata : registry.component_types) {
        if (metadata.type == type) {
            return &metadata;
        }
    }
    return 0;
}

ComponentContainer* World::find_or_create_component_container(
    const ComponentType& type)
{
    for (auto& container : components) {
        if (container.component_type == type) {
            return &container;
        }
    }

    ComponentContainer* tmp_result = components.add();
    ASSERT(tmp_result);
    ComponentContainer* result =
        new (tmp_result) ComponentContainer(allocator, type);

    return result;
}
