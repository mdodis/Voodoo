#pragma once
#include <glm/glm.hpp>

#include "Component.h"

struct EntityBase {
    bool pending_deletion = false;
};

struct LogicalEntity {};

struct SpatialEntity;

struct EntityReference {
    u32 id;
};