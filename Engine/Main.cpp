#include <glm/gtc/matrix_transform.hpp>

#include "Base.h"
#include "Engine.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Reflection.h"
#include "Serialization/JSON.h"
#include "imgui.h"

struct {
    Input  input{System_Allocator};
    Window window = {
        .input = &input,
    };
} G;

int main(int argc, char const* argv[])
{
    int width  = 640;
    int height = 480;

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    G.window.init(width, height).unwrap();

    Engine eng = {
        .window            = &G.window,
        .input             = &G.input,
        .validation_layers = true,
        .allocator         = System_Allocator,
    };
    eng.init();
    DEFER(eng.deinit());

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

    bool show_demo = true;

    G.window.poll();
    while (G.window.is_open) {
        G.input.update();
        eng.update();

        eng.imgui_new_frame();
        G.window.imgui_new_frame();

        ImGui::NewFrame();

        ImGui::ShowDemoWindow(&show_demo);
        ImGui::Render();
        eng.draw();
        G.window.poll();
    }

    print(LIT("Closing...\n"));
    return 0;
}
