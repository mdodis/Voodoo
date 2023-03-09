#pragma once
#include <glm/glm.hpp>

#include "Component.h"

struct EntityBase {
    bool                       pending_deletion = false;
    TArray<ComponentReference> components;
};

struct LogicalEntity : EntityBase {};

struct SpatialEntity : EntityBase {
    glm::vec3 test;
};

struct EntityReference {
    u32 id;
};