#pragma once
#include "Base.h"
#include "Reflection.h"
#include "Tape.h"

namespace AssetKind {
    enum Type : u32
    {
        Unrecognized = 0,
        Texture,
        Mesh,
    };
}
typedef u32 EAssetKind;

namespace TextureFormat {
    enum Type : u32
    {
        Unknown      = 0,
        R8G8B8A8UInt = 1,
    };
}
typedef TextureFormat::Type ETextureFormat;

PROC_FMT_ENUM(TextureFormat, {
    FMT_ENUM_CASE(TextureFormat, Unknown);
    FMT_ENUM_CASE(TextureFormat, R8G8B8A8UInt);
    FMT_ENUM_DEFAULT_CASE(Unknown);
})

PROC_PARSE_ENUM(TextureFormat, {
    PARSE_ENUM_CASE(TextureFormat, Unknown);
    PARSE_ENUM_CASE(TextureFormat, R8G8B8A8UInt);
})

struct TextureAsset {
    u32            width;
    u32            height;
    u32            depth;
    ETextureFormat format;
};

struct AssetInfo {
    int        version;
    EAssetKind kind;

    union {
        TextureAsset texture;
    };

    void write(Tape* output);
};

#pragma pack(push, 1)
struct AssetHeader {
    /** Length of asset info data */
    u32 info_len;
};
#pragma pack(pop)

struct Asset {
    AssetInfo info;
    Slice<u8> blob;

    bool write(IAllocator& allocator, Tape* output);
};

struct TextureAssetDescriptor : IDescriptor {
    PrimitiveDescriptor<u32> width_desc = {
        OFFSET_OF(TextureAsset, width), LIT("width")};
    PrimitiveDescriptor<u32> height_desc = {
        OFFSET_OF(TextureAsset, height), LIT("height")};
    PrimitiveDescriptor<u32> depth_desc = {
        OFFSET_OF(TextureAsset, depth), LIT("depth")};
    PrimitiveDescriptor<u32> format_desc = {
        OFFSET_OF(TextureAsset, format), LIT("format")};

    IDescriptor* descs[4] = {
        &width_desc,
        &height_desc,
        &depth_desc,
        &format_desc,
    };

    CUSTOM_DESC_DEFAULT(TextureAssetDescriptor)
    virtual Str type_name() override { return LIT("TextureAsset"); }
    virtual Slice<IDescriptor*> subdescriptors(umm self) override
    {
        return Slice<IDescriptor*>(descs, ARRAY_COUNT(descs));
    }
};

struct AssetInfoDescriptor : IDescriptor {
    PrimitiveDescriptor<int> version_desc = {
        OFFSET_OF(AssetInfo, version), LIT("version")};
    PrimitiveDescriptor<EAssetKind> kind_desc = {
        OFFSET_OF(AssetInfo, kind), LIT("kind")};

    TextureAssetDescriptor texture_desc = {
        OFFSET_OF(AssetInfo, texture), LIT("texture")};

    IDescriptor* descs_unrecognized[2] = {
        &version_desc,
        &kind_desc,
    };

    IDescriptor* descs_texture[3] = {
        &version_desc,
        &kind_desc,
        &texture_desc,
    };

    CUSTOM_DESC_DEFAULT(AssetInfoDescriptor)

    virtual Str type_name() override { return LIT("AssetInfo"); }
    virtual Slice<IDescriptor*> subdescriptors(umm self) override
    {
        AssetInfo* info = (AssetInfo*)self;
        switch (info->kind) {
            case AssetKind::Unrecognized:
            case AssetKind::Mesh:
            default:
                return Slice<IDescriptor*>(
                    descs_unrecognized,
                    ARRAY_COUNT(descs_unrecognized));

            case AssetKind::Texture:
                return Slice<IDescriptor*>(
                    descs_texture,
                    ARRAY_COUNT(descs_texture));
        }
    }
};

DEFINE_DESCRIPTOR_OF_INL(TextureAsset)
DEFINE_DESCRIPTOR_OF_INL(AssetInfo)
