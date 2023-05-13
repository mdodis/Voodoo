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

    AssetReference clone(Allocator& allocator) const
    {
        u64   total_len = ns.len + path.len;
        char* buf       = (char*)allocator.reserve(total_len);

        memcpy(buf, ns.data, ns.len);
        memcpy(buf + ns.len, path.data, path.len);

        return AssetReference{
            .ns   = Str(buf, ns.len),
            .path = Str(buf + ns.len, path.len),
        };
    }
};

static _inline u64 hash_of(const AssetReference& reference, u32 seed)
{
    u64 h1 = hash_of(reference.ns, seed);
    u64 h2 = hash_of(reference.path, seed);

    return h1 ^ h2;
}

static _inline bool operator==(
    const AssetReference& left, const AssetReference& right)
{
    if (left.ns != right.ns) return false;

    if (left.path.len != right.path.len) return false;

    for (u64 i = 0; i < left.path.len; ++i) {
        switch (left.path[i]) {
            case '/':
            case '\\':
                if ((right.path[i] != '/') && (right.path[i] != '\\')) {
                    return false;
                }
                break;
            default:
                if (left.path[i] != right.path[i]) return false;
                break;
        }
    }

    return true;
}

/**
 * An asset id holds the unique identifier of the asset
 */
struct AssetID {
    SflUUID id;

    _inline bool is_valid() const
    {
        return (id.qwords[0] == 0) && (id.qwords[1] == 0);
    }

    static _inline AssetID create(SflUUIDContext& context)
    {
        AssetID id;
        sfl_uuid_gen_v4(&context, &id.id);
        return id;
    }

    static _inline constexpr AssetID invalid()
    {
        SflUUID id;
        id.qwords[0] = 0;
        id.qwords[1] = 0;
        return AssetID{id};
    }
};

PROC_FMT_INL(AssetID)
{
    char buf[SFL_UUID_BUFFER_SIZE];
    sfl_uuid_to_string(&type.id, buf);

    Str s(buf, SFL_UUID_BUFFER_SIZE - 1);
    tape->write_str(s);
}

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

struct AssetState {
    bool    is_loaded;
    AssetID id;
    Asset   value;
};

static _inline bool operator==(const AssetState& left, const AssetState& right)
{
    return left.id == right.id;
}

struct AssetSystem {
    void init(Allocator& allocator);
    void deinit();

    void refresh_registry();

    AssetID resolve_reference(const AssetReference& reference);

    Asset* load_asset_now(const AssetID& id);

private:
    Str convert_reference_to_path(
        Allocator& allocator, const AssetReference& reference);

    struct {
        TMap<AssetID, AssetReference> ids_to_references;
        TMap<AssetReference, AssetID> references_to_ids;
    } registry;

    TMap<AssetID, AssetState> asset_states;
    SflUUIDContext            uuid_context;
};