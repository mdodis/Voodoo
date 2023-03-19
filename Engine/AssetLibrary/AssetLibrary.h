#pragma once
#include "Base.h"
#include "Reflection.h"
#include "Result.h"
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

namespace AssetLoadError {
    enum Type : u32
    {
        Unknown = 0,
        InvalidFormat,
        OutOfMemory,
        CompressionFailed,
    };
}
typedef AssetLoadError::Type EAssetLoadError;

PROC_FMT_ENUM(AssetLoadError, {
    FMT_ENUM_CASE(AssetLoadError, Unknown);
    FMT_ENUM_CASE(AssetLoadError, InvalidFormat);
    FMT_ENUM_CASE(AssetLoadError, OutOfMemory);
    FMT_ENUM_CASE(AssetLoadError, CompressionFailed);
})

namespace AssetCompression {
    enum Type : u32
    {
        None = 0,
        LZ4
    };
}
typedef AssetCompression::Type EAssetCompression;

PROC_FMT_ENUM(AssetCompression, {
    FMT_ENUM_CASE(AssetCompression, None);
    FMT_ENUM_CASE(AssetCompression, LZ4);
    FMT_ENUM_DEFAULT_CASE(None);
})

PROC_PARSE_ENUM(AssetCompression, {
    PARSE_ENUM_CASE(AssetCompression, None);
    PARSE_ENUM_CASE(AssetCompression, LZ4);
})

struct TextureAsset {
    u32            width;
    u32            height;
    u32            depth;
    ETextureFormat format;
};

struct AssetInfo {
    /** Version ID */
    int               version;
    /** The type of the asset */
    EAssetKind        kind;
    /** What kind of compression (if any) is used */
    EAssetCompression compression;
    /** The actual size of the blob (if compressed) */
    u64               actual_size;

    union {
        TextureAsset texture;
    };

    _inline bool is_compressed() const
    {
        return compression != AssetCompression::None;
    }

    // Not Serialized

    /** Blob size in the file. Used when the blob hasn't been loaded */
    u64 blob_size   = 0;
    /** Blob offset from file start. Used when the blob hasn't been loaded */
    i64 blob_offset = 0;
};

#pragma pack(push, 1)
struct AssetHeader {
    /** Length of asset info data */
    u32 info_len;
    u64 blob_size;
};
#pragma pack(pop)

struct Asset {
    AssetInfo info;
    Slice<u8> blob;

    bool write(IAllocator& allocator, Tape* output);
    static Result<Asset, EAssetLoadError> load(
        IAllocator& allocator, Tape* input);

    static Result<AssetInfo, EAssetLoadError> probe(
        IAllocator& allocator, Tape* input);

    static Result<void, EAssetLoadError> unpack(
        IAllocator&      allocator,
        const AssetInfo& info,
        Tape*            input,
        Slice<u8>&       buffer);
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
    EnumDescriptor<EAssetCompression> compression_desc = {
        OFFSET_OF(AssetInfo, compression), LIT("compression")};
    PrimitiveDescriptor<u64> actual_size_desc = {
        OFFSET_OF(AssetInfo, actual_size), LIT("actual_size")};

    TextureAssetDescriptor texture_desc = {
        OFFSET_OF(AssetInfo, texture), LIT("texture")};

    IDescriptor* descs_unrecognized[4] = {
        &version_desc,
        &kind_desc,
        &compression_desc,
        &actual_size_desc,
    };

    IDescriptor* descs_texture[5] = {
        &version_desc,
        &kind_desc,
        &compression_desc,
        &actual_size_desc,
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

#define PROC_IMPORTER_IMPORT(name) \
    Result<Asset, Str> name(Str path, IAllocator& allocator)
typedef PROC_IMPORTER_IMPORT(ProcImporterImport);

struct Importer {
    Str                 name;
    EAssetKind          kind;
    Slice<Str>          file_extensions;
    ProcImporterImport* import;
};

struct ImporterRegistry {
    TArray<Importer> importers;
    ImporterRegistry(IAllocator& allocator) : importers(&allocator) {}

    void               init_default_importers();
    void               register_importer(const Importer& importer);
    Result<Asset, Str> import_asset_from_file(Str path, IAllocator& allocator);
};