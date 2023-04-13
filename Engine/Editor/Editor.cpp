#include "Editor.h"

#include "Engine.h"
#include "MenuRegistrar.h"
#include "Window/Window.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "portable-file-dialogs.h"

void Editor::init(win::Window* host_window, Engine* engine, ECS* ecs)
{
    host.window = host_window;
    host.engine = engine;
    host.ecs    = ecs;

    windows.alloc = &System_Allocator;
    textures.init(System_Allocator);
    importers.init_default_importers();

    // Editor queries
    {
        queries.entity_view_query =
            ecs->world.query_builder<const EditorSelectableComponent>()
                .read<EditorSelectableComponent>()
                .build();
        queries.component_view_query =
            ecs->world.query_builder<flecs::Component>()
                .term(flecs::ChildOf, flecs::Flecs)
                .oper(flecs::Not)
                .self()
                .parent()
                .build();
    }

    next_window_id = 1;

    add_texture_from_file(LIT("EditorAssets/DnD.png"), LIT("icons.dnd"));
}

void Editor::draw()
{
    // Begin frame
    host.engine->imgui_new_frame();
    host.window->imgui_new_frame();

    ImGui::NewFrame();

    host.window->imgui_process();

    get_menu_registrar()->draw();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport(
        viewport,
        ImGuiDockNodeFlags_PassthruCentralNode);

    if (imgui_demo) {
        ImGui::ShowDemoWindow(&imgui_demo);
    }

    // Draw windows
    for (EditorWindow* window : windows) {
        if (!window->is_valid()) continue;

        bool is_window_open = true;
        ImGui::Begin(window->name().data, &is_window_open);

        if (is_window_open) {
            window->draw();
        } else {
            window->kill();
        }

        ImGui::End();
    }

    // Free windows with invalid ids
    u64 i = windows.size;
    while (i != 0) {
        i--;
        EditorWindow* window = windows[i];

        if (!window->is_valid()) {
            window->deinit();
            System_Allocator.release((umm)window);
            windows.del(i);
        }
    }
}

void Editor::add_window(EditorWindow* window)
{
    window->id = next_window_id++;
    windows.add(window);
    window->init();
}

void Editor::kill_window(EditorWindow* window) { window->id = 0; }

void Editor::add_texture_from_file(Str path, Str name)
{
    Asset texture_asset =
        importers.import_asset_from_file(path, System_Allocator).unwrap();
    DEFER(System_Allocator.release(texture_asset.blob.ptr));

    AllocatedImage image = host.engine->upload_image(texture_asset).unwrap();

    VkImageView           view;
    VkImageViewCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = 0,
        .flags    = 0,
        .image    = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };
    VK_CHECK(vkCreateImageView(host.engine->device, &create_info, 0, &view));

    VkSamplerCreateInfo sampler_create_info =
        make_sampler_create_info(VK_FILTER_LINEAR);

    VkSampler sampler;
    VK_CHECK(vkCreateSampler(
        host.engine->device,
        &sampler_create_info,
        0,
        &sampler));

    EditorTexture editor_texture = {
        .image      = image,
        .view       = view,
        .sampler    = sampler,
        .descriptor = ImGui_ImplVulkan_AddTexture(
            sampler,
            view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
    };

    textures.add(name, editor_texture);
}

EditorTexture* Editor::get_texture(Str name) { 
    if (textures.contains(name)) return &textures[name];
    return nullptr;
}

void Editor::deinit() { 
    windows.release(); 

    for (auto pair : textures) {
        vkDestroyImageView(host.engine->device, pair.val.view, nullptr);
        vkDestroySampler(host.engine->device, pair.val.sampler, nullptr);
    }
}

Editor The_Editor;

/* Default menu items */

STATIC_MENU_ITEM(
    "File/Quit", { The_Editor.host.window->is_open = false; }, -200);

STATIC_MENU_ITEM(
    "File/Load World...",
    {
        auto f =
            pfd::open_file("Open world", "", {"ASSET file (.asset)", "*.asset"})
                .result();
        if (!f.empty()) {
            const char* open_file = f[0].c_str();
            The_Editor.host.ecs->open_world(Str(open_file));
        }
    },
    0);

STATIC_MENU_ITEM(
    "File/Save World...",
    {
        auto f = pfd::save_file(
                     "Save world to",
                     "",
                     {"ASSET File (.asset)", "*.asset"},
                     pfd::opt::none)
                     .result();

        if (!f.empty()) {
            const char* save_file = f.c_str();
            The_Editor.host.ecs->save_world(Str(save_file));
        }
    },
    0);

STATIC_MENU_ITEM(
    "Help/ImGui", { The_Editor.imgui_demo = !The_Editor.imgui_demo; }, -200);

flecs::world& EditorWindow::editor_world() const
{
    return The_Editor.host.ecs->world;
}

ECS& EditorWindow::ecs() const { return *The_Editor.host.ecs; }
