#include <glm/gtc/matrix_transform.hpp>

#include "Arg.h"
#include "AssetLibrary/AssetLibrary.h"
#include "Base.h"
#include "ECS.h"
#include "Engine.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Memory/AllocTape.h"
#include "Reflection.h"
#include "Serialization/JSON.h"
#include "Time/Time.h"
#include "imgui.h"
#include "portable-file-dialogs.h"

struct {
    Input            input{System_Allocator};
    win::Window*     window;
    ImporterRegistry importers{System_Allocator};

    ECS  ecs;
    bool imgui_demo = false;
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

    // Initialize importers
    G.importers.init_default_importers();

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
        CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(1));
        for (int i = 0; i < 10; ++i) {
            SAVE_ARENA(temp);

            Str  entity_name = format(temp, LIT("Entity #{}\0"), i);
            auto ent         = G.ecs.create_entity(entity_name);
            ent.set<TransformComponent>(
                {{i * 5.0f, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
        }

        auto ent = G.ecs.create_entity(LIT("Lost Empire"));
        ent.set<TransformComponent>({{0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
        ent.set<MeshMaterialComponent>({
            .mesh     = eng.get_mesh(LIT("lost_empire")),
            .material = eng.get_material(LIT("default.mesh.textured")),
        });
    }

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

        eng.imgui_new_frame();
        G.window->imgui_new_frame();

        ImGui::NewFrame();

        {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Save World...")) {
                        auto f = pfd::save_file(
                                     "Save world to",
                                     "",
                                     {"JSON File (.json)", ",json"},
                                     pfd::opt::force_overwrite)
                                     .result();

                        const char* save_file = f.c_str();
                        G.ecs.save_world(Str(save_file));
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Quit")) {
                        G.window->is_open = false;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help")) {
                    if (ImGui::MenuItem("IMGUI")) {
                        G.imgui_demo = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::DockSpaceOverViewport(
                viewport,
                ImGuiDockNodeFlags_PassthruCentralNode);

            {
                ImGui::Begin("Engine - General");
                ImGui::Text(
                    "Swapchain format: %s",
                    string_VkFormat(eng.swap_chain_image_format));
                ImGui::Text("App time (gpu + cpu): %f", delta);
                ImGui::End();
            }

            if (G.imgui_demo) {
                ImGui::ShowDemoWindow(&G.imgui_demo);
            }
        }
        G.ecs.draw_editor();

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
    auto out_tape = TBufferedFileTape<true>(open_file_write(out_path));

    ImporterRegistry registry(temp);
    registry.init_default_importers();
    Asset asset = registry.import_asset_from_file(in_path, temp).unwrap();
    if (!asset.write(temp, &out_tape)) {
        print(LIT("Failed to convert import {}\n"), in_path);
        return -1;
    }

    return 0;
}
