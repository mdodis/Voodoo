#include "Shader.h"

#include <string>
#include <sstream>

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
