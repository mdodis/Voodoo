#pragma once
#include "Core/Color.h"
#include "Core/MathTypes.h"

namespace ed {
    namespace gizmos {
        static thread_local Vec3 Dummy_Intersection;

        bool box(
            Vec3  position,
            Quat  rotation,
            Vec3  extents,
            Color default_color,
            Color hovered_color,
            Vec3& intersection = Dummy_Intersection);

        void translation(Vec3& position, Quat rotation);
    }  // namespace gizmos
}  // namespace ed
