#pragma once

#include "BlockList.h"
#include "Containers/Array.h"
#include "Entity.h"

struct ComponentContainer {
    ComponentContainer(IAllocator& allocator, u32 component_size)
        : list(allocator, component_size)
    {}
    u32                component_type_id;
    BlockList<u32, 64> list;
};

struct World {
    World(IAllocator& in_allocator) : allocator(in_allocator) {}

    void        init();
    EntityBase* create_entity(bool logical);

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
    IAllocator&                allocator;
};