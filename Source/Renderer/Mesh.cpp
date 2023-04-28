#include "Mesh.h"

#include "Containers/Extras.h"

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

Result<Mesh, EAssetLoadError> Mesh::load_from_asset(
    IAllocator& allocator, Str path)
{
    static_assert(sizeof(Vertex_P3fN3fC3fU2f) == sizeof(Vertex));
    auto load_result = Asset::load(allocator, path);

    if (!load_result.ok()) {
        return Err(load_result.err());
    }

    Asset asset = load_result.value();

    ASSERT(asset.info.kind == AssetKind::Mesh);
    ASSERT(asset.info.mesh.format == VertexFormat::P3fN3fC3fU2f);

    Slice<Vertex> vertices = slice<Vertex>(
        (Vertex*)asset.blob.ptr,
        asset.info.mesh.vertex_buffer_size);

    Slice<u32> indices = slice<u32>(
        (u32*)(asset.blob.ptr + asset.info.mesh.vertex_buffer_size * sizeof(Vertex)),
        asset.info.mesh.index_buffer_size);

    return Ok(Mesh{
        .vertices = vertices,
        .indices  = indices,
    });
}
