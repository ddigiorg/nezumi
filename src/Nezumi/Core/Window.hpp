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
#include "Nezumi/Core/WindowConfig.hpp"
#include "Nezumi/Core/WindowPrepareFunc.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Event/EventListenerID.hpp"
#include <SDL3/SDL.h>

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

    EventListenerID on_window_resize_ = {};
};

} // namespace Nezumi
