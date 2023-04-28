#pragma once
#include "Containers/Map.h"
#include "ECSTypes.h"

static _inline u64 hash_of(const IDescriptor& descriptor, u32 seed)
{
    Str type_name = descriptor.type_name();
    return hash_of(type_name, seed);
}

static _inline bool operator==(
    const IDescriptor& left, const IDescriptor& right)
{
    return left.type_name() == right.type_name();
}

struct SerializedComponent {
    Str        type_name;
    /** An inline archive of the serialized component's data */
    TArray<u8> data;
};

struct SerializedComponentDescriptor : IDescriptor {
    StrDescriptor type_name_desc = {
        OFFSET_OF(SerializedComponent, type_name), LIT("type")};
    ArrayDescriptor<u8> data_desc = {
        OFFSET_OF(SerializedComponent, data), LIT("data")};

    IDescriptor* descs[2] = {
        &type_name_desc,
        &data_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(SerializedComponent, descs)
};
DEFINE_DESCRIPTOR_OF_INL(SerializedComponent)

struct SerializedEntity {
    SerializedEntity() {}
    SerializedEntity(Str name, IAllocator& allocator)
        : name(name), components(&allocator)
    {}
    Str                         name;
    TArray<SerializedComponent> components;
};

struct SerializedEntityDescriptor : IDescriptor {
    StrDescriptor name_desc = {OFFSET_OF(SerializedEntity, name), LIT("name")};
    ArrayDescriptor<SerializedComponent> components_desc = {
        OFFSET_OF(SerializedEntity, components), LIT("components")};

    IDescriptor* descs[2] = {
        &name_desc,
        &components_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(SerializedEntity, descs);
};
DEFINE_DESCRIPTOR_OF_INL(SerializedEntity);

struct SerializedWorld {
    SerializedWorld() {}
    SerializedWorld(IAllocator& allocator) : entities(&allocator) {}

    TArray<SerializedEntity> entities;
};

struct SerializedWorldDescriptor : IDescriptor {
    ArrayDescriptor<SerializedEntity> entities_desc = {
        OFFSET_OF(SerializedWorld, entities), LIT("entities")};

    IDescriptor* descs[1] = {
        &entities_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(SerializedWorld, descs)
};
DEFINE_DESCRIPTOR_OF_INL(SerializedWorld);

struct WorldSerializer {
    void init(IAllocator& allocator);
    void register_descriptor(u64 id, IDescriptor* descriptor);
    void import(flecs::world& world, Str path);
    void save(const flecs::world& world, Str path);
    void deinit();

    TMap<u64, IDescriptor*> id_to_desc;
    TMap<Str, u64>          type_name_to_id;
};
