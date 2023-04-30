#include "AssetManager.h"

void AssetManager::init(IAllocator& allocator)
{
    this->allocator = allocator;
    loaded_assets.init(allocator);
}

Asset* AssetManager::get_asset(Str path)
{
    if (loaded_assets.contains(path)) {
        return &loaded_assets[path];
    }

    auto asset_load_result = Asset::load(allocator, path);
    if (!asset_load_result.ok()) {
        return nullptr;
    }

    Asset asset = asset_load_result.value();
    loaded_assets.add(path, asset);

    return &loaded_assets[path];
}

void AssetManager::flush_assets()
{
    for (auto pair : loaded_assets) {
        allocator.release((umm)pair.val.blob.ptr);
    }

    loaded_assets.empty();
}

void AssetManager::deinit()
{
    flush_assets();
    loaded_assets.release();
}
