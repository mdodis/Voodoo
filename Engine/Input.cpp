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
            action.delta      = (new_value - action.last_value);
            action.last_value = new_value;
        }
    }
}

void Input::send_axis_delta(InputAxis axis, float delta)
{
    for (auto& action : axis_motion_actions) {
        if (action.axis == axis) {
            action.delta += delta * action.scale;
        }
    }
}

void Input::update()
{
    for (auto& action : axis_motion_actions) {
        action.callback.call(action.delta);
        action.delta = 0.f;
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
