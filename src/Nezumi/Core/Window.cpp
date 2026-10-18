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
