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

struct AssetTexture {
    u32 width;
    u32 height;
    u32 depth;
    u32 format;
};

struct AssetInfo {
    int        version;
    EAssetKind kind;

    union {
        AssetTexture texture;
    };

    void write(Tape* output);
};

struct Asset {
    AssetInfo info;
    Slice<u8> blob;

    void write(Tape* output);
};

struct AssetTextureDescriptor : IDescriptor {
    PrimitiveDescriptor<u32> width_desc = {
        OFFSET_OF(AssetTexture, width), LIT("width")};
    PrimitiveDescriptor<u32> height_desc = {
        OFFSET_OF(AssetTexture, height), LIT("height")};
    PrimitiveDescriptor<u32> depth_desc = {
        OFFSET_OF(AssetTexture, depth), LIT("depth")};
    PrimitiveDescriptor<u32> format_desc = {
        OFFSET_OF(AssetTexture, format), LIT("format")};

    IDescriptor* descs[4] = {
        &width_desc,
        &height_desc,
        &depth_desc,
        &format_desc,
    };

    CUSTOM_DESC_DEFAULT(AssetTextureDescriptor)
    virtual Str type_name() override { return LIT("AssetTexture"); }
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

    AssetTextureDescriptor texture_desc = {
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

DEFINE_DESCRIPTOR_OF_INL(AssetTexture)
DEFINE_DESCRIPTOR_OF_INL(AssetInfo)
