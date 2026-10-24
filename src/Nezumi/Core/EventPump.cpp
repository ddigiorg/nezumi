// =============================================================================
// Pump.cpp
// =============================================================================

#include "Nezumi/Core/Events/OnGameExited.hpp"
#include "Nezumi/Core/Events/OnWindowResized.hpp"
#include "Nezumi/Core/EventPump.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnKeyUp.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonUp.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Input/Events/OnMouseWheel.hpp"
#include "Nezumi/Input/Events/OnTextInput.hpp"
#include "Nezumi/Input/Mouse.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include <SDL3/SDL.h>
#include <cstring> // strncpy

namespace Nezumi {

void EventPump::poll_events() {
    SDL_Event sdl;

    while (SDL_PollEvent(&sdl)) { switch (sdl.type) {

        // ---------------------------------------------------------------------
        // Application events
        // ---------------------------------------------------------------------

        case SDL_EVENT_QUIT: {
            events_.dispatch(OnGameExited{});
            break;
        }

        // ---------------------------------------------------------------------
        // Window events
        // ---------------------------------------------------------------------

        // case SDL_EVENT_WINDOW_SHOWN: {
        //     break;
        // }

        // case SDL_EVENT_WINDOW_HIDDEN: {
        //     break;
        // }

        case SDL_EVENT_WINDOW_RESIZED: {
            events_.dispatch(OnWindowResized{
                .width  = sdl.window.data1,
                .height = sdl.window.data2,
            });
            break;
        }

        // case SDL_EVENT_WINDOW_MINIMIZED: {
        //     break;
        // }

        // case SDL_EVENT_WINDOW_MAXIMIZED: {
        //     break;
        // }

        // case SDL_EVENT_WINDOW_MOUSE_ENTER: {
        //     break;
        // }

        // case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
        //     break;
        // }

        // case SDL_EVENT_WINDOW_FOCUS_GAINED: {
        //     break;
        // }

        // case SDL_EVENT_WINDOW_FOCUS_LOST: {
        //     break;
        // }

        // ---------------------------------------------------------------------
        // Keyboard events
        // ---------------------------------------------------------------------

        case SDL_EVENT_KEY_DOWN: {
            events_.dispatch(OnKeyDown{
                .scancode = static_cast<ScanCode>(sdl.key.scancode),
                .repeat   = sdl.key.repeat,
            });
            break;
        }

        case SDL_EVENT_KEY_UP: {
            events_.dispatch(OnKeyUp{
                .scancode = static_cast<ScanCode>(sdl.key.scancode),
            });
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            OnTextInput e;
            if (sdl.text.text) std::strncpy(e.text, sdl.text.text, sizeof e.text - 1);
            events_.dispatch(e);
            break;
        }

        // ---------------------------------------------------------------------
        // Mouse events
        // ---------------------------------------------------------------------

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            events_.dispatch(OnMouseButtonDown{
                .button = static_cast<MouseButton>(sdl.button.button),
            });
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            events_.dispatch(OnMouseButtonUp{
                .button = static_cast<MouseButton>(sdl.button.button),
            });
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            // SDL reports window points; cameras and the UI work in the
            // pixels the window renders, which differ on HiDPI screens
            SDL_Window* window  = SDL_GetWindowFromID(sdl.motion.windowID);
            const float density = window ? SDL_GetWindowPixelDensity(window) : 0.0f;
            const float s       = density > 0.0f ? density : 1.0f;
            events_.dispatch(OnMouseMotion{
                .pos_x = sdl.motion.x    * s,
                .pos_y = sdl.motion.y    * s,
                .rel_x = sdl.motion.xrel * s,
                .rel_y = sdl.motion.yrel * s,
            });
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            events_.dispatch(OnMouseWheel{
                .rel_x = sdl.wheel.x,
                .rel_y = sdl.wheel.y,
            });
            break;
        }

        // ---------------------------------------------------------------------
        // Gamepad events
        // ---------------------------------------------------------------------

        // ---------------------------------------------------------------------
        // Default
        // ---------------------------------------------------------------------

        default:
            break;
    }}
}

} // namespace Nezumi
