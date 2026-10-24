// =============================================================================
// Camera.cpp
// =============================================================================

#include "Nezumi/Render/View/Camera.hpp"
#include <cassert>                      // assert
#include <glm/gtc/matrix_transform.hpp> // orthoZO, projectZO, unProjectZO

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Camera::Camera(const CameraConfig& config)
    : origin_         (config.origin)
    , pixels_per_unit_(config.pixels_per_unit)
    , position_       (config.position)
    , size_           (config.size)
{
    assert(pixels_per_unit_ > 0.0f && "Camera: pixels_per_unit must be > 0");
    update_proj_();
    update_view_();
}

// -----------------------------------------------------------------------------
// Setters
// -----------------------------------------------------------------------------

void Camera::translate(glm::vec2 delta) {
    // Pixels to world units, so translation distance on screen is consistent
    // at all zoom levels
    position_ += delta / get_scale();
    update_view_();
}

void Camera::set_position(glm::vec2 position) {
    position_ = position;
    update_view_();
}

void Camera::set_size(glm::vec2 size) {
    if (size == size_) return; // the renderer calls this every frame
    size_ = size;
    update_proj_();
}

void Camera::set_zoom(float zoom) {
    assert(zoom > 0.0f && "Camera: zoom must be > 0");
    zoom_ = zoom;
    update_proj_();
}

void Camera::set_pixels_per_unit(float pixels) {
    assert(pixels > 0.0f && "Camera: pixels_per_unit must be > 0");
    pixels_per_unit_ = pixels;
    update_proj_();
}

void Camera::set_display_scale(float scale) {
    assert(scale > 0.0f && "Camera: display_scale must be > 0");
    if (scale == display_scale_) return; // the renderer calls this every frame
    display_scale_ = scale;
    update_proj_();
}

// -----------------------------------------------------------------------------
// Getters
//
// Screen positions are window pixels with (0, 0) at the TOP-left, y down, as
// SDL reports the mouse. glm's project/unProject use window coordinates with
// (0, 0) at the BOTTOM-left, y up (OpenGL's convention), so y is flipped on
// the way in and on the way out.
// -----------------------------------------------------------------------------

glm::vec2 Camera::screen_to_world(glm::vec2 screen_pos) const {
    const glm::vec4 viewport = { 0.0f, 0.0f, size_.x, size_.y };
    const glm::vec3 world = glm::unProjectZO(
        /*.win      = */glm::vec3(screen_pos.x, size_.y - screen_pos.y, 0.0f),
        /*.model    = */view_,
        /*.proj     = */proj_,
        /*.viewport = */viewport
    );
    return glm::vec2(world);
}

glm::vec2 Camera::world_to_screen(glm::vec2 world_pos) const {
    const glm::vec4 viewport = { 0.0f, 0.0f, size_.x, size_.y };
    const glm::vec3 screen = glm::projectZO(
        /*.obj      = */glm::vec3(world_pos, 0.0f),
        /*.model    = */view_,
        /*.proj     = */proj_,
        /*.viewport = */viewport
    );
    return { screen.x, size_.y - screen.y };
}

glm::vec4 Camera::world_bounds() const {
    // Screen y points down, so the bottom-left corner is (0, height).
    const glm::vec2 a = screen_to_world({ 0.0f,    size_.y });
    const glm::vec2 b = screen_to_world({ size_.x, 0.0f    });
    return { glm::min(a, b), glm::max(a, b) };
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void Camera::update_proj_() {
    // Viewport extent in world units
    const glm::vec2 extent = size_ / get_scale();
    const float     w      = extent.x * 0.5f;
    const float     h      = extent.y * 0.5f;

    switch (origin_) {
        case CameraOrigin::Centered:
            proj_ = glm::orthoZO(
                /*.left   = */-w,
                /*.right  = */ w,
                /*.bottom = */-h,
                /*.top    = */ h,
                /*.zNear  = */-10.0f,
                /*.zFar   = */ 10.0f
            );
            break;

        case CameraOrigin::BottomLeft:
            proj_ = glm::orthoZO(
                /*.left   = */ 0.0f,
                /*.right  = */ extent.x,
                /*.bottom = */ 0.0f,
                /*.top    = */ extent.y,
                /*.zNear  = */-10.0f,
                /*.zFar   = */ 10.0f
            );
            break;

        case CameraOrigin::TopLeft:
            proj_ = glm::orthoZO(
                /*.left   = */ 0.0f,
                /*.right  = */ extent.x,
                /*.bottom = */ extent.y,
                /*.top    = */ 0.0f,
                /*.zNear  = */-10.0f,
                /*.zFar   = */ 10.0f
            );
            break;
    }
}

void Camera::update_view_() {
    view_ = glm::translate(glm::mat4(1.0f), glm::vec3(-position_, 0.0f));
}

} // namespace Nezumi::Render
