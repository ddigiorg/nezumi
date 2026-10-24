// =============================================================================
// Camera.hpp
//
// A 2D orthographic camera.
//
// World units to screen pixels: one world unit spans get_scale() pixels, where
//
//   scale = pixels_per_unit * zoom * display_scale
//
// pixels_per_unit is the camera's fixed convention for the world it looks at
// (e.g. 32 px per tile); zoom is the player-facing factor on top of it.
// display_scale is the screen's HiDPI factor (1.5 at 150%), set by the
// renderer each frame, so the world keeps its physical size on any screen
// while rendering at full resolution. pixels_per_unit is therefore in
// points (1x pixels); get_scale() is in real pixels.
//
// Note: Projections use glm's zero-to-one ("ZO") variants because the RHI's
// clip-space depth range is 0..1 on every backend.
// =============================================================================

#pragma once
#include "Nezumi/Render/View/CameraConfig.hpp"
#include "Nezumi/Render/View/CameraOrigin.hpp"
#include <glm/glm.hpp>

namespace Nezumi::Render {

class Camera {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit Camera(const CameraConfig& config = {});

    // Non-copyable and non-movable
    Camera(const Camera&)            = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&)                 = delete;
    Camera& operator=(Camera&&)      = delete;

    // -------------------------------------------------------------------------
    // Setters
    // -------------------------------------------------------------------------

    // Move by `delta` screen pixels (y up), so the distance on screen is the
    // same at every zoom level.
    void translate(glm::vec2 delta);

    void set_position       (glm::vec2 position);
    void set_size           (glm::vec2 size    );
    void set_zoom           (float     factor  );
    void set_pixels_per_unit(float     pixels  );
    void set_display_scale  (float     scale   );

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    float     get_zoom()            const { return zoom_;                    }
    float     get_pixels_per_unit() const { return pixels_per_unit_;         }
    float     get_display_scale()   const { return display_scale_;           }
    float     get_scale()           const { return pixels_per_unit_ * zoom_ * display_scale_; } // screen px per world unit
    glm::vec2 get_position()        const { return position_;                }
    glm::vec2 get_size()            const { return size_;                    }
    glm::mat4 get_view()            const { return view_;                    }
    glm::mat4 get_proj()            const { return proj_;                    }
    glm::mat4 get_vp()              const { return proj_ * view_;            }

    // Convert a screen-space position (pixels) to world-space
    glm::vec2 screen_to_world(glm::vec2 screen_pos) const;

    // Convert a world-space position to screen-space (pixels)
    glm::vec2 world_to_screen(glm::vec2 world_pos)  const;

    // World-space rectangle the camera sees { min_x, min_y, max_x, max_y }.
    // For culling: skip anything that doesn't overlap it.
    glm::vec4 world_bounds() const;

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    void update_proj_();
    void update_view_();

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    CameraOrigin origin_          = CameraOrigin::Centered;
    float        zoom_            = 1.0f;
    float        pixels_per_unit_ = 1.0f;
    float        display_scale_   = 1.0f;
    glm::vec2    position_        = {   0.0f,   0.0f };
    glm::vec2    size_            = { 800.0f, 600.0f };
    glm::mat4    view_            = glm::mat4(1.0f);
    glm::mat4    proj_            = glm::mat4(1.0f);
};

} // namespace Nezumi::Render
