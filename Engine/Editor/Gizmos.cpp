#include "Gizmos.h"

#include "Editor.h"
#include "Engine.h"
namespace ed {
    namespace gizmos {
        bool box(glm::vec3 world_position)
        {
            // @todo: cache view & proj in scene view first
            auto& imm = The_Editor.immediate_draw_queue();
            imm.box(world_position, glm::vec3(1.0f));
            return false;
        }
    }  // namespace gizmos
}  // namespace ed