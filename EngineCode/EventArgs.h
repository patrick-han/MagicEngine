#pragma once


namespace Magic
{

struct KeyEventArgs
{
    enum class KeyState
    {
        KeyPressed
        , KeyReleased
        , None
    };
    KeyEventArgs() = default;
    KeyEventArgs(KeyState _keyState, bool _ctrl, bool _shift, bool _alt) : keyState(_keyState), ctrl(_ctrl), shift(_shift), alt(_alt) {}

    KeyState keyState = KeyState::None;
    unsigned int character;

    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

}