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
                .format = 5,
            },
    };

    {
        auto ft = open_write_tape("./asset_info.test.json");
        info.write(&ft);
    }

    return MPASSED();
}