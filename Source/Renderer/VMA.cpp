#include "VMA.h"

#include "VulkanCommon/VulkanCommon.h"

void VMA::init(
    VkPhysicalDevice physical_device,
    VkDevice         logical_device,
    VkInstance       instance)
{
    VmaAllocatorCreateInfo create_info = {
        .physicalDevice = physical_device,
        .device         = logical_device,
        .instance       = instance,
    };

    VK_CHECK(vmaCreateAllocator(&create_info, &gpu_allocator));

#if VMA_TRACK_ALLOCATIONS
    allocations.alloc = &System_Allocator;
#endif
}

void VMA::deinit()
{
#if VMA_TRACK_ALLOCATIONS
    check_and_free_allocations();
    allocations.release();
#endif
    vmaDestroyAllocator(gpu_allocator);
}

Result<AllocatedBufferBase, VkResult> VMA::create_buffer(
    size_t             alloc_size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage     memory_usage,
    const char*        file,
    size_t             line)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = alloc_size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage     = memory_usage,
        .pUserData = nullptr,
    };

    AllocatedBufferBase result;
    VK_RETURN_IF_ERR(vmaCreateBuffer(
        gpu_allocator,
        &buffer_info,
        &alloc_info,
        &result.buffer,
        &result.allocation,
        nullptr));

#if VMA_TRACK_ALLOCATIONS
    track_allocation(result, file, line);
#endif

    return Ok(result);
}

void VMA::destroy_buffer(const AllocatedBufferBase& buffer)
{
#if VMA_TRACK_ALLOCATIONS
    check_and_free_allocation(buffer.allocation);
#endif
    vmaDestroyBuffer(gpu_allocator, buffer.buffer, buffer.allocation);
}

Result<VMA::Image, VkResult> VMA::create_image(
    const VkImageCreateInfo& info,
    VmaMemoryUsage           usage,
    VkMemoryPropertyFlags    required_flags,
    const char*              file,
    size_t                   line)
{
    VmaAllocationCreateInfo image_allocation_info = {
        .usage         = usage,
        .requiredFlags = required_flags,
    };

    VMA::Image result;
    VK_RETURN_IF_ERR(vmaCreateImage(
        gpu_allocator,
        &info,
        &image_allocation_info,
        &result.image,
        &result.allocation,
        0));

#if VMA_TRACK_ALLOCATIONS
    track_allocation(result, file, line);
#endif

    return Ok(result);
}

void VMA::destroy_image(const Image& image)
{
#if VMA_TRACK_ALLOCATIONS
    check_and_free_allocation(image.allocation);
#endif
    vmaDestroyImage(gpu_allocator, image.image, image.allocation);
}

void* VMA::map(const AllocatedBufferBase& buffer)
{
    void* result;
    VK_CHECK(vmaMapMemory(gpu_allocator, buffer.allocation, &result));
    return result;
}

void VMA::unmap(const AllocatedBufferBase& buffer)
{
    vmaUnmapMemory(gpu_allocator, buffer.allocation);
}

#if VMA_TRACK_ALLOCATIONS

void VMA::track_allocation(
    const AllocatedBufferBase& buffer, const char* file, size_t line)
{
    VMA::Metadata metadata = {
        .file   = file,
        .line   = line,
        .kind   = AllocationKind::Buffer,
        .buffer = buffer,
    };

    _track_allocation(metadata);
}

void VMA::track_allocation(const Image& image, const char* file, size_t line)
{
    VMA::Metadata metadata = {
        .file  = file,
        .line  = line,
        .kind  = AllocationKind::Image,
        .image = image,
    };

    _track_allocation(metadata);
}

void VMA::_track_allocation(const Metadata& metadata)
{
    VMA::Metadata* pmetadata =
        (VMA::Metadata*)System_Allocator.reserve(sizeof(VMA::Metadata));
    *pmetadata       = metadata;
    pmetadata->valid = true;

    allocations.add(pmetadata);

    vmaSetAllocationUserData(
        gpu_allocator,
        metadata.allocation(),
        (void*)pmetadata);
}

void VMA::check_and_free_allocation(VmaAllocation allocation)
{
    VmaAllocationInfo info;
    vmaGetAllocationInfo(gpu_allocator, allocation, &info);

    ASSERT(info.pUserData);

    VMA::Metadata* metadata = (VMA::Metadata*)info.pUserData;

    if (!metadata->valid) {
        print(
            LIT("[VMA]: Allocation at {}:{} was already freed\n"),
            metadata->file,
            metadata->line);
    }

    metadata->valid = false;
}

void VMA::check_and_free_allocations()
{
    for (VMA::Metadata* metadata : allocations) {
        if (!metadata->valid) continue;
        print(
            LIT("[VMA]: Allocation at {}:{} was not freed\n"),
            metadata->file,
            metadata->line);

        switch (metadata->kind) {
            case AllocationKind::Buffer: {
                vmaDestroyBuffer(
                    gpu_allocator,
                    metadata->buffer.buffer,
                    metadata->buffer.allocation);
            } break;

            case AllocationKind::Image: {
                vmaDestroyImage(
                    gpu_allocator,
                    metadata->image.image,
                    metadata->image.allocation);
            } break;

            default: {
                ASSERT(false);
            } break;
        }

        System_Allocator.release((umm)metadata);
    }
}

#endif