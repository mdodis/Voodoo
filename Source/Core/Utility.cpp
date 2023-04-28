#include "Utility.h"

#include <glm/gtx/norm.hpp>

#include "Traits.h"

namespace core {
    MeshBounds calculate_mesh_bounds(const Slice<Vertex_P3fN3fC3fU2f>& vertices)
    {
        glm::vec3 min =
            {NumProps<f32>::max, NumProps<f32>::max, NumProps<f32>::max};

        glm::vec3 max =
            {NumProps<f32>::min, NumProps<f32>::min, NumProps<f32>::min};

        for (u64 i = 0; i < vertices.count; ++i) {
            min.x = glm::min(min.x, vertices[i].position.x);
            min.y = glm::min(min.y, vertices[i].position.y);
            min.z = glm::min(min.z, vertices[i].position.z);

            max.x = glm::max(max.x, vertices[i].position.x);
            max.y = glm::max(max.y, vertices[i].position.y);
            max.z = glm::max(max.z, vertices[i].position.z);
        }

        MeshBounds result;

        result.extents = (max - min) / 2.0f;
        result.origin  = result.extents + min;

        f32 r2 = 0.0f;
        for (u64 i = 0; i < vertices.count; ++i) {
            float sqdistance =
                glm::distance2(vertices[i].position, result.origin);
            r2 = glm::max(r2, sqdistance);
        }
        result.radius = glm::sqrt(r2);

        return result;
    }

}  // namespace core