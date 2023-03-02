#pragma once
#include <type_traits>

#include "Containers/Array.h"
#include "Delegates.h"
#include "Platform/Keycodes.h"
#include "Str.h"

/*
Input input;
input.provider = window.input_provider;

InputAction<DigitalInput>

input.add_action(move_forward);
input.add_action(press_forward);

*/

enum class InputAxis
{
    MouseX,
    MouseY,
};

struct InputAction {
    Str name;
};

template <typename... DelegateArgs>
struct InputActionBase : InputAction {
    using CallbackDelegate = Delegate<void, DelegateArgs...>;
    CallbackDelegate callback;
};

struct InputDigitalContinuousAction : InputActionBase<> {
    InputKey key;
    bool     current_state;
};

struct InputAxisMotionAction : InputActionBase<float> {
    InputAxis axis;
    bool      needs_update;
    float     scale;
    float     last_value;
    float     delta;
};

static _inline bool operator==(InputAction& left, InputAction& right)
{
    return left.name == right.name;
}

struct Input {
    Input(IAllocator& allocator)
        : digital_continuous_actions(&allocator)
        , axis_motion_actions(&allocator)
    {}

    TArray<InputDigitalContinuousAction> digital_continuous_actions;
    TArray<InputAxisMotionAction>        axis_motion_actions;

    template <typename ActionType>
    void add_action(ActionType& action)
    {
        if constexpr (std::is_same_v<ActionType, InputDigitalContinuousAction>)
        {
            digital_continuous_actions.add(action);
        } else if constexpr (std::is_same_v<ActionType, InputAxisMotionAction>)
        {
            axis_motion_actions.add(action);
        }
    }

    // Helpers

    void add_digital_continuous_action(
        Str                                              name,
        InputKey                                         key,
        InputDigitalContinuousAction::CallbackDelegate&& callback)
    {
        InputDigitalContinuousAction action;
        action.name          = name;
        action.current_state = false;
        action.key           = InputKey::W;
        action.callback      = callback;
        add_action(action);
    }

    void add_axis_motion_action(
        Str                                       name,
        InputAxis                                 axis,
        float                                     scale,
        InputAxisMotionAction::CallbackDelegate&& callback)
    {
        InputAxisMotionAction action;
        action.name         = name;
        action.axis         = axis;
        action.scale        = scale;
        action.delta        = 0.f;
        action.last_value   = 0.f;
        action.needs_update = false;
        action.callback     = callback;
        add_action(action);
    }

    void send_input(InputKey key, bool press_state);
    void send_axis(InputAxis axis, float new_value);

    void update();
};