#pragma once
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include "Containers/Array.h"
#include "Containers/Slice.h"
#include "Core/Utility.h"
#include "EngineConfig.h"
#include "Result.h"
#include "Types.h"
#include "vk_mem_alloc.h"

namespace AllocationKind {
    enum Type : u32
    {
        Buffer,
        Image,
    };
}
typedef AllocationKind::Type EAllocationKind;

/**
 * Thin abstraction over vk_mem_alloc.
 *
 * Can track allocations and display them
 *
 * @todo: Only take in file/line if VMA_TRACK_ALLOCATIONS is enabled
 */
struct VMA {
    struct Buffer {
        VkBuffer      buffer     = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    struct Image {
        VkImage       image      = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
    };

    struct Metadata {
        const char*     file;
        size_t          line;
        bool            valid;
        EAllocationKind kind;
        union {
            Buffer buffer;
            Image  image;
        };

        _inline VmaAllocation allocation() const
        {
            switch (kind) {
                case AllocationKind::Buffer:
                    return buffer.allocation;

                case AllocationKind::Image:
                    return image.allocation;

                default:
                    return VK_NULL_HANDLE;
            }
        }
    };

    void init(
        VkPhysicalDevice physical_device,
        VkDevice         logical_device,
        VkInstance       instance);
    void deinit();

    Result<Buffer, VkResult> create_buffer(
        size_t             alloc_size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage     memory_usage,
        const char*        file = 0,
        size_t             line = 0);

    void destroy_buffer(const Buffer& buffer);

    Result<Image, VkResult> create_image(
        const VkImageCreateInfo& info,
        VmaMemoryUsage           usage,
        VkMemoryPropertyFlags    required_flags,
        const char*              file = 0,
        size_t                   line = 0);

    void destroy_image(const Image& image);

    void* map(const Buffer& buffer);
    void  unmap(const Buffer& buffer);

private:
    VmaAllocator gpu_allocator;

#if VMA_TRACK_ALLOCATIONS
    TArray<Metadata*> allocations;

    void track_allocation(const Buffer& buffer, const char* file, size_t line);

    void track_allocation(const Image& image, const char* file, size_t line);

    void _track_allocation(const Metadata& metadata);

    void check_and_free_allocation(VmaAllocation allocation);
    void check_and_free_allocations();
#endif
};

#define VMA_CREATE_BUFFER(vma, alloc_size, usage, memory_usage) \
    vma.create_buffer(alloc_size, usage, memory_usage, __FILE__, __LINE__)

#define VMA_DESTROY_BUFFER(vma, buffer) vma.destroy_buffer(buffer)

#define VMA_CREATE_IMAGE_(vma, info, usage, required_flags, file, line) \
    vma.create_image(info, usage, required_flags, file, line)

#define VMA_CREATE_IMAGE(vma, info, usage) \
    VMA_CREATE_IMAGE_(vma, info, usage, 0, __FILE__, __LINE__)

#define VMA_CREATE_IMAGE2(vma, info, usage, required_flags) \
    VMA_CREATE_IMAGE_(vma, info, usage, required_flags, __FILE__, __LINE__)

#define VMA_DESTROY_IMAGE(vma, image) vma.destroy_image(image)

#define VMA_MAP(vma, buffer) vma.map(buffer)
#define VMA_UNMAP(vma, buffer) vma.unmap(buffer)

using AllocatedBuffer = VMA::Buffer;
using AllocatedImage  = VMA::Image;