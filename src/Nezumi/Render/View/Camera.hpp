// =============================================================================
// Camera.hpp
//
// A 2D orthographic camera.
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

    void translate   (glm::vec2 delta   );
    void set_position(glm::vec2 position);
    void set_size    (glm::vec2 size    );
    void set_zoom    (float     factor  );

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    float     get_zoom()     const { return zoom_;         }
    glm::vec2 get_position() const { return position_;     }
    glm::vec2 get_size()     const { return size_;         }
    glm::mat4 get_view()     const { return view_;         }
    glm::mat4 get_proj()     const { return proj_;         }
    glm::mat4 get_vp()       const { return proj_ * view_; }

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

    CameraOrigin origin_   = CameraOrigin::Centered;
    float        zoom_     = 1.0f;
    glm::vec2    position_ = {   0.0f,   0.0f };
    glm::vec2    size_     = { 800.0f, 600.0f };
    glm::mat4    view_     = glm::mat4(1.0f);
    glm::mat4    proj_     = glm::mat4(1.0f);
};

} // namespace Nezumi::Render
