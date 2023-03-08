#include "World.h"

void World::init()
{
    entities.logical.allocator = &allocator;
    entities.spatial.allocator = &allocator;
    components.allocator       = &allocator;
}

EntityBase* create_entity(bool logical) { return nullptr; }
