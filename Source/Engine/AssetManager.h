#pragma once
#include "AssetLibrary/AssetLibrary.h"
#include "Containers/Map.h"

struct AssetManager {
    void   init(IAllocator& allocator);
    Asset* get_asset(Str path);
    void   deinit();

    void flush_assets();

    /** @todo: Set to false when using PAK files */
    bool is_on_plain_fs = true;

    TMap<Str, Asset> loaded_assets;
    IAllocator&      allocator = System_Allocator;
};