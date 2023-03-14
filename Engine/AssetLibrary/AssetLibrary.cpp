#include "AssetLibrary.h"

#include "Memory/AllocTape.h"
#include "Serialization/JSON.h"

bool Asset::write(IAllocator& allocator, Tape* output)
{
    AssetHeader header;
    AllocTape   info_tape(allocator);

    json_serialize_pretty(&info_tape, &info);

    header.info_len = (u32)info_tape.wr_offset;

    if (!output->write(info_tape.ptr, header.info_len)) return false;
    if (!output->write(blob.ptr, blob.size())) return false;
    return true;
}

void AssetInfo::write(Tape* output) { json_serialize_pretty(output, this); }
