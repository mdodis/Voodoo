#include "WorldSerializer.h"

#include "Core/Archive.h"
#include "Memory/AllocTape.h"

void WorldSerializer::init(IAllocator& allocator)
{
    id_to_desc.init(allocator);
    type_name_to_id.init(allocator);
}

void WorldSerializer::register_descriptor(u64 id, IDescriptor* descriptor)
{
    if (descriptor == nullptr) return;

    id_to_desc.add(id, descriptor);
    type_name_to_id.add(descriptor->type_name(), id);
}

void WorldSerializer::save(const flecs::world& world, Str path)
{
    // @todo: sth is completely borked here; it just writes zeroes!
    CREATE_SCOPED_ARENA(&System_Allocator, component_allocator, KILOBYTES(10));
    CREATE_SCOPED_ARENA(&System_Allocator, all_allocator, KILOBYTES(10));

    AllocWriteTape output(System_Allocator);
    DEFER(output.release());

    {
        SAVE_ARENA(all_allocator);
        SerializedWorld serialized_world(all_allocator);

        auto world_filter =
            world.filter_builder<EditorSelectableComponent>().build();

        world_filter.each(
            [&](flecs::entity entity, EditorSelectableComponent& component) {
                Str              name(entity.name().c_str());
                SerializedEntity ser_entity(name, all_allocator);

                entity.each([&](flecs::id id) {
                    if (!id.is_entity()) return;

                    flecs::entity comp  = id.entity();
                    auto          rawid = comp.raw_id();

                    umm comp_ptr = (umm)entity.get_mut(comp);

                    if (!id_to_desc.contains(rawid)) return;
                    IDescriptor* desc = id_to_desc[rawid];

                    TArray<u8> serialized_data(&all_allocator);

                    {
                        AllocWriteTape write_tape(component_allocator);
                        DEFER(write_tape.release());

                        ASSERT(archive_serialize(&write_tape, desc, comp_ptr));

                        serialized_data.init_range(write_tape.size);
                        memcpy(
                            serialized_data.data,
                            write_tape.ptr,
                            serialized_data.size);
                    }

                    SerializedComponent ser_comp = {
                        .type_name = desc->type_name(),
                        .data      = serialized_data,
                    };

                    ser_entity.components.add(ser_comp);
                });

                serialized_world.entities.add(ser_entity);
            });

        // Serialize the whole thing
        ASSERT(archive_serialize(
            &output,
            descriptor_of(&serialized_world),
            (umm)&serialized_world));
    }

    // Convert it to an asset
    Asset asset = make_archive_asset(slice(output.ptr, output.size));

    // Write it
    BufferedWriteTape<true> t(open_file_write(path));
    ASSERT(asset.write(all_allocator, &t, false));
}

void WorldSerializer::import(flecs::world& world, Str path)
{
    CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(10));

    Asset asset = Asset::load(temp, path).unwrap();

    RawReadTape input(Raw{asset.blob.ptr, asset.blob.count});

    SerializedWorld ser_world;
    ASSERT(archive_deserialize(
        &input,
        temp,
        descriptor_of(&ser_world),
        (umm)&ser_world));

    for (const auto& ser_entity : ser_world.entities) {
        flecs::entity entity;
        {
            SAVE_ARENA(temp);
            const char* name = format_cstr(temp, LIT("{}"), ser_entity.name);
            entity           = world.entity(name);
        }

        for (const auto& ser_comp : ser_entity.components) {
            SAVE_ARENA(temp);

            RawReadTape t(Raw{ser_comp.data.data, ser_comp.data.size});

            // Get the component's id and descriptor by its type name
            ecs_id_t     id   = type_name_to_id[ser_comp.type_name];
            IDescriptor* desc = id_to_desc[id];

            // Get the associated type info
            const ecs_type_info_t* type_info =
                ecs_get_type_info(world.c_ptr(), id);

            // @todo: need a copy_object(descriptor, dst, src) to copy fields
            // from an object to another For now, leaking memory is fine (:
            umm ptr = temp.reserve(type_info->size);
            memset(ptr, 0, type_info->size);

            ASSERT(archive_deserialize(&t, System_Allocator, desc, ptr));
            ecs_add_id(world.c_ptr(), entity.id(), type_info->component);

            ecs_set_id(world.c_ptr(), entity.id(), id, type_info->size, ptr);
        }
    }
}

void WorldSerializer::deinit() { id_to_desc.release(); }
