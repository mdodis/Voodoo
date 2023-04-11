#include "ModelToVertexArrayEditorWindow.h"

#include "AssetLibrary/AssetLibrary.h"
#include "MenuRegistrar.h"
#include "imgui.h"

void ModelToVertexArrayEditorWindow::init() {}

void ModelToVertexArrayEditorWindow::draw()
{
    Str            s = LIT("hello");
    
    EditorTexture* t = The_Editor.get_texture(LIT("icons.dnd"));

    ImGui::Text("Drop a file here to convert it");

    ImVec2 image_size = ImVec2(64, 64);
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - image_size.x) * 0.5f);
    ImGui::Image((ImTextureID)t->descriptor, image_size);

    ImGuiInputTextFlags text_flags = ImGuiInputTextFlags_ReadOnly;
    ImGui::InputTextMultiline(
        "##source",
        s.data,
        s.len,
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16),
        text_flags);
}

void ModelToVertexArrayEditorWindow::deinit() {}

STATIC_MENU_ITEM(
    "Tools/Generators/Model to Vertex Array",
    { The_Editor.create_window<ModelToVertexArrayEditorWindow>(); },
    -200);
