// =============================================================================
// Window.cpp
// =============================================================================

#include "Nezumi/Core/Events/OnWindowResized.hpp"
#include "Nezumi/Core/Window.hpp"
#include <stdexcept>
#include <string>

namespace Nezumi {

// -----------------------------------------------------------------------------
// Window
// -----------------------------------------------------------------------------

Window::Window(
    EventBus& events,
    const WindowPrepareFunc& prepare,
    const WindowConfig& config)
        : events_(events)
{
    try {
        init_sdl_(prepare, config);
        init_events_();
    }
    catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Window construction failed: ") + e.what());
    }
}

Window::~Window() {
    for (SDL_Cursor*& cursor : cursors_) {
        if (cursor) SDL_DestroyCursor(cursor);
        cursor = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (on_window_resize_)
        events_.unsubscribe(on_window_resize_);

    SDL_Quit();
}

void Window::resize(int w, int h) {
    width_  = w;
    height_ = h;
}

// -----------------------------------------------------------------------------
// Cursor
// -----------------------------------------------------------------------------

static SDL_SystemCursor to_sdl_cursor(CursorStyle style) {
    switch (style) {
        case CursorStyle::Arrow:      return SDL_SYSTEM_CURSOR_DEFAULT;
        case CursorStyle::Text:       return SDL_SYSTEM_CURSOR_TEXT;
        case CursorStyle::Hand:       return SDL_SYSTEM_CURSOR_POINTER;
        case CursorStyle::Move:       return SDL_SYSTEM_CURSOR_MOVE;
        case CursorStyle::ResizeEW:   return SDL_SYSTEM_CURSOR_EW_RESIZE;
        case CursorStyle::ResizeNS:   return SDL_SYSTEM_CURSOR_NS_RESIZE;
        case CursorStyle::ResizeNWSE: return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
        case CursorStyle::ResizeNESW: return SDL_SYSTEM_CURSOR_NESW_RESIZE;
        case CursorStyle::Crosshair:  return SDL_SYSTEM_CURSOR_CROSSHAIR;
        case CursorStyle::NotAllowed: return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
        case CursorStyle::Wait:       return SDL_SYSTEM_CURSOR_WAIT;
        case CursorStyle::Count:      break;
    }
    return SDL_SYSTEM_CURSOR_DEFAULT;
}

void Window::set_cursor(CursorStyle style) {
    if (style == cursor_ || style >= CursorStyle::Count) return;

    SDL_Cursor*& cursor = cursors_[static_cast<std::size_t>(style)];
    if (!cursor) cursor = SDL_CreateSystemCursor(to_sdl_cursor(style));

    // A platform without this cursor keeps the current one
    if (cursor && SDL_SetCursor(cursor)) cursor_ = style;
}

// -----------------------------------------------------------------------------
// Text Input
// -----------------------------------------------------------------------------

void Window::set_text_input(bool on) {
    if (on == text_input_ || !window_) return;

    if (on) SDL_StartTextInput(window_);
    else    SDL_StopTextInput (window_);
    text_input_ = on;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void Window::init_sdl_(
    const WindowPrepareFunc& prepare,
    const WindowConfig& config
) {
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE,       "0");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(
            std::string("SDL_Init failed: ") + SDL_GetError());
    }

    // Graphics backends need to configure SDL after SDL_Init but before the
    // window exists (e.g. OpenGL context attributes), and return the window
    // flags they require.
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (prepare) flags |= static_cast<SDL_WindowFlags>(prepare());

    window_ = SDL_CreateWindow(
        config.title,
        config.width,
        config.height,
        flags
    );

    if (!window_) {
        throw std::runtime_error(
            std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    if (config.min_width > 0 || config.min_height > 0) {
        // if (!SDL_SetWindowMinimumSize(Window_, config.min_width, config.min_height))
        //     logger_->warn("SDL_SetWindowMinimumSize failed: {}", SDL_GetError());
    }

    if (config.mouse_grab) {
        SDL_SetWindowMouseGrab(window_, true);
        SDL_WarpMouseInWindow(window_,
            config.width  * 0.5f,
            config.height * 0.5f);
    }

    width_  = config.width;
    height_ = config.height;
}

void Window::init_events_() {
    on_window_resize_ = events_.subscribe<OnWindowResized>(
        [this](const OnWindowResized& e) {
            this->resize(e.width, e.height);
        });
}

} // namespace Nezumi
