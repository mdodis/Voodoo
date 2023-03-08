#pragma once

#include "BlockList.h"
#include "Containers/Array.h"
#include "Entity.h"
#include "Component.h"

struct ComponentContainer {
    ComponentContainer(IAllocator& allocator, u32 component_size)
        : list(allocator, component_size)
    {}
    u32                component_type_id;
    BlockList<u32, 64> list;
};

struct World {
    World(IAllocator& in_allocator) : allocator(in_allocator) {}


    // Lifetime
    void        init();

    // Registry
    void register_component_type(ComponentType& type, size_t storage_size);

    template <typename ComponentStorageType>
    void register_native_component_type() {
        ComponentType new_type = {
            .provider = ComponentTypeProvider::Native,
            .id = typeid(ComponentStorageType).hash_code(),
            .size = sizeof(ComponentStorageType)
        };
        register_component_type(new_type);
    }

    // Entity creation
    EntityBase* create_entity(bool logical);
    void *create_component(const ComponentType& type);

    LogicalEntity* create_logical_entity()
    {
        return (LogicalEntity*)create_entity(true);
    }

    SpatialEntity* create_spatial_entity()
    {
        return (SpatialEntity*)create_entity(false);
    }

    struct {
        TArray<LogicalEntity> logical;
        TArray<SpatialEntity> spatial;
    } entities;

    TArray<ComponentContainer> components;

    struct {
        TArray<ComponentMetadata> component_types;
    } registry;

    IAllocator&                allocator;
};