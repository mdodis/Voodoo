#pragma once
#include <glm/glm.hpp>

#include "Component.h"

struct EntityBase {
    bool pending_deletion = false;
    TArray<ComponentReference> components;
};

struct LogicalEntity : EntityBase {};

struct SpatialEntity : EntityBase {};

struct EntityReference {
    u32 id;
};