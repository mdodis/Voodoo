#pragma once
#include <glm/glm.hpp>

#include "Containers/Slice.h"
#include "Types.h"

namespace core {

    struct MeshBounds {
        glm::vec3 origin;
        float     radius;
        glm::vec3 extents;
    };

    namespace VertexFormat {
        enum Type : u32
        {
            Unknown      = 0,
            /**
             * float position[3];
             * float normal[3];
             * float color[3];
             * float uv[2];
             */
            P3fN3fC3fU2f = 1,
        };
    }
    typedef VertexFormat::Type EVertexFormat;

    struct Vertex_P3fN3fC3fU2f {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;
    };

    template <typename T>
    _inline void pack_vertex(
        T&    v,
        float vx,
        float vy,
        float vz,
        float nx,
        float ny,
        float nz,
        float ux,
        float uy,
        float cx = 1.0f,
        float cy = 1.0f,
        float cz = 1.0f);

    template <>
    _inline void pack_vertex(
        Vertex_P3fN3fC3fU2f& v,
        float                vx,
        float                vy,
        float                vz,
        float                nx,
        float                ny,
        float                nz,
        float                ux,
        float                uy,
        float                cx,
        float                cy,
        float                cz)
    {
        v.position = glm::vec3(vx, vy, vz);
        v.normal   = glm::vec3(nx, ny, nz);
        v.color    = glm::vec3(cx, cy, cz);
        v.uv       = glm::vec2(ux, uy);
    }

    MeshBounds calculate_mesh_bounds(
        const Slice<Vertex_P3fN3fC3fU2f>& vertices);

}  // namespace core

PROC_FMT_ENUM(core::VertexFormat, {
    FMT_ENUM_CASE(core::VertexFormat, Unknown);
    FMT_ENUM_CASE(core::VertexFormat, P3fN3fC3fU2f);
    FMT_ENUM_DEFAULT_CASE(Unknown);
})

PROC_PARSE_ENUM(core::VertexFormat, {
    PARSE_ENUM_CASE(core::VertexFormat, Unknown);
    PARSE_ENUM_CASE(core::VertexFormat, P3fN3fC3fU2f);
})
