#include "Containers/Extras.h"
#include "DefaultImporters.h"
#include "Memory/Extras.h"
#include "tiny_obj_loader.h"

static thread_local IAllocator* Local_Allocator;
static Str                      File_Extensions[] = {".obj"};

PROC_IMPORTER_IMPORT(tinyobjloader_import)
{
    Local_Allocator = &allocator;
    CREATE_SCOPED_ARENA(Local_Allocator, temp, 1024);

    Str path_cstr = format(temp, LIT("{}\0"), path);

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      err;

    bool success = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &err,
        path_cstr.data,
        nullptr);

    if (!success) {
        return Err(format(
            allocator,
            LIT("Failed to load mesh {} {}"),
            path,
            err.c_str()));
    }

    if (shapes.size() == 0) {
        return Err(LIT("No shapes in model"));
    }

    // @note: using stl because we don't have a variable memory allocator
    // implementation
    std::vector<Vertex_P3fN3fC3fU2f> vertices;
    std::vector<u32>                 indices;

    for (size_t s = 0; s < shapes.size(); ++s) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
            // hardcode face vertices to triangles
            int fv = 3;

            for (size_t v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                // Position
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

                // Normal
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                // UV
                tinyobj::real_t ux =
                    attrib.texcoords[2 * idx.texcoord_index + 0];
                tinyobj::real_t uy =
                    attrib.texcoords[2 * idx.texcoord_index + 1];

                Vertex_P3fN3fC3fU2f vertex;
                pack_vertex(
                    vertex,
                    vx,
                    vy,
                    vz,
                    nx,
                    ny,
                    nz,
                    ux,
                    1.f - uy,
                    nx,
                    ny,
                    nz);

                indices.push_back((u32)vertices.size());
                vertices.push_back(vertex);
            }

            index_offset += fv;
        }
    }

    const size_t vertices_size = vertices.size() * sizeof(Vertex_P3fN3fC3fU2f);
    const size_t indices_size  = indices.size() * sizeof(u32);
    size_t       total_size    = vertices_size + indices_size;
    auto         blob          = alloc_slice<u8>(allocator, total_size);

    memcpy(blob.ptr, &vertices[0], vertices_size);
    memcpy(blob.ptr + vertices_size, &indices[0], indices_size);

    return Ok(Asset{
        .info =
            {
                .version     = 1,
                .kind        = AssetKind::Mesh,
                .compression = AssetCompression::None,
                .actual_size = total_size,
                .mesh =
                    {
                        .vertex_buffer_size = vertices.size(),
                        .index_buffer_size  = indices.size(),
                        .bounds             = calculate_mesh_bounds(
                            slice(&vertices[0], vertices.size())),
                        .format = VertexFormat::P3fN3fC3fU2f,
                    },
            },
        .blob = blob,
    });
}

Importer Tiny_OBJ_Loader_Importer = {
    .name = LIT("tinyobjloader"),
    .kind = AssetKind::Mesh,
    .file_extensions =
        Slice<Str>(File_Extensions, ARRAY_COUNT(File_Extensions)),
    .import = tinyobjloader_import,
};