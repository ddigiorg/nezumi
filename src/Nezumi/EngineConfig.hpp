// =============================================================================
// EngineConfig
// =============================================================================

#pragma once
#include "Nezumi/Core/ClockConfig.hpp"
#include "Nezumi/Core/RandomConfig.hpp"
#include "Nezumi/Core/WindowConfig.hpp"
#include "Nezumi/Logger/LoggerConfig.hpp"
#include "Nezumi/Render/2D/Renderer2DConfig.hpp"
#include "Nezumi/Render/Core/RenderCoreConfig.hpp"
#include "Nezumi/RHI/DeviceConfig.hpp"
#include "Nezumi/UI/UIConfig.hpp"

namespace Nezumi {

struct EngineConfig {
    bool enable_graphics   = true;
    bool enable_renderer2d = true; // Renderer2D and the UI (needs graphics)
    bool enable_debug    = true;
    bool escape_exits    = false;

    ClockConfig clock  = {
        .fixed_timestep  = 1.0f / 60.0f, // seconds per fixed update (60 UPS)
        .max_frame_delta = 0.10f,        // clamp frame delta to this (spiral of death prevention)
        .max_accumulator = 0.25f,        // clamp accumulator to this (catch-up prevention)
    };

    LoggerConfig logger = {
        .filepath       = "", // TODO: make string_view
        .min_level      = LoggerLevel::Debug,
        .log_to_console = true,
        .log_to_file    = false,
        .include_time   = true,
        .include_level  = true,
    };

    RandomConfig random = {
        .seed = 0,
    };

    Render::RenderCoreConfig render = {
        .clear_color = { 0.0f, 0.0f, 0.0f, 1.0f },
    };

    Render::Renderer2DConfig renderer2d = {};

    RHI::DeviceConfig device = {
        .name = "Game Device",
        .backend = RHI::BackendType::OpenGL, // TODO: make string_view
        .enable_vsync = true,
        .enable_debug = true,
        .frames_in_flight = 2,
        .srgb_backbuffer = false,
    };

    UI::UIConfig ui = {}; // font, view and layer, style

    WindowConfig window = {
        .title      = "Game Window", // TODO: make string_view
        .width      = 800,
        .height     = 600,
        .min_width  = 320,
        .min_height = 200,
        .mouse_grab = false, // TODO: should default true?
    };
};

} // namespace Nezumi
