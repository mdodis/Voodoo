#include "TextureSystem.h"

#include "AssetLibrary/AssetLibrary.h"
#include "FileSystem/FileSystem.h"
#include "Renderer.h"
#include "tracy/Tracy.hpp"

void TextureSystem::init(Allocator& allocator, Renderer* renderer)
{
    owner = renderer;
    textures.init(allocator);

    // Samplers
    {
        VkSamplerCreateInfo sampler_create_info =
            make_sampler_create_info(VK_FILTER_NEAREST);

        VK_CHECK(vkCreateSampler(
            owner->device,
            &sampler_create_info,
            0,
            &samplers.pixel));
    }
}

AllocatedImage TextureSystem::load_texture_from_file(Str path)
{
    ZoneScoped;

    CREATE_SCOPED_ARENA(System_Allocator, temp, MEGABYTES(5));
    BufferedReadTape<true> read_tape(open_file_read(path));

    AssetInfo info;
    {
        ZoneScopedN("Probe texture asset");
        info = Asset::probe(temp, &read_tape).unwrap();
        ASSERT(info.kind == AssetKind::Texture);
        ASSERT(info.texture.format == TextureFormat::R8G8B8A8UInt);
    }

    VkFormat     image_format = VK_FORMAT_R8G8B8A8_SRGB;
    VkDeviceSize image_size   = info.actual_size;

    AllocatedBuffer staging_buffer =
        VMA_CREATE_BUFFER(
            owner->vma,
            image_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY)
            .unwrap();

    {
        ZoneScopedN("Unpack texture");
        void*     data = VMA_MAP(owner->vma, staging_buffer);
        Slice<u8> buffer_ptr((u8*)data, image_size);
        Asset::unpack(temp, info, &read_tape, buffer_ptr).unwrap();
        VMA_UNMAP(owner->vma, staging_buffer);
    }

    VkExtent3D image_extent = {
        .width  = (u32)info.texture.width,
        .height = (u32)info.texture.height,
        .depth  = 1,
    };

    VkImageCreateInfo image_create_info = {
        .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext       = 0,
        .imageType   = VK_IMAGE_TYPE_2D,
        .format      = image_format,
        .extent      = image_extent,
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = VK_SAMPLE_COUNT_1_BIT,
        .tiling      = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };

    VmaAllocationCreateInfo image_allocation_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };

    AllocatedImage result =
        VMA_CREATE_IMAGE(
            owner->vma,
            image_create_info,
            VMA_MEMORY_USAGE_GPU_ONLY)
            .unwrap();
    owner->immediate_submit_lambda([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        VkImageMemoryBarrier transfer_barrier = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext            = 0,
            .srcAccessMask    = 0,
            .dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image            = result.image,
            .subresourceRange = range,
        };

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &transfer_barrier);

        VkBufferImageCopy copy_region = {
            .bufferOffset      = 0,
            .bufferRowLength   = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel       = 0,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
            .imageExtent = image_extent,
        };

        vkCmdCopyBufferToImage(
            cmd,
            staging_buffer.buffer,
            result.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copy_region);

        VkImageMemoryBarrier layout_change_barrier = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext            = 0,
            .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image            = result.image,
            .subresourceRange = range,
        };

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &layout_change_barrier);
    });

    VMA_DESTROY_BUFFER(owner->vma, staging_buffer);

    return result;
}

THandle<Texture> TextureSystem::create_texture(Str path)
{
    AllocatedImage image = load_texture_from_file(path);

    VkImageView           view;
    VkImageViewCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = 0,
        .flags    = 0,
        .image    = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };
    VK_CHECK(vkCreateImageView(owner->device, &create_info, 0, &view));

    Texture texture = {
        .image = image,
        .view  = view,
    };

    u32 id = textures.create_resource(path, texture);
    return THandle<Texture>{.id = id};
}

THandle<Texture> TextureSystem::get_handle(Str path)
{
    return textures.get_handle(path);
}

void TextureSystem::release_handle(THandle<Texture> handle)
{
    textures.release(handle);
}

Texture* TextureSystem::resolve_handle(THandle<Texture> handle)
{
    return &textures.resolve(handle);
}

void TextureSystem::deinit()
{
    vkDestroySampler(owner->device, samplers.pixel, nullptr);

    textures.deinit();
}