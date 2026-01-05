#pragma once

#include <cstdint>

namespace dirk::Input {

enum Key : std::uint16_t {
    // clang-format off
    Unknown = 0,

    // Letters (USB HID: 0x04-0x1D)
    A = 0x04,
    B, C, D, E, F, G, H, I, J, K, L, M, N,
    O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers (USB HID: 0x1E-0x27)
    D1 = 0x1E,
    D2, D3, D4, D5, D6, D7, D8, D9, D0,

    // Special keys
    Enter = 0x28,
    Escape = 0x29,
    Backspace = 0x2A,
    Tab = 0x2B,
    Space = 0x2C,
    Minus = 0x2D,
    Equal = 0x2E,
    LeftBracket = 0x2F,
    RightBracket = 0x30,
    Backslash = 0x31,
    Semicolon = 0x33,
    Apostrophe = 0x34,
    GraveAccent = 0x35, // `
    Comma = 0x36,
    Period = 0x37,
    Slash = 0x38,
    CapsLock = 0x39,

    // Function keys (USB HID: 0x3A-0x45)
    F1 = 0x3A, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    F13 = 0x68, F14, F15, F16, F17, F18,
    F19, F20, F21, F22, F23, F24,

    // System keys
    PrintScreen = 0x46,
    ScrollLock = 0x47,
    Pause = 0x48,
    Insert = 0x49,
    Home = 0x4A,
    PageUp = 0x4B,
    Delete = 0x4C,
    End = 0x4D,
    PageDown = 0x4E,
    Right = 0x4F,
    Left = 0x50,
    Down = 0x51,
    Up = 0x52,

    // Numpad
    NumLock = 0x53,
    KPDivide = 0x54,
    KPMultiply = 0x55,
    KPMinus = 0x56,
    KPPlus = 0x57,
    KPEnter = 0x58,
    KP1 = 0x59,
    KP2 = 0x5A,
    KP3 = 0x5B,
    KP4 = 0x5C,
    KP5 = 0x5D,
    KP6 = 0x5E,
    KP7 = 0x5F,
    KP8 = 0x60,
    KP9 = 0x61,
    KP0 = 0x62,
    KPPeriod = 0x63,

    // Modifiers (USB HID: 0xE0-0xE7)
    LeftCtrl = 0xE0,
    LeftShift = 0xE1,
    LeftAlt = 0xE2,
    LeftSuper = 0xE3, // Super/Windows/Command
    RightCtrl = 0xE4,
    RightShift = 0xE5,
    RightAlt = 0xE6,
    RightSuper = 0xE7,

    // Media keys
    Mute = 0x7F,
    VolumeUp = 0x80,
    VolumeDown = 0x81,

    NumKeys = 0x82,
    // clang-format on
};

enum KeyState {
    None = -1,
    Pressed,
    Held,
    Released
};

enum CursorMode {
    Normal = 0,
    Hidden = 1,
    Locked = 2,
};

enum class MouseButton : uint16_t {
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    Left = Button0,
    Right = Button1,
    Middle = Button2
};

} // namespace dirk::Input
