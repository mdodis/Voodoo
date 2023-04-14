#include "ModelToVertexArrayEditorWindow.h"

#include "AssetLibrary/AssetLibrary.h"
#include "Memory/AllocTape.h"
#include "MenuRegistrar.h"
#include "imgui.h"

void ModelToVertexArrayEditorWindow::init() {}

void ModelToVertexArrayEditorWindow::draw()
{
    Str s = LIT("hello");

    EditorTexture* t = The_Editor.get_texture(LIT("icons.dnd"));

    ImGui::Text("Drop a file here to convert it");

    ImVec2 image_size = ImVec2(64, 64);
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - image_size.x) * 0.5f);
    ImGui::Image((ImTextureID)t->descriptor, image_size);

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILES"))
        {
            if (payload->DataSize == sizeof(TArray<Str>)) {
                TArray<Str>* s = (TArray<Str>*)payload->Data;
                if (s->size > 0) {
                    convert_file((*s)[0]);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (current_data.len > 0) {
        ImGuiInputTextFlags text_flags = ImGuiInputTextFlags_ReadOnly;
        ImGui::InputTextMultiline(
            "##source",
            current_data.data,
            current_data.len,
            ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16),
            text_flags);
    }
}

void ModelToVertexArrayEditorWindow::deinit() {}

bool ModelToVertexArrayEditorWindow::convert_file(Str path)
{
    CREATE_SCOPED_ARENA(&System_Allocator, temp, MEGABYTES(2));

    // @todo: dont just assert here
    Asset asset =
        The_Editor.importers.import_asset_from_file(path, temp).unwrap();
    ASSERT(asset.info.kind == AssetKind::Mesh);
    ASSERT(asset.info.mesh.format == core::VertexFormat::P3fN3fC3fU2f);

    const size_t num_vertices = asset.info.mesh.vertex_buffer_size;
    const size_t num_indices  = asset.info.mesh.index_buffer_size;

    Vertex_P3fN3fC3fU2f* vertices = (Vertex_P3fN3fC3fU2f*)asset.blob.ptr;
    u32*                 indices =
        (u32*)asset.blob.ptr + num_vertices * sizeof(Vertex_P3fN3fC3fU2f);

    AllocWriteTape out(System_Allocator);

    format(&out, LIT("static float vertices[] = {\n"));

    for (size_t index = 0; index < num_indices; ++index) {
        const Vertex_P3fN3fC3fU2f& vertex = vertices[index];

        format(
            &out,
            LIT("{}f, {}f, {}f,\n"),
            vertex.position.x,
            vertex.position.y,
            vertex.position.z);
    }
    format(&out, LIT("};\n"));
    if (current_data.len > 0) {
        System_Allocator.release((umm)current_data.data);
    }

    current_data = Str(out.ptr, out.size);

    return true;
}

STATIC_MENU_ITEM(
    "Tools/Generators/Model to Vertex Array",
    { The_Editor.create_window<ModelToVertexArrayEditorWindow>(); },
    -200);
