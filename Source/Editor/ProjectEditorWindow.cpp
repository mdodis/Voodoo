#include "ProjectEditorWindow.h"

#include "MenuRegistrar.h"
#include "imgui.h"
#include "portable-file-dialogs.h"

STATIC_MENU_ITEM(
    "Window/Project", { The_Editor.create_window<ProjectEditorWindow>(); }, 0);

void ProjectEditorWindow::init() {}
void ProjectEditorWindow::draw()
{
    if (ImGui::Button("Open Project")) {
        auto f = pfd::open_file(
                     "Open Project",
                     "",
                     {"Project file (.json)", "project.json"})
                     .result();
        if (!f.empty()) {
            const char* open_file = f[0].c_str();
            editor().project.manager.switch_project(Str(open_file));
        }
    }
}
void ProjectEditorWindow::deinit() {}