#pragma once
#include "AssetLibrary/AssetLibrary.h"
#include "Containers/Map.h"
#include "Core/Handle.h"
#include "sfl_uuid.h"

/**
 * Represents a static reference to an asset.
 *
 * As a string, an asset named cube.asset located in the Engine module asset
 * directory, would be:
 *
 * "@Engine /Primitives/Cube.asset"
 */
struct AssetReference {
    Str ns;
    Str path;
};

/**
 * An asset id holds the unique identifier of the asset, as well as the state
 * that it's in currently, loaded or not.
 */
struct AssetID {
    SflUUID id;
    bool    loaded;
};

static _inline u64 hash_of(const AssetID& id, u32 seed)
{
    const u8* p = (const u8*)&id.id;
    return murmur_hash2(p, sizeof(SflUUID), seed);
}

static _inline bool operator==(const AssetID& left, const AssetID& right)
{
    return (left.id.qwords[0] == right.id.qwords[0]) &&
           (left.id.qwords[1] == right.id.qwords[1]);
}

struct AssetSystem {
    void init(Allocator& allocator);
    void deinit();

    void refresh_registry();

    AssetID resolve_reference(const AssetReference& reference) const;

    TMap<AssetReference, AssetID> registry;
    TMap<AssetID, Asset>          asset_states;
    SflUUIDContext                uuid_context;
};