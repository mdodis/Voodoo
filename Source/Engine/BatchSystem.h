#pragma once
#include "VulkanCommon/VulkanCommon.h"

struct Mesh;
struct MaterialInstance;
struct ShaderPass;

template <typename T>
struct NamedIndex {
    u32 index;

    bool operator==(const NamedIndex& other) { return index == other.index; }
};

struct DrawMesh {
    u32   first_vertex;
    u32   first_index;
    u32   index_count;
    u32   vertex_count;
    bool  is_merged;
    Mesh* original;
};

struct GPUIndirectObject {
    VkDrawIndexedIndirectCommand command;
    uint32_t                     object_id;
    uint32_t                     batch_id;
};

struct RenderObject {
    NamedIndex<DrawMesh>         mesh_id;
    NamedIndex<MaterialInstance> material;
    u32                          update_index;
    u32                          custom_sort_key = 0;
    i32                          pass_indices;
    Mat4                         transform_matrix;
    RenderBounds                 bounds;
};

struct GPUInstance {
    u32 object_id;
    u32 batch_id;
};

struct BatchSystem {
    struct PassMaterial {
        VkDescriptorSet material_set;
        ShaderPass*     shader_pass;

        bool operator==(const PassMaterial& other) const
        {
            return (material_set == other.material_set) &&
                   (shader_pass == other.shader_pass);
        }
    };

    struct PassObject {
        PassMaterial             material;
        NamedIndex<DrawMesh>     mesh_id;
        NamedIndex<RenderObject> original;
        i32                      built_batch;
        u32                      custom_key;
    };

    struct RenderBatch {
        NamedIndex<PassObject> pass_object;
        u64                    sort_key;

        bool operator==(const RenderBatch& other)
        {
            return (pass_object == other.pass_object) &&
                   (sort_key == other.sort_key);
        }
    };

    struct IndirectBatch {
        NamedIndex<DrawMesh> mesh_id;
        PassMaterial         material;
        u32                  first;
        u32                  count;
    };

    struct Multibatch {
        u32 first;
        u32 count;
    };

    struct MeshPass {
        TArray<Multibatch>               multibatches;
        TArray<IndirectBatch>            indirect_batches;
        TArray<NamedIndex<RenderObject>> unbatched_objects;
        TArray<RenderBatch>              flat_batches;
        TArray<PassObject>               objects;
        TArray<NamedIndex<PassObject>>   resuable_objects;
        TArray<NamedIndex<PassObject>>   objects_to_delete;

        AllocatedBuffer<u32>               compacted_instance_buffer;
        AllocatedBuffer<GPUInstance>       pass_objects_buffer;
        AllocatedBuffer<GPUIndirectObject> draw_indirect_buffer;
        AllocatedBuffer<GPUIndirectObject> clear_indirect_buffer;

        PassObject*  get(NamedIndex<PassObject> index);
        MeshPassType type;

        bool needs_indirect_refresh = true;
        bool needs_instance_refresh = true;
    };
};