#include "AssetSystem.h"

#include "FileSystem/DirectoryIterator.h"

void AssetSystem::init(Allocator& allocator)
{
    sfl_uuid_init(&uuid_context);
    registry.init(allocator);
    asset_states.init(allocator);

    refresh_registry();
}

void AssetSystem::refresh_registry()
{
    CREATE_SCOPED_ARENA(System_Allocator, temp, MEGABYTES(2));

    Str current_directory = get_cwd(temp);

    Str asset_directory =
        format(temp, LIT("{}/Assets"), FmtPath(current_directory));

    print(LIT("[Asset System]: On directory: {}\n"), asset_directory);

    DirectoryIterator iterator = open_dir(asset_directory);

    FileData it_data;
    while (iterator.next_file(&it_data)) {
        if (it_data.filename == LIT(".")) continue;
        if (it_data.filename == LIT("..")) continue;
        if (it_data.attributes == FileAttributes::File) {
            print(
                LIT("[Asset System]: Discovered asset: {}\n"),
                it_data.filename);
        }
    }
}

void AssetSystem::deinit() {}