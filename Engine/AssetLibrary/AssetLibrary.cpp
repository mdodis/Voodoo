#include "AssetLibrary.h"

#include "Memory/AllocTape.h"
#include "Serialization/JSON.h"

void Asset::write(IAllocator& allocator, Tape* output)
{
    AssetHeader header;
    AllocTape   info_tape(allocator);

    json_serialize_pretty(&info_tape, &info);

    header.info_len = info_tape.wr_offset;
}

void AssetInfo::write(Tape* output) { json_serialize_pretty(output, this); }
