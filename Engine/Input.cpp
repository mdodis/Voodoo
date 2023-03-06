#include "Input.h"

void Input::send_input(InputKey key, bool press_state)
{
    for (auto& action : digital_continuous_actions) {
        if (action.key == key) {
            action.current_state = press_state;
        }
    }

    for (auto& action : digital_stroke_actions) {
        if (action.key == key) {
            if ((action.current_state == false) && press_state) {
                action.changed = true;
            }
            action.current_state = press_state;
        }
    }
}

void Input::send_axis(InputAxis axis, float new_value)
{
    for (auto& action : axis_motion_actions) {
        if (action.axis == axis) {
            action.delta        = (new_value - action.last_value);
            action.last_value   = new_value;
            action.needs_update = true;
        }
    }
}

void Input::send_axis_delta(InputAxis axis, float delta)
{
    for (auto& action : axis_motion_actions) {
        if (action.axis == axis) {
            action.delta        = delta;
            action.needs_update = true;
        }
    }
}

void Input::update()
{
    for (auto& action : axis_motion_actions) {
        if (action.needs_update) {
            action.callback.call(action.delta * action.scale);
            action.needs_update = false;
        }
    }

    for (auto& action : digital_continuous_actions) {
        if (action.current_state) {
            action.callback.call();
        }
    }

    for (auto& action : digital_stroke_actions) {
        if (action.changed) {
            action.callback.call();
            action.changed = false;
        }
    }
}
