#pragma once

#include "Component.h"
#include "Containers/Array.h"
#include "Entity.h"

struct World {
    World(IAllocator& in_allocator) : allocator(in_allocator) {}

    // Lifetime
    void init();

    // Registry
    void register_component_type(ComponentType& type);

    template <typename ComponentStorageType>
    void register_native_component_type()
    {
        ComponentType new_type =
            make_native_component_type<ComponentStorageType>();
        register_component_type(new_type);
    }

    EntityBase* create_entity(bool logical);

    // Components
    ComponentNativePtr allocate_component(const ComponentType& type);
    void               attach_component_to_entity(
                      ComponentNativePtr component, EntityBase* entity);

    template <typename ComponentStorageType>
    ComponentPtr<ComponentStorageType> create_component()
    {
        return ComponentPtr<ComponentStorageType>(allocate_component(
            make_native_component_type<ComponentStorageType>()));
    }

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

    // Internal
    ComponentMetadata*  find_component_metadata(const ComponentType& type);
    ComponentContainer* find_or_create_component_container(
        const ComponentType& type);

    IAllocator& allocator;
};