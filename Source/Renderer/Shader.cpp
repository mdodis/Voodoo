#include "Shader.h"

#include <spirv_reflect.h>

#include <sstream>
#include <string>

#include "Sort.h"
#include "VulkanCommon/VulkanCommon.h"

void ShaderEffect::init(Allocator& allocator)
{
    bindings.init(allocator);
    stages.alloc = &allocator;
}

Result<ShaderModule, VkResult> ShaderModule::load(VkDevice device, Str path)
{
    Raw data = dump_file(path, System_Allocator);
    DEFER(System_Allocator.release((umm)data.buffer));

    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .codeSize = data.size,
        .pCode    = (u32*)data.buffer,
    };

    VkShaderModule result;
    VK_RETURN_IF_ERR(
        vkCreateShaderModule(device, &create_info, nullptr, &result));

    return Ok(ShaderModule{
        .code = Slice<u8>((u8*)data.buffer, data.size),
        .mod  = result,
    });
}

u64 hash_of(VkDescriptorSetLayoutCreateInfo& info, u32 seed)
{
    // @todo: Maybe we need a buffer builder in MokLib or sth similar?
    std::stringstream ss{};
    ss << info.flags;
    ss << info.bindingCount;

    for (auto i = 0u; i < info.bindingCount; ++i) {
        const VkDescriptorSetLayoutBinding& binding = info.pBindings[i];
        ss << binding.binding;
        ss << binding.descriptorCount;
        ss << binding.descriptorType;
        ss << binding.stageFlags;
    }

    auto str = ss.str();

    return murmur_hash2(str.c_str(), str.length(), seed);
}

void ShaderEffect::add_stage(ShaderModule* mod, VkShaderStageFlagBits flags)
{
    ShaderEffect::Stage new_stage = {
        .mod   = mod,
        .stage = flags,
    };

    stages.add(new_stage);
}

struct DescriptorSetLayoutData {
    DescriptorSetLayoutData() {}
    DescriptorSetLayoutData(Allocator& allocator) : bindings(&allocator) {}
    u32                                  set_number;
    VkDescriptorSetLayoutCreateInfo      create_info;
    TArray<VkDescriptorSetLayoutBinding> bindings;
};

void ShaderEffect::reflect_layout(
    VkDevice device, const Slice<MetadataOverride> overrides)
{
    CREATE_SCOPED_ARENA(System_Allocator, temp, MEGABYTES(1));

    TArray<DescriptorSetLayoutData> set_layouts(&System_Allocator);
    DEFER(set_layouts.release());
    TArray<VkPushConstantRange> constant_ranges(&System_Allocator);
    DEFER(constant_ranges.release());

    for (Stage& stage : stages) {
        SpvReflectShaderModule spv_mod;
        SpvReflectResult       result = spvReflectCreateShaderModule(
            stage.mod->code.count,
            stage.mod->code.ptr,
            &spv_mod);

        ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        // Descriptor Sets
        u32 count = 0;
        result = spvReflectEnumerateDescriptorSets(&spv_mod, &count, nullptr);
        ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
        TArray<SpvReflectDescriptorSet*> sets(&temp);
        sets.init_range(count);

        result = spvReflectEnumerateDescriptorSets(&spv_mod, &count, sets.data);
        ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        for (u64 i = 0; i < sets.size; ++i) {
            const SpvReflectDescriptorSet& set = *(sets[i]);

            DescriptorSetLayoutData layout(temp);

            layout.bindings.init_range(set.binding_count);

            for (u64 b = 0; b < set.binding_count; ++b) {
                const SpvReflectDescriptorBinding& binding = *(set.bindings[b]);

                VkDescriptorSetLayoutBinding& layout_binding =
                    layout.bindings[b];
                layout_binding.binding = binding.binding;
                layout_binding.descriptorType =
                    (VkDescriptorType)binding.descriptor_type;

                for (const MetadataOverride& override : overrides) {
                    if (Str(binding.name) == override.name) {
                        layout_binding.descriptorType = override.overriden_type;
                    }
                }

                layout_binding.descriptorCount = 1;
                for (u32 dim_index = 0; dim_index < binding.array.dims_count;
                     ++dim_index)
                {
                    layout_binding.descriptorCount *=
                        binding.array.dims[dim_index];
                }

                layout_binding.stageFlags =
                    static_cast<VkShaderStageFlagBits>(spv_mod.shader_stage);

                ShaderEffect::BindingMetadata metadata = {
                    .set     = set.set,
                    .binding = layout_binding.binding,
                    .type    = layout_binding.descriptorType,
                };

                bindings
                    .add(Str(binding.name).clone(System_Allocator), metadata);
            }

            layout.set_number = set.set;
            layout.create_info.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout.create_info.bindingCount = set.binding_count;
            layout.create_info.pBindings    = layout.bindings.data;

            set_layouts.add(layout);
        }

        result =
            spvReflectEnumeratePushConstantBlocks(&spv_mod, &count, nullptr);
        ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        TArray<SpvReflectBlockVariable*> constants(&temp);
        constants.init_range(count);
        result =
            spvReflectEnumeratePushConstantBlocks(&spv_mod, &count, nullptr);
        ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

        if (count > 0) {
            VkPushConstantRange pcs = {
                .stageFlags = (VkShaderStageFlags)stage.stage,
                .offset     = constants[0]->offset,
                .size       = constants[0]->size,
            };

            constant_ranges.add(pcs);
        }
    }

    TArr<DescriptorSetLayoutData, 4> merged_layouts;

    for (int i = 0; i < 4; ++i) {
        DescriptorSetLayoutData& ly = merged_layouts[i];
        ly.bindings.alloc           = &temp;

        ly.set_number = i;
        ly.create_info.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        int num_added = 0;

        TMap<u32, VkDescriptorSetLayoutBinding> binds(temp);
        for (DescriptorSetLayoutData& s : set_layouts) {
            if (s.set_number == i) {
                for (VkDescriptorSetLayoutBinding& b : s.bindings) {
                    if (binds.contains(b.binding)) {
                        binds[b.binding].stageFlags |= b.stageFlags;
                    } else {
                        b.pImmutableSamplers = nullptr;
                        binds.add(b.binding, b);
                        num_added++;
                    }
                }
            }
        }

        for (auto pair : binds) {
            ly.bindings.add(pair.val);
        }

        ASSERT(num_added == ly.bindings.size);

        Slice<VkDescriptorSetLayoutBinding> s = slice(ly.bindings);
        sort::quicksort(
            s,
            sort::CompareFunc<VkDescriptorSetLayoutBinding>::create_lambda(
                [](const VkDescriptorSetLayoutBinding& left,
                   const VkDescriptorSetLayoutBinding& right) {
                    return left.binding < right.binding;
                }));

        ly.create_info.bindingCount = (u32)ly.bindings.size;
        ly.create_info.pBindings    = ly.bindings.data;
        ly.create_info.flags        = 0;
        ly.create_info.pNext        = nullptr;

        if (ly.create_info.bindingCount > 0) {
            set_hashes[i]                 = hash_of(ly.create_info, 0x1337);
            VkDescriptorSetLayout& target = this->set_layouts[i];
            VK_CHECK(vkCreateDescriptorSetLayout(
                device,
                &ly.create_info,
                nullptr,
                &target));
        } else {
            set_hashes[i]        = 0;
            this->set_layouts[i] = VK_NULL_HANDLE;
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info =
        make_pipeline_layout_create_info();

    pipeline_layout_info.pPushConstantRanges    = constant_ranges.data;
    pipeline_layout_info.pushConstantRangeCount = (u32)constant_ranges.size;

    TArr<VkDescriptorSetLayout, 4> compacted_layouts;
    int                            s = 0;
    for (int i = 0; i < 4; ++i) {
        if (this->set_layouts[i] != VK_NULL_HANDLE) {
            compacted_layouts[s] = this->set_layouts[i];
            s++;
        }
    }

    pipeline_layout_info.setLayoutCount = s;
    pipeline_layout_info.pSetLayouts    = compacted_layouts.elements;
    VK_CHECK(vkCreatePipelineLayout(
        device,
        &pipeline_layout_info,
        nullptr,
        &built_layout));
}

void ShaderEffect::fill_stages(
    TArray<VkPipelineShaderStageCreateInfo>& pipeline_stages)
{
    for (ShaderEffect::Stage& s : stages) {
        pipeline_stages.add(
            make_pipeline_shader_stage_create_info(s.stage, s.mod->mod));
    }
}

void ShaderCache::init(Allocator& allocator, VkDevice device)
{
    module_cache.init(allocator);
    this->device = device;
}

ShaderModule* ShaderCache::get_shader(Str path)
{
    if (module_cache.contains(path)) {
        return &module_cache[path];
    }

    Raw data = dump_file(path, System_Allocator);

    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = data.size,
        .pCode    = (u32*)data.buffer,
    };

    VkShaderModule result;
    VK_CHECK(vkCreateShaderModule(device, &create_info, 0, &result));

    ShaderModule new_shader;
    new_shader.mod  = result;
    new_shader.code = slice((u8*)data.buffer, data.size);
    module_cache.add(path.clone(System_Allocator), new_shader);

    return &module_cache[path];
}

void ShaderCache::deinit() { module_cache.release(); }
