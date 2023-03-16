#include "AssetLibrary.h"

#include "Importers/DefaultImporters.h"
#include "Memory/AllocTape.h"
#include "Serialization/JSON.h"

bool Asset::write(IAllocator& allocator, Tape* output)
{
    AssetHeader header;
    AllocTape   info_tape(allocator);

    json_serialize_pretty(&info_tape, &info);

    header.info_len  = (u32)info_tape.wr_offset;
    header.blob_size = blob.size();

    if (!output->write(&header, sizeof(header))) return false;
    if (!output->write(info_tape.ptr, header.info_len)) return false;
    if (!output->write(blob.ptr, blob.size())) return false;
    return true;
}

Result<Asset, EAssetLoadError> Asset::load(IAllocator& allocator, Tape* input)
{
    AssetHeader header;
    AssetInfo   asset_info;
    ParseTape   pt(input);

    if (!pt.read_struct(&header)) return Err(AssetLoadError::InvalidFormat);

    Raw blob = alloc_raw(allocator, header.blob_size);
    if (!blob) return Err(AssetLoadError::OutOfMemory);

    if (!deserialize(&pt, allocator, asset_info, json_deserialize))
        return Err(AssetLoadError::InvalidFormat);
    pt.restore();

    input->move(header.info_len);
    if (!input->read_raw(blob)) return Err(AssetLoadError::InvalidFormat);

    return Ok(Asset{});
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
