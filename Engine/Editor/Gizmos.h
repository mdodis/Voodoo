#pragma once
#include "Core/MathTypes.h"
namespace ed {
    namespace gizmos {
        bool box(
            Vec3 position, Quat rotation, Vec3 extents = Vec3::one() * 0.5f);
    }
}  // namespace ed
