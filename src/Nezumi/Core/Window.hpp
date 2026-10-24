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
    int  get_width()  const { return width_;             } // points
    int  get_height() const { return height_;            } // points

    // -------------------------------------------------------------------------
    // HiDPI
    //
    // The window's size is in points; what it renders is in pixels. On a
    // HiDPI screen the two differ:
    //
    //   pixel density  pixels per point (2.0 on a 2x Mac, 1.5 on Wayland
    //                  at 150%, 1.0 on Windows, where points are pixels)
    //   display scale  how much bigger than 1x content should be drawn to
    //                  keep its physical size (1.5 at 150% on any OS)
    //
    // Mouse events arrive already converted to pixels (see EventPump), and
    // the renderer and UI scale by the display scale (see Engine).
    // -------------------------------------------------------------------------

    float get_pixel_density() const;
    float get_display_scale() const;

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

    bool high_pixel_density_ = false;

    static constexpr std::size_t CURSOR_COUNT = static_cast<std::size_t>(CursorStyle::Count);
    std::array<SDL_Cursor*, CURSOR_COUNT> cursors_ = {};

    CursorStyle cursor_     = CursorStyle::Arrow;
    bool        text_input_ = false;

    EventListenerID on_window_resize_ = {};
};

} // namespace Nezumi
