#include "AssetLibrary.h"

#include "Containers/Array.h"
#include "FileSystem/FileSystem.h"
#include "Test/Test.h"

TEST_CASE("AssetLibrary/Main", "Write/Read asset basic")
{
    auto blob = arr<u8>(
        (u8)0x49,
        (u8)0x20,
        (u8)0x41,
        (u8)0x4d,
        (u8)0x20,
        (u8)0x41,
        (u8)0x4e,
        (u8)0x20,
        (u8)0x41,
        (u8)0x53,
        (u8)0x53,
        (u8)0x45,
        (u8)0x54);

    AssetInfo info = {
        .version     = 1,
        .kind        = AssetKind::Texture,
        .compression = AssetCompression::None,
        .actual_size = blob.size(),
        .texture =
            {
                .width  = 1,
                .height = 2,
                .depth  = 3,
                .format = TextureFormat::Unknown,
            },
    };

    Asset asset = {
        .info = info,
        .blob = slice(blob),
    };

    {
        FileHandle              fh = open_file_write("./asset.test.asset");
        TBufferedFileTape<true> ft(fh);

        REQUIRE(asset.write(System_Allocator, &ft), "Write asset");
    }

    {
        CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(1));

        auto ft = open_read_tape(LIT("./asset.test.asset"));

        Asset asset = asset.load(temp, &ft).unwrap();

        REQUIRE(
            memcmp(blob.elements, asset.blob.ptr, asset.blob.size()) == 0,
            "");
    }

    return MPASSED();
}