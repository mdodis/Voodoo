#include "Project.h"

#include "FileSystem/FileSystem.h"
#include "Serialization/JSON.h"

void ProjectManager::switch_project(Str path)
{
    FileReadTape<true> rt(open_file_read(path));

    ASSERT(
        deserialize(&rt, System_Allocator, current_project, json_deserialize));

    current_project.base_directory = directory_of(path);
}