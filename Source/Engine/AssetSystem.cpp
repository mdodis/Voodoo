#include "AssetSystem.h"

#include "FileSystem/DirectoryIterator.h"

void AssetSystem::init(Allocator& allocator)
{
    sfl_uuid_init(&uuid_context);
    registry.ids_to_references.init(allocator);
    registry.references_to_ids.init(allocator);
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
            Str asset_ns = LIT("Engine").clone(System_Allocator);
            Str asset_path =
                format(System_Allocator, LIT("/{}"), it_data.filename);

            AssetReference reference = {
                .module = asset_ns,
                .path   = asset_path,
            };

            AssetID asset_id = AssetID::create(uuid_context);

            registry.ids_to_references.add(asset_id, reference);
            registry.references_to_ids.add(reference, asset_id);

            print(
                LIT("[Asset System]: Discovered asset: @{} {} ({})\n"),
                reference.module,
                reference.path,
                asset_id);
        }
    }
}

AssetID AssetSystem::resolve_reference(const AssetReference& reference)
{
    if (registry.references_to_ids.contains(reference)) {
        return registry.references_to_ids[reference];
    }

    return AssetID::invalid();
}

Asset* AssetSystem::load_asset_now(const AssetID& id)
{
    CREATE_SCOPED_ARENA(System_Allocator, temp, KILOBYTES(1));

    if (!asset_states.contains(id)) {
        const AssetReference& reference = registry.ids_to_references[id];

        Str path = convert_reference_to_path(temp, reference);

        auto asset_load_result = Asset::load(System_Allocator, path);
        if (!asset_load_result.ok()) {
            print(
                LIT("Failed to load asset '{}': {}\n"),
                path,
                asset_load_result.err());
            return nullptr;
        }

        AssetState state = {
            .is_loaded = true,
            .id        = id,
            .value     = asset_load_result.value(),
        };

        asset_states.add(id, state);
    }

    AssetState& state = asset_states[id];
    return &state.value;
}

Str AssetSystem::convert_reference_to_path(
    Allocator& allocator, const AssetReference& reference)
{
    return format(allocator, LIT("Assets{}\0"), reference.path);
}

void AssetSystem::deinit() {}