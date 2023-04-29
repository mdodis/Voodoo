#pragma once
#include "Reflection.h"
#include "Str.h"

struct Project {
    Str name = Str::NullStr;

    // Not Serialized

    Str base_directory;

    _inline bool is_valid() const { return name == Str::NullStr; }
};

struct ProjectDescriptor : IDescriptor {
    StrDescriptor name_desc = {OFFSET_OF(Project, name), LIT("name")};

    IDescriptor* descs[1] = {
        &name_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(Project, descs)
};

DEFINE_DESCRIPTOR_OF_INL(Project)

struct ProjectManager {
    Project current_project;

    /**
     * Switch to the target project specified at the path
     * @param path The path to the project.json file
     */
    void switch_project(Str path);

    _inline bool on_valid_project() const { return current_project.is_valid(); }
};
