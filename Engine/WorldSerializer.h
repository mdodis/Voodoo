#pragma once
#include "Containers/Map.h"
#include "ECSTypes.h"

struct WorldSerializer {
    void init(IAllocator& allocator);
    void register_descriptor(u64 id, IDescriptor* descriptor);
    void save(const flecs::world& world);
    void deinit();

    TMap<u64, IDescriptor*> id_to_desc;
};
