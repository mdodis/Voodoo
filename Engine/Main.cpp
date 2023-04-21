#include <glm/gtc/matrix_transform.hpp>

#include "Arg.h"
#include "AssetLibrary/AssetLibrary.h"
#include "Base.h"
#include "ECS.h"
#include "Editor/Editor.h"
#include "Engine.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Memory/AllocTape.h"
#include "Reflection.h"
#include "Serialization/JSON.h"
#include "Time/Time.h"
#include "imgui.h"

struct {
    Input        input{System_Allocator};
    win::Window* window;
    ECS          ecs;
    bool         imgui_demo = false;
} G;

static int run();
static int convert(Slice<Str> args);

int main(int argc, char const* argv[])
{
    TArray<Str> args(&System_Allocator);

    for (int i = 0; i < argc; ++i) {
        args.add(Str(argv[i]));
    }

    if (args.size <= 1) {
        return run();
    }

    Str operation = args[1];

    if (operation == LIT("run")) {
        return run();
    }

    if (operation == LIT("convert")) {
        return convert(slice(args, 2));
    }

    return -1;
}

static int run()
{
    int width  = 1600;
    int height = 900;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
    ImGui::StyleColorsDark();

    // Initialize window
    G.window        = win::create_window(System_Allocator);
    G.window->input = &G.input;
    G.window->init(width, height).unwrap();

    // Initialize engine
    Engine eng = {
        .window            = G.window,
        .input             = &G.input,
        .validation_layers = true,
        .allocator         = System_Allocator,
    };
    eng.init();
    DEFER(eng.deinit());

    // Initialize ECS
    G.ecs.engine = &eng;
    G.ecs.init();
    {
        auto empire = G.ecs.create_entity(LIT("Lost Empire"));
        empire.set<TransformComponent>({{0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
        empire.set<MeshMaterialComponent>({
            .mesh_name     = LIT("lost_empire"),
            .material_name = LIT("default.mesh.textured"),
        });

        const int monke_count = 10;

        flecs::entity last_monke;
        CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(1));
        for (int i = 0; i < monke_count; ++i) {
            SAVE_ARENA(temp);

            Str  entity_name = format(temp, LIT("Entity #{}\0"), i);
            auto ent         = G.ecs.create_entity(entity_name);
            ent.set<TransformComponent>(
                {{i * 5.0f, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
            ent.set<MeshMaterialComponent>({
                .mesh_name     = LIT("monke"),
                .material_name = LIT("default.mesh"),
            });
            ent.child_of(empire);

            if (i == 5) last_monke = ent;
        }

        {
            SAVE_ARENA(temp);

            Str  entity_name = format(temp, LIT("Entity #{}\0"), monke_count);
            auto ent         = G.ecs.create_entity(entity_name);
            ent.set<TransformComponent>(
                {{monke_count * 5.0f, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
            ent.set<MeshMaterialComponent>({
                .mesh_name     = LIT("monke"),
                .material_name = LIT("default.mesh"),
            });
            ent.child_of(last_monke);
        }
    }

    // Initialize Editor
    The_Editor.init(G.window, &eng, &G.ecs);
    DEFER(The_Editor.deinit());

    TimeSpec last_time = now_time();
    G.window->poll();
    while (G.window->is_open) {
        TimeSpec now   = now_time();
        TimeSpec diff  = now - last_time;
        float    delta = (float)diff.to_ms();
        last_time      = now;

        G.input.update();
        eng.update();

        G.ecs.run();

        The_Editor.draw();

        ImGui::Render();
        eng.draw();
        G.window->poll();
    }

    print(LIT("Closing...\n"));
    return 0;
}

static int convert(Slice<Str> args)
{
    CREATE_SCOPED_ARENA(&System_Allocator, temp, MEGABYTES(5));

    ArgCollection arguments;
    arguments.register_arg<Str>(LIT("i"), LIT(""), LIT("Input file "));
    arguments.register_arg<Str>(LIT("o"), LIT(""), LIT("Output asset path"));

    if (!arguments.parse_args(args)) {
        print(LIT("Invalid arguments, exiting.\n"));
        arguments.summary();
        return -1;
    }

    Str  in_path  = *arguments.get_arg<Str>(LIT("i"));
    Str  out_path = *arguments.get_arg<Str>(LIT("o"));
    auto out_tape = BufferedWriteTape<true>(open_file_write(out_path));

    ImporterRegistry registry(temp);
    registry.init_default_importers();
    Asset asset = registry.import_asset_from_file(in_path, temp).unwrap();
    if (!asset.write(temp, &out_tape)) {
        print(LIT("Failed to convert import {}\n"), in_path);
        return -1;
    }

    return 0;
}
