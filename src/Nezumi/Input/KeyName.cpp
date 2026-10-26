// =============================================================================
// KeyName.cpp
// =============================================================================

#include "Nezumi/Input/KeyName.hpp"
#include <cstdint> // uint32_t

namespace Nezumi {

std::string_view key_name(ScanCode key) {
    const auto code = static_cast<std::uint32_t>(key);

    // Letters and digits are contiguous: A..Z, then 1..9 and 0
    static constexpr std::string_view LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static constexpr std::string_view DIGITS  = "1234567890";
    if (code >= static_cast<std::uint32_t>(ScanCode::A) && code <= static_cast<std::uint32_t>(ScanCode::Z))
        return LETTERS.substr(code - static_cast<std::uint32_t>(ScanCode::A), 1);
    if (code >= static_cast<std::uint32_t>(ScanCode::Num1) && code <= static_cast<std::uint32_t>(ScanCode::Num0))
        return DIGITS.substr(code - static_cast<std::uint32_t>(ScanCode::Num1), 1);

    switch (key) {
        case ScanCode::Return:       return "Enter";
        case ScanCode::Escape:       return "Esc";
        case ScanCode::Backspace:    return "Bksp";
        case ScanCode::Tab:          return "Tab";
        case ScanCode::Space:        return "Space";
        case ScanCode::Delete:       return "Del";
        case ScanCode::Insert:       return "Ins";
        case ScanCode::Home:         return "Home";
        case ScanCode::End:          return "End";
        case ScanCode::PageUp:       return "PgUp";
        case ScanCode::PageDown:     return "PgDn";
        case ScanCode::Right:        return "Right";
        case ScanCode::Left:         return "Left";
        case ScanCode::Down:         return "Down";
        case ScanCode::Up:           return "Up";
        case ScanCode::F1:           return "F1";
        case ScanCode::F2:           return "F2";
        case ScanCode::F3:           return "F3";
        case ScanCode::F4:           return "F4";
        case ScanCode::F5:           return "F5";
        case ScanCode::F6:           return "F6";
        case ScanCode::F7:           return "F7";
        case ScanCode::F8:           return "F8";
        case ScanCode::F9:           return "F9";
        case ScanCode::F10:          return "F10";
        case ScanCode::F11:          return "F11";
        case ScanCode::F12:          return "F12";
        case ScanCode::LCtrl:        return "Ctrl";
        case ScanCode::RCtrl:        return "RCtrl";
        case ScanCode::LShift:       return "Shift";
        case ScanCode::RShift:       return "RShft";
        case ScanCode::LAlt:         return "Alt";
        case ScanCode::RAlt:         return "RAlt";
        case ScanCode::LGui:         return "Super";
        case ScanCode::RGui:         return "RSupr";
        case ScanCode::Numpad0:      return "N0";
        case ScanCode::Numpad1:      return "N1";
        case ScanCode::Numpad2:      return "N2";
        case ScanCode::Numpad3:      return "N3";
        case ScanCode::Numpad4:      return "N4";
        case ScanCode::Numpad5:      return "N5";
        case ScanCode::Numpad6:      return "N6";
        case ScanCode::Numpad7:      return "N7";
        case ScanCode::Numpad8:      return "N8";
        case ScanCode::Numpad9:      return "N9";
        case ScanCode::NumpadEnter:  return "NEnt";
        case ScanCode::NumpadPlus:   return "N+";
        case ScanCode::NumpadMinus:  return "N-";
        case ScanCode::NumpadStar:   return "N*";
        case ScanCode::NumpadSlash:  return "N/";
        case ScanCode::NumpadPeriod: return "N.";
        case ScanCode::Minus:        return "-";
        case ScanCode::Equals:       return "=";
        case ScanCode::LeftBracket:  return "[";
        case ScanCode::RightBracket: return "]";
        case ScanCode::Backslash:    return "\\";
        case ScanCode::Semicolon:    return ";";
        case ScanCode::Apostrophe:   return "'";
        case ScanCode::Grave:        return "`";
        case ScanCode::Comma:        return ",";
        case ScanCode::Period:       return ".";
        case ScanCode::Slash:        return "/";
        case ScanCode::PrintScreen:  return "PrtSc";
        case ScanCode::ScrollLock:   return "ScrLk";
        case ScanCode::Pause:        return "Pause";
        case ScanCode::CapsLock:     return "Caps";
        case ScanCode::NumLock:      return "NumLk";
        default:                     return "?";
    }
}

std::string_view mouse_name(MouseButton button) {
    switch (button) {
        case MouseButton::Left:   return "M1";
        case MouseButton::Right:  return "M2";
        case MouseButton::Middle: return "M3";
        case MouseButton::X1:     return "M4";
        case MouseButton::X2:     return "M5";
        default:                  return "?";
    }
}

std::string_view input_name(InputSource source) {
    switch (source.kind) {
        case InputSource::Kind::Key:         return key_name  (static_cast<ScanCode   >(source.code));
        case InputSource::Kind::MouseButton: return mouse_name(static_cast<MouseButton>(source.code));
    }
    return "?";
}

} // namespace Nezumi
