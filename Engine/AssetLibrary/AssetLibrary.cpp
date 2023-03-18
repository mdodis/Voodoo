#include "AssetLibrary.h"

#include "Importers/DefaultImporters.h"
#include "Memory/AllocTape.h"
#include "Serialization/JSON.h"
#include "lz4.h"

bool Asset::write(IAllocator& allocator, Tape* output)
{
    AssetHeader header;
    AllocTape   info_tape(allocator);

    Slice<u8> compressed_blob = blob;

    bool did_compress = false;

    // Compress if needed
    if (!info.is_compressed()) {
        ASSERT(blob.size() < NumProps<int>::max);

        int max_size = LZ4_compressBound((int)blob.size());
        umm ptr      = allocator.reserve(max_size);

        int compressed_size = LZ4_compress_default(
            (const char*)blob.ptr,
            (char*)ptr,
            (int)blob.size(),
            (int)max_size);

        if (compressed_size == 0) return false;
        compressed_blob = slice(ptr, compressed_size);

        info.compression = AssetCompression::LZ4;
        did_compress     = true;
    }

    DEFER(if (did_compress) allocator.release(compressed_blob.ptr));

    json_serialize_pretty(&info_tape, &info);
    DEFER(info_tape.release());

    header.info_len  = (u32)info_tape.wr_offset;
    header.blob_size = did_compress ? compressed_blob.size() : blob.size();

    if (!output->write(&header, sizeof(header))) return false;
    if (!output->write(info_tape.ptr, header.info_len)) return false;

    if (!output->write(compressed_blob.ptr, compressed_blob.size()))
        return false;

    return true;
}

Result<AssetInfo, EAssetLoadError> Asset::probe(
    IAllocator& allocator, Tape* input)
{
    AssetHeader header;
    AssetInfo   asset_info;

    if (!input->read_struct(&header)) return Err(AssetLoadError::InvalidFormat);

    if (!deserialize(input, allocator, asset_info, json_deserialize))
        return Err(AssetLoadError::InvalidFormat);

    asset_info.blob_size   = header.blob_size;
    asset_info.blob_offset = sizeof(AssetHeader) + header.info_len;
    return Ok(asset_info);
}

Result<Asset, EAssetLoadError> Asset::load(IAllocator& allocator, Tape* input)
{
    AssetInfo asset_info;

    // Get metadata
    {
        ParseTape pt(input);
        DEFER(pt.restore());

        auto probe_result = Asset::probe(allocator, &pt);
        if (!probe_result.ok()) return Err(probe_result.err());

        asset_info = probe_result.value();
    }

    input->move(asset_info.blob_offset);

    Raw compressed_blob = alloc_raw(allocator, asset_info.blob_size);
    if (!compressed_blob) return Err(AssetLoadError::OutOfMemory);

    // Read in the blob
    if (!input->read_raw(compressed_blob))
        return Err(AssetLoadError::InvalidFormat);

    Raw blob = compressed_blob;

    // If it's compressed, decompress it
    if (asset_info.is_compressed()) {
        DEFER(allocator.release(compressed_blob.buffer));

        Raw uncompressed_blob = alloc_raw(allocator, asset_info.actual_size);
        if (!uncompressed_blob) return Err(AssetLoadError::OutOfMemory);

        // @note: For now we only have LZ4 compression
        int result = LZ4_decompress_safe(
            (const char*)compressed_blob.buffer,
            (char*)uncompressed_blob.buffer,
            compressed_blob.size,
            uncompressed_blob.size);

        if (result <= 0) {
            return Err(AssetLoadError::CompressionFailed);
        }

        blob = uncompressed_blob;
    }

    return Ok(Asset{
        .info = asset_info,
        .blob = slice(blob),
    });
}

void ImporterRegistry::register_importer(const Importer& importer)
{
    for (const Importer& imp : importers) {
        ASSERT(
            (imp.name != importer.name) &&
            "There cannot be an importer of the same name");
    }

    importers.add(importer);
}

Result<Asset, Str> ImporterRegistry::import_asset_from_file(
    Str path, IAllocator& allocator)
{
    Str       extension       = path.chop_left_last_of('.');
    Importer* chosen_importer = 0;

    for (Importer& importer : importers) {
        for (Str supported_extension : importer.file_extensions) {
            if (extension == supported_extension) {
                chosen_importer = &importer;
                break;
            }
        }
    }

    if (!chosen_importer) {
        return Err(LIT("No importer found for the specified file"));
    }

    return chosen_importer->import(path, allocator);
}

void ImporterRegistry::init_default_importers()
{
    importers.add(Stb_Image_Importer);
}
