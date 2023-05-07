#pragma once

#include "Arg.h"
#include "ECS.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Renderer/Renderer.h"
#include "VulkanCommon/VulkanCommon.h"
#include "backends/imgui_impl_vulkan.h"

#if OS_MSWINDOWS
#include <imgui.h>

#include "Compat/Win32Base.h"
#include "Win32ImGUI.h"
#include "Window/Win32Window.h"
#else
#error "This platform is not supported!"
#endif

static struct {
    Engine engine;
} G;

static void on_engine_pre_init(Engine* engine);
static void on_engine_post_update(Engine* engine);
static void on_engine_pre_draw(Engine* engine);

static void on_window_post_init(void* wptr);
static void on_window_pre_poll(void* wptr, void* msg, bool* handled);
static void on_window_drag_enter(void* wptr);
static void on_window_drag_over(void* wptr, i32 x, i32 y);
static void on_window_drag_drop_finish(void* wptr);

static void on_renderer_post_init(Renderer* renderer);
static void on_renderer_post_present_pass(
    Renderer* renderer, VkCommandBuffer cmd);

static void populate_demo_scene();

static int run(void);
static int convert(Slice<Str> args);

int main(int argc, char* argv[])
{
    TArray<Str> args(&System_Allocator);
    for (int i = 0; i < argc; ++i) {
        args.add(Str(argv[i]));
    }

    if (args.size <= 1) {
        return run();
    }

    Str operation = args[1];

    if (operation == LIT("convert")) {
        return convert(slice(args, 2));
    }

    return 0;
}

static int run(void)
{
    // Create imgui context
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
    ImGui::StyleColorsDark();

    // Initialize hooks
    G.engine.hooks.pre_init.add_static(on_engine_pre_init);
    G.engine.hooks.post_update.add_static(on_engine_post_update);
    G.engine.hooks.pre_draw.add_static(on_engine_pre_draw);

    // Initialize engine & editor
    G.engine.init();
    The_Editor.init(G.engine.window, G.engine.renderer, G.engine.ecs);

    populate_demo_scene();

    G.engine.loop();

    // Cleanup
    The_Editor.deinit();
    G.engine.deinit();

    return 0;
}

static int convert(Slice<Str> args)
{
    CREATE_SCOPED_ARENA(System_Allocator, temp, MEGABYTES(5));

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

static void populate_demo_scene()
{
    auto empire = G.engine.ecs->create_entity(LIT("Lost Empire"));
    empire.set<TransformComponent>({{0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
    empire.set<MeshMaterialComponent>({
        .mesh_name     = LIT("lost_empire"),
        .material_name = LIT("default.mesh.textured"),
    });

    empire.set<StaticMeshComponent>({
        .name = LIT("lost_empire"),
    });

    const int monke_count = 10;

    flecs::entity last_monke;
    CREATE_SCOPED_ARENA(System_Allocator, temp, KILOBYTES(1));
    for (int i = 0; i < monke_count; ++i) {
        SAVE_ARENA(temp);

        Str  entity_name = format(temp, LIT("Entity #{}\0"), i);
        auto ent         = G.engine.ecs->create_entity(entity_name);
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
        auto ent         = G.engine.ecs->create_entity(entity_name);
        ent.set<TransformComponent>(
            {{monke_count * 5.0f, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
        ent.set<MeshMaterialComponent>({
            .mesh_name     = LIT("monke"),
            .material_name = LIT("default.mesh"),
        });
        ent.child_of(last_monke);
    }
}

static void on_engine_pre_init(Engine* engine)
{
    engine->window->hooks.post_init.add_static(on_window_post_init);
    engine->window->hooks.pre_poll.add_static(on_window_pre_poll);
    engine->window->hooks.drag_enter.add_static(on_window_drag_enter);
    engine->window->hooks.drag_over.add_static(on_window_drag_over);
    engine->window->hooks.drag_drop_finish.add_static(
        on_window_drag_drop_finish);

    engine->renderer->hooks.post_init.add_static(on_renderer_post_init);
    engine->renderer->hooks.post_present_pass.add_static(
        on_renderer_post_present_pass);
}

static void on_engine_post_update(Engine* engine) {}

static void on_engine_pre_draw(Engine* engine)
{
    ImGui_ImplVulkan_NewFrame();
#if OS_MSWINDOWS
    win32_imgui_new_frame();
#endif

    ImGui::NewFrame();

#if OS_MSWINDOWS
    auto* window = (win::Win32Window*)engine->window;

    if (window->is_dragging_files || window->just_dropped_files) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
            ImGui::SetDragDropPayload(
                "FILES",
                &window->dnd.dropped_files,
                sizeof(TArray<Str>));
            ImGui::BeginTooltip();
            ImGui::Text("Files");
            ImGui::EndTooltip();
            ImGui::EndDragDropSource();
        }
    }

    if (window->just_dropped_files) {
        window->just_dropped_files = false;
        window->is_dragging_files  = false;
    }
#endif

    The_Editor.draw();

    ImGui::Render();
}

static void on_window_post_init(void* wptr)
{
#if OS_MSWINDOWS
    win32_imgui_init(*(Win32::HWND*)wptr);
#endif
}

static void on_window_pre_poll(void* wptr, void* msg, bool* handled)
{
#if OS_MSWINDOWS
    Win32::WndProcParameters* params = (Win32::WndProcParameters*)msg;
    if (win32_imgui_wndproc(
            params->hwnd,
            params->msg,
            params->wparam,
            params->lparam))
    {
        *handled = true;
    }
#endif
}

static void on_window_drag_enter(void* wptr)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(0, true);
}

static void on_window_drag_over(void* wptr, i32 x, i32 y)
{
#if OS_MSWINDOWS
    ImGuiIO& io = ImGui::GetIO();

    Win32::POINT p{x, y};
    Win32::ScreenToClient(*(Win32::HWND*)wptr, &p);
    io.AddMousePosEvent((f32)p.x, (f32)p.y);
#endif
}

static void on_window_drag_drop_finish(void* wptr)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(0, false);
}

static void imgui_check_vk_result(VkResult result)
{
    if (result != VK_SUCCESS) {
        print(LIT("Vk result failed {}\n"), result);
    }
}

static void on_renderer_post_init(Renderer* renderer)
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 1000,
        .poolSizeCount = ARRAY_COUNT(pool_sizes),
        .pPoolSizes    = pool_sizes,
    };

    VkDescriptorPool imgui_pool;
    VK_CHECK(vkCreateDescriptorPool(
        renderer->device,
        &pool_info,
        nullptr,
        &imgui_pool));
    ImGui_ImplVulkan_InitInfo vk_init_info = {
        .Instance        = renderer->instance,
        .PhysicalDevice  = renderer->physical_device,
        .Device          = renderer->device,
        .QueueFamily     = renderer->graphics.family,
        .Queue           = renderer->graphics.queue,
        .DescriptorPool  = imgui_pool,
        .MinImageCount   = 3,
        .ImageCount      = 3,
        .MSAASamples     = VK_SAMPLE_COUNT_1_BIT,
        .CheckVkResultFn = imgui_check_vk_result,
    };
    ASSERT(ImGui_ImplVulkan_Init(
        &vk_init_info,
        renderer->present_pass.render_pass));

    renderer->immediate_submit_lambda([&](VkCommandBuffer cmd) {
        ASSERT(ImGui_ImplVulkan_CreateFontsTexture(cmd));
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    renderer->main_deletion_queue.add_lambda([renderer, imgui_pool]() {
        vkDestroyDescriptorPool(renderer->device, imgui_pool, nullptr);
        ImGui_ImplVulkan_Shutdown();
    });
}

static void on_renderer_post_present_pass(
    Renderer* renderer, VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
