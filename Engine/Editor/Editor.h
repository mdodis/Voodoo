#pragma once

#include "AssetLibrary/AssetLibrary.h"
#include "ColorScheme.h"
#include "Containers/Map.h"
#include "ECS.h"
#include "Memory/Extras.h"

namespace win {
    struct Window;
}

namespace events {
    struct OnEntitySelectionChanged {};
}  // namespace events

struct EditorTexture {
    AllocatedImage  image;
    VkImageView     view;
    VkSampler       sampler;
    VkDescriptorSet descriptor;
};

struct EditorWindow {
    u64          id       = 0;
    virtual Str  name()   = 0;
    virtual void init()   = 0;
    virtual void draw()   = 0;
    virtual void deinit() = 0;

    virtual void begin_style() { return; }
    virtual void end_style() { return; }

    struct Engine& engine() const;
    flecs::world&  editor_world() const;
    ECS&           ecs() const;

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
    T* find_window()
    {
        for (EditorWindow* window : windows) {
            if (IS_A(window, T)) {
                return dynamic_cast<T*>(window);
            }
        }

        return nullptr;
    }

    void apply_color_scheme(const EditorColorScheme& scheme);
    void apply_style(const EditorStyle& style);

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

    void           add_texture_from_file(Str path, Str name);
    EditorTexture* get_texture(Str name);

    struct ImmediateDrawQueue& immediate_draw_queue() const;

    ImporterRegistry         importers{System_Allocator};
    TArray<EditorWindow*>    windows;
    TMap<Str, EditorTexture> textures;
    u64                      next_window_id;
    bool                     imgui_demo = false;
};

extern Editor The_Editor;
