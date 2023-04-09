#pragma once

#include "ECS.h"
#include "Memory/Extras.h"

namespace win {
    struct Window;
}

namespace events {
    struct OnEntitySelectionChanged {};
}  // namespace events


struct EditorWindow {
    u64          id       = 0;
    virtual Str  name()   = 0;
    virtual void init()   = 0;
    virtual void draw()   = 0;
    virtual void deinit() = 0;

    flecs::world& editor_world() const;
    ECS&          ecs() const;

    _inline void kill() { id = 0; }
    _inline bool is_valid() const { return id != 0; }
};

struct Editor {
    void init(win::Window* host_window, struct Engine* engine, struct ECS* ecs);
    void draw();
    void deinit();

    void add_window(EditorWindow* window);
    void kill_window(EditorWindow* window);

    template <typename T>
    _inline T* create_window()
    {
        T* result = alloc<T>(System_Allocator);
        add_window(result);
        return result;
    }

    struct {
        flecs::query<const EditorSelectableComponent> entity_view_query;
        flecs::query<>                                component_view_query;
    } queries;

    struct {
        win::Window*   window;
        struct Engine* engine;
        struct ECS*    ecs;
    } host;

    TArray<EditorWindow*> windows;
    u64                   next_window_id;
    bool                  imgui_demo = false;
};

extern Editor The_Editor;