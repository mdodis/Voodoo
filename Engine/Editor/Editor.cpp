#include "Editor.h"

#include "Engine.h"
#include "Gizmos.h"
#include "MenuRegistrar.h"
#include "Window/Window.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "portable-file-dialogs.h"

// clang-format off
static EditorColorScheme Default_Color_Scheme = {
    .border                 = {0.57f, 0.57f, 0.57f, 0.50f},
    .frame_bg               = {0.12f, 0.12f, 0.12f, 0.39f},
    .frame_bg_hovered       = {0.15f, 0.15f, 0.15f, 0.39f},
    .frame_bg_active        = {0.58f, 0.58f, 0.58f, 0.39f},
    .title_bg               = {0.44f, 0.00f, 0.07f, 1.00f},
    .title_bg_active        = {0.94f, 0.00f, 0.16f, 1.00f},
    .check_mark             = {0.94f, 0.00f, 0.16f, 1.00f},
    .slider_grab            = {0.94f, 0.00f, 0.16f, 0.39f},
    .slider_grab_active     = {0.94f, 0.26f, 0.05f, 1.00f},
    .button                 = {0.64f, 0.64f, 0.64f, 0.39f},
    .button_hovered         = {0.64f, 0.64f, 0.64f, 0.71f},
    .button_active          = {0.94f, 0.26f, 0.05f, 1.00f},
    .header                 = {0.44f, 0.00f, 0.07f, 1.00f},
    .header_hovered         = {0.69f, 0.00f, 0.11f, 1.00f},
    .header_active          = {0.94f, 0.26f, 0.05f, 1.00f},
    .resize_grip            = {0.44f, 0.00f, 0.07f, 1.00f},
    .resize_grip_hovered    = {0.69f, 0.00f, 0.11f, 1.00f},
    .resize_grip_active     = {0.94f, 0.26f, 0.05f, 1.00f},
    .tab                    = {0.44f, 0.00f, 0.07f, 1.00f},
    .tab_hovered            = {0.69f, 0.00f, 0.11f, 1.00f},
    .tab_active             = {0.99f, 0.00f, 0.16f, 1.00f},
    .docking_preview        = {0.94f, 0.00f, 0.16f, 1.00f},
    .plot_lines_hovered     = {0.09f, 0.94f, 0.73f, 1.00f},
    .plot_histogram         = {0.94f, 0.00f, 0.16f, 1.00f},
    .plot_histogram_hovered = {0.09f, 0.94f, 0.73f, 1.00f},
    .text_selection_bg      = {0.53f, 0.53f, 0.53f, 0.35f},
    .drag_drop_target       = {0.94f, 0.26f, 0.05f, 1.00f},
};
// clang-format on

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

    apply_color_scheme(Default_Color_Scheme);
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

    if (ed::gizmos::box(glm::vec3(0))) {
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

void Editor::apply_color_scheme(const EditorColorScheme& scheme)
{
    // clang-format off
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Border]               = *((ImVec4*)&scheme.border);
    colors[ImGuiCol_FrameBg]              = *((ImVec4*)&scheme.frame_bg);
    colors[ImGuiCol_FrameBgHovered]       = *((ImVec4*)&scheme.frame_bg_hovered);
    colors[ImGuiCol_FrameBgActive]        = *((ImVec4*)&scheme.frame_bg_active);
    colors[ImGuiCol_TitleBg]              = *((ImVec4*)&scheme.title_bg);
    colors[ImGuiCol_TitleBgActive]        = *((ImVec4*)&scheme.title_bg_active);
    colors[ImGuiCol_CheckMark]            = *((ImVec4*)&scheme.check_mark);
    colors[ImGuiCol_SliderGrab]           = *((ImVec4*)&scheme.slider_grab);
    colors[ImGuiCol_SliderGrabActive]     = *((ImVec4*)&scheme.slider_grab_active);
    colors[ImGuiCol_Button]               = *((ImVec4*)&scheme.button);
    colors[ImGuiCol_ButtonHovered]        = *((ImVec4*)&scheme.button_hovered);
    colors[ImGuiCol_ButtonActive]         = *((ImVec4*)&scheme.button_active);
    colors[ImGuiCol_Header]               = *((ImVec4*)&scheme.header);
    colors[ImGuiCol_HeaderHovered]        = *((ImVec4*)&scheme.header_hovered);
    colors[ImGuiCol_HeaderActive]         = *((ImVec4*)&scheme.header_active);
    colors[ImGuiCol_ResizeGrip]           = *((ImVec4*)&scheme.resize_grip);
    colors[ImGuiCol_ResizeGripHovered]    = *((ImVec4*)&scheme.resize_grip_hovered);
    colors[ImGuiCol_ResizeGripActive]     = *((ImVec4*)&scheme.resize_grip_active);
    colors[ImGuiCol_Tab]                  = *((ImVec4*)&scheme.tab);
    colors[ImGuiCol_TabHovered]           = *((ImVec4*)&scheme.tab_hovered);
    colors[ImGuiCol_TabActive]            = *((ImVec4*)&scheme.tab_active);
    colors[ImGuiCol_DockingPreview]       = *((ImVec4*)&scheme.docking_preview);
    colors[ImGuiCol_PlotLinesHovered]     = *((ImVec4*)&scheme.plot_lines_hovered);
    colors[ImGuiCol_PlotHistogram]        = *((ImVec4*)&scheme.plot_histogram);
    colors[ImGuiCol_PlotHistogramHovered] = *((ImVec4*)&scheme.plot_histogram_hovered);
    colors[ImGuiCol_TextSelectedBg]       = *((ImVec4*)&scheme.text_selection_bg);
    colors[ImGuiCol_DragDropTarget]       = *((ImVec4*)&scheme.drag_drop_target);
    // clang-format on
}

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

EditorTexture* Editor::get_texture(Str name)
{
    if (textures.contains(name)) return &textures[name];
    return nullptr;
}

ImmediateDrawQueue& Editor::immediate_draw_queue() const
{
    return host.engine->imm;
}

void Editor::deinit()
{
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
