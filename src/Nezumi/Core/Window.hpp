// =============================================================================
// Window.hpp
//
// Owns an SDL window. Knows nothing about graphics APIs: whatever a graphics
// backend needs from the window at creation time is supplied through
// WindowConfig::prepare (see RHI::prepare_window), and the context or
// surface itself belongs to the RHI device created for this window.
//
// The device must be destroyed before the window.
// =============================================================================

#pragma once
#include "Nezumi/Core/CursorStyle.hpp"
#include "Nezumi/Core/WindowConfig.hpp"
#include "Nezumi/Core/WindowPrepareFunc.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Event/EventListenerID.hpp"
#include <SDL3/SDL.h>
#include <array> // array

namespace Nezumi {

class Window {
public:
    Window(
        EventBus& events,
        const WindowPrepareFunc& prepare = {},
        const WindowConfig& config = {}
    );

    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    void resize(int w, int h);

    bool is_ready()   const { return window_ != nullptr; }
    int  get_width()  const { return width_;             }
    int  get_height() const { return height_;            }

    SDL_Window* get_sdl_window() const { return window_; }

    // -------------------------------------------------------------------------
    // Cursor
    //
    // System cursors are created on first use and kept until the window is
    // destroyed. Setting the style it already has does nothing, so calling
    // this every frame is cheap.
    // -------------------------------------------------------------------------

    void        set_cursor(CursorStyle style);
    CursorStyle get_cursor() const { return cursor_; }

    // -------------------------------------------------------------------------
    // Text Input
    //
    // While on, typed text arrives as OnTextInput events (and an on-screen
    // keyboard or IME may open). Off by default, so plain key presses don't
    // also produce text. The UI turns it on while a text field has focus.
    // -------------------------------------------------------------------------

    void set_text_input(bool on);
    bool is_text_input() const { return text_input_; }

private:
    void init_sdl_(
        const WindowPrepareFunc& prepare,
        const WindowConfig& config
    );

    void init_events_();

    EventBus&   events_;
    SDL_Window* window_ = nullptr;

    int width_  = 0;
    int height_ = 0;

    static constexpr std::size_t CURSOR_COUNT = static_cast<std::size_t>(CursorStyle::Count);
    std::array<SDL_Cursor*, CURSOR_COUNT> cursors_ = {};

    CursorStyle cursor_     = CursorStyle::Arrow;
    bool        text_input_ = false;

    EventListenerID on_window_resize_ = {};
};

} // namespace Nezumi
