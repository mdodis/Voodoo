#include "Mesh.h"

#include "Containers/Extras.h"
#include "tiny_obj_loader.h"

VertexInputInfo Vertex::get_input_info(IAllocator& allocator)
{
    VertexInputInfo result = {
        .bindings = alloc_slice<VkVertexInputBindingDescription>(allocator, 1),
        .attributes =
            alloc_slice<VkVertexInputAttributeDescription>(allocator, 4),
    };

    result.bindings[0] = {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    result.attributes[0] = {
        .location = 0,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, position),
    };

    result.attributes[1] = {
        .location = 1,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, normal),
    };

    result.attributes[2] = {
        .location = 2,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, color),
    };

    result.attributes[3] = {
        .location = 3,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32_SFLOAT,
        .offset   = OFFSET_OF(Vertex, uv),
    };

    return result;
}

Result<Mesh, Str> Mesh::load_from_file(IAllocator& allocator, const char* path)
{
    std::string                      warn, err;
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    TArray<Vertex> vertices(&allocator);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path, 0, true)) {
        return Err(Str(err.c_str()));
    }

    if (shapes.size() == 0) {
        return Err(LIT("No shapes in model"));
    }

    for (size_t s = 0; s < shapes.size(); ++s) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
            int fv = 3;

            for (size_t v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                // vertex position
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                // vertex normal
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                tinyobj::real_t ux =
                    attrib.texcoords[2 * idx.texcoord_index + 0];
                tinyobj::real_t uy =
                    attrib.texcoords[2 * idx.texcoord_index + 1];

                // copy it into our vertex
                Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;

                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                new_vert.uv.x = ux;
                new_vert.uv.y = 1 - uy;

                // we are setting the vertex color as the vertex normal. This is
                // just for display purposes
                new_vert.color = new_vert.normal;

                vertices.add(new_vert);
            }
            index_offset += fv;
        }
    }

    return Ok(Mesh{
        .vertices = slice(vertices),
    });
}
