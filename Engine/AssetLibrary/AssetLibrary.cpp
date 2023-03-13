#include "AssetLibrary.h"

#include "Serialization/JSON.h"

void Asset::write(Tape* output) {}

void AssetInfo::write(Tape* output) { json_serialize_pretty(output, this); }
