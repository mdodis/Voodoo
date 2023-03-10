#pragma once
#include <glm/glm.hpp>

#include "Component.h"
#include "Archetype.h"

struct EntityBase {
    bool pending_deletion = false;
    ArchetypeReference archetype_id;
};

struct LogicalEntity : EntityBase {};

struct SpatialEntity : EntityBase {};

struct EntityReference {
    u32 id;
};