#include "WorldSerializer.h"

void WorldSerializer::init(IAllocator& allocator)
{
    id_to_desc.init(allocator);
}

void WorldSerializer::register_descriptor(u64 id, IDescriptor* descriptor)
{
    id_to_desc.add(id, descriptor);
}

void WorldSerializer::save(const flecs::world& world) {}

void WorldSerializer::deinit() { id_to_desc.release(); }
