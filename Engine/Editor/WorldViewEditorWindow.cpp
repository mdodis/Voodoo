#include "WorldViewEditorWindow.h"

#include "Engine.h"
#include "Gizmos.h"
#include "MenuRegistrar.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

void WorldViewEditorWindow::init()
{
    Engine& eng      = engine();
    eng.do_blit_pass = false;

    viewport_texture_ref = (void*)ImGui_ImplVulkan_AddTexture(
        eng.present_pass.texture_sampler,
        eng.color_pass.color_image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void WorldViewEditorWindow::draw()
{
    window_pos.x = ImGui::GetWindowPos().x;
    window_pos.y = ImGui::GetWindowPos().y;

    Engine& eng = engine();

    ImVec2 viewport = ImGui::GetContentRegionAvail();
    if (viewport_width != (u32)viewport.x || viewport_height != (u32)viewport.y)
    {
        viewport_width  = viewport.x;
        viewport_height = viewport.y;

        ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)viewport_texture_ref);

        eng.resize_offscreen_buffer(viewport_width, viewport_height);

        viewport_texture_ref = (void*)ImGui_ImplVulkan_AddTexture(
            eng.present_pass.texture_sampler,
            eng.color_pass.color_image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    ImGui::Image(
        (ImTextureID)viewport_texture_ref,
        ImVec2(viewport_width, viewport_height));

    if (ed::gizmos::box(Vec3::zero(), Quat::identity())) {
    }
}

void WorldViewEditorWindow::deinit()
{
    Engine& eng      = engine();
    eng.do_blit_pass = true;

    eng.resize_offscreen_buffer(eng.extent.width, eng.extent.height);
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)viewport_texture_ref);
}

Ray WorldViewEditorWindow::ray_from_mouse_pos()
{
    Engine& eng = engine();

    Vec2 imgui_mouse_pos = {
        ImGui::GetMousePos().x,
        ImGui::GetMousePos().y,
    };

    ImGuiStyle& style     = ImGui::GetStyle();
    float titlebar_height = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

    imgui_mouse_pos.x -= window_pos.x;
    imgui_mouse_pos.y -= (window_pos.y + titlebar_height);

    Vec2 mouse_pos = {
        imgui_mouse_pos.x / (float(viewport_width) * 0.5f) - 1.0f,
        imgui_mouse_pos.y / (float(viewport_height) * 0.5f) - 1.0f,
    };

    Mat4 inv = Mat4(eng.debug_camera.proj * eng.debug_camera.view).inverse();
    Vec4 screen_pos = Vec4(mouse_pos.x, mouse_pos.y, 1.0f, 1.0f);
    Vec4 world_pos  = Vec4(inv * screen_pos);

    Vec3 direction = normalize(world_pos.xyz());

    return Ray{
        .origin    = eng.debug_camera.position,
        .direction = direction,
    };

#if 0

    // convert coordinates to window coordinates
    mouse_pos = window_pos - mouse_pos;

    Vec4 viewport(0, 0, viewport_width, viewport_height);
    Vec3 win_coords(mouse_pos, 0.0f);

    Ray ray;
    ray.origin = glm::unProject(
        win_coords,
        eng.debug_camera.view,
        eng.debug_camera.proj,
        viewport);
    win_coords.z = 1.0f;

    Vec3 end = glm::unProject(
        win_coords,
        eng.debug_camera.view,
        eng.debug_camera.proj,
        viewport);

    ray.direction = normalize(end - ray.origin);

    return ray;
#endif
}

STATIC_MENU_ITEM(
    "Window/World", { The_Editor.create_window<WorldViewEditorWindow>(); }, 0);
