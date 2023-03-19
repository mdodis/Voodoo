#include <glm/gtc/matrix_transform.hpp>

#include "Base.h"
#include "ECS.h"
#include "Engine.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Memory/AllocTape.h"
#include "Reflection.h"
#include "Serialization/JSON.h"
#include "imgui.h"

struct {
    Input  input{System_Allocator};
    win::Window *window;

    ECS  ecs;
    bool imgui_demo = false;
} G;

int main(int argc, char const* argv[])
{
    int width  = 1600;
    int height = 900;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
    ImGui::StyleColorsDark();

    G.window = win::create_window(System_Allocator);
    G.window->input = &G.input;
    G.window->init(width, height).unwrap();

    // Initialize ECS
    G.ecs.init();
    {
        CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(1));
        for (int i = 0; i < 10; ++i) {
            SAVE_ARENA(temp);

            Str  entity_name = format(temp, LIT("Entity #{}\0"), i);
            auto ent         = G.ecs.create_entity(entity_name);
            ent.set<TransformComponent>({{i, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
        }
    }

    Engine eng = {
        .window            = G.window,
        .input             = &G.input,
        .validation_layers = true,
        .allocator         = System_Allocator,
    };
    eng.init();
    DEFER(eng.deinit());

    // Add render objects
    RenderObject monke = {
        .mesh      = eng.get_mesh(LIT("monke")),
        .material  = eng.get_material(LIT("default.mesh")),
        .transform = glm::mat4(1.0f),
    };
    RenderObject lost_empire = {
        .mesh      = eng.get_mesh(LIT("lost_empire")),
        .material  = eng.get_material(LIT("default.mesh.textured")),
        .transform = glm::mat4(1.f),
    };

    eng.render_objects.add(monke);
    eng.render_objects.add(lost_empire);
    for (int x = -20; x <= 20; ++x) {
        for (int y = -20; y <= 20; ++y) {
            glm::mat4 transform =
                glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3((float)x, -2.f, (float)y)) *
                glm::scale(glm::mat4(1.f), glm::vec3(.2f));

            RenderObject triangle = {
                .mesh      = eng.get_mesh(LIT("triangle")),
                .material  = eng.get_material(LIT("default.mesh")),
                .transform = transform,
            };

            eng.render_objects.add(triangle);
        }
    }

    G.window->poll();
    while (G.window->is_open) {
        G.input.update();
        eng.update();

        G.ecs.run();

        eng.imgui_new_frame();
        G.window->imgui_new_frame();

        ImGui::NewFrame();

        {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
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
