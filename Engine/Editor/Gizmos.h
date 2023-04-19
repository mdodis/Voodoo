#pragma once
#include "Core/MathTypes.h"
#include "Core/Color.h"

namespace ed {
    namespace gizmos {
        bool box(
            Vec3  position,
            Quat  rotation,
            Vec3  extents,
            Color default_color,
            Color hovered_color);

        void translation(Vec3& position, Quat rotation);
    }  // namespace gizmos
}  // namespace ed
