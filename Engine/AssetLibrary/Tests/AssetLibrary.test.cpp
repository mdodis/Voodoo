#include "AssetLibrary.h"

#include "FileSystem/FileSystem.h"
#include "Test/Test.h"

TEST_CASE("AssetLibrary/Main", "Write AssetInfo")
{
    AssetInfo info = {
        .version = 100,
        .kind    = AssetKind::Texture,
        .texture =
            {
                .width  = 100,
                .height = 200,
                .depth  = 32,
                .format = TextureFormat::Unknown,
            },
    };

    FileHandle              fh = open_file_write("./asset_info.test.json");
    TBufferedFileTape<true> ft(fh);

    info.write(&ft);

    return MPASSED();
}