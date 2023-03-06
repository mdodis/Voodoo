#pragma once
#include <type_traits>

#include "Containers/Array.h"
#include "Delegates.h"
#include "Platform/Keycodes.h"
#include "Str.h"

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

struct InputDigitalStrokeAction : InputActionBase<> {
    InputKey key;
    bool     current_state;
    bool     changed;
};

struct InputAxisMotionAction : InputActionBase<float> {
    InputAxis axis;
    float     scale;
    float     last_value;
    float     delta;
};

static _inline bool operator==(InputAction& left, InputAction& right)
{
    return left.name == right.name;
}

/**
 * @todo: Separate action and mapping concepts. The action just has a name and a
 * type, while the mapping has the actual key and scale, for example.
 */
struct Input {
    Input(IAllocator& allocator)
        : digital_continuous_actions(&allocator)
        , axis_motion_actions(&allocator)
        , digital_stroke_actions(&allocator)
    {}

    TArray<InputDigitalContinuousAction> digital_continuous_actions;
    TArray<InputAxisMotionAction>        axis_motion_actions;
    TArray<InputDigitalStrokeAction>     digital_stroke_actions;

    template <typename ActionType>
    void add_action(ActionType& action)
    {
        if constexpr (std::is_same_v<ActionType, InputDigitalContinuousAction>)
        {
            digital_continuous_actions.add(action);
        } else if constexpr (std::is_same_v<ActionType, InputAxisMotionAction>)
        {
            axis_motion_actions.add(action);
        } else if constexpr (std::is_same_v<
                                 ActionType,
                                 InputDigitalStrokeAction>)
        {
            digital_stroke_actions.add(action);
        }
    }

    // Helpers

    /**
     * Add a digital continuous action. The callback will be called on each
     * update() that the button state is down.
     * @param name     The name of the action
     * @param key      The input key of the action
     * @param callback Callback delegate
     */
    void add_digital_continuous_action(
        Str                                              name,
        InputKey                                         key,
        InputDigitalContinuousAction::CallbackDelegate&& callback)
    {
        InputDigitalContinuousAction action;
        action.name          = name;
        action.current_state = false;
        action.key           = key;
        action.callback      = callback;
        add_action(action);
    }

    /**
     * Add a digital continuous action. The callback will be called on the
     * update() call that the button state changed from up to down.
     * @param name     The name of the action
     * @param key      The input key of the action
     * @param callback Callback delegate
     */
    void add_digital_stroke_action(
        Str                                          name,
        InputKey                                     key,
        InputDigitalStrokeAction::CallbackDelegate&& callback)
    {
        InputDigitalStrokeAction action;
        action.name          = name;
        action.current_state = false;
        action.changed       = false;
        action.key           = key;
        action.callback      = callback;
        add_action(action);
    }

    /**
     * Add an axis motion action. The callback will be called each time the
     * motion is received.
     * @param name     The name of the action
     * @param axis     The axis enum value
     * @param scale    The reported value will be pre-multiplied by this value
     * @param callback The callback delegate
     */
    void add_axis_motion_action(
        Str                                       name,
        InputAxis                                 axis,
        float                                     scale,
        InputAxisMotionAction::CallbackDelegate&& callback)
    {
        InputAxisMotionAction action;
        action.name       = name;
        action.axis       = axis;
        action.scale      = scale;
        action.delta      = 0.f;
        action.last_value = 0.f;
        action.callback   = callback;
        add_action(action);
    }

    void send_input(InputKey key, bool press_state);
    void send_axis(InputAxis axis, float new_value);
    void send_axis_delta(InputAxis axis, float delta);

    void update();
};