#pragma once
#include "Containers/Map.h"
#include "Core/Handle.h"
#include "RendererTypes.h"

struct TextureSystem {
    void init(Allocator& allocator, struct Renderer* renderer);
    void deinit();

    THandle<Texture> create_texture(Str path);

private:
    struct Renderer*            owner;
    THandleSystem<Str, Texture> textures;

    static const u32 Handle_Seed = 0x26125192;
};