// =============================================================================
// KeyName.hpp
//
// Short, printable names for keys and mouse buttons, for showing bindings in
// the UI: the "1" in an action bar slot's corner, "Rebind: B" in a settings
// screen. Names follow the US layout's key caps, since scancodes are key
// positions: the key left of 1 is "`" whatever a layout prints on it.
//
//   if (auto source = input.first_binding(Action::UseSlot0))
//       label = Nezumi::input_name(*source); // "1"
//
// At most 5 characters and plain ASCII, so they fit a slot's corner and the
// UI font. Keys without a short name are "?".
// =============================================================================

#pragma once
#include "Nezumi/Input/InputSource.hpp"
#include "Nezumi/Input/Mouse.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include <string_view> // string_view

namespace Nezumi {

std::string_view key_name  (ScanCode    key   );
std::string_view mouse_name(MouseButton button); // "M1" (left) ... "M5"
std::string_view input_name(InputSource source);

} // namespace Nezumi
