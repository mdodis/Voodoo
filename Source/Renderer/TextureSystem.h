#pragma once
#include "Containers/Map.h"
#include "Core/Handle.h"
#include "RendererTypes.h"

struct TextureSystem {
    void init(Allocator& allocator, struct Renderer* renderer);
    void deinit();

    THandle<Texture> create_texture(Str path);

    THandle<Texture> get_handle(Str path);
    Texture*         resolve_handle(THandle<Texture> handle);
    void             release_handle(THandle<Texture> handle);

    struct {
        VkSampler pixel;
    } samplers;

private:
    struct Renderer*            owner;
    THandleSystem<Str, Texture> textures;

    AllocatedImage load_texture_from_file(Str path);

    static const u32 Handle_Seed = 0x26125192;
};