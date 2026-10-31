// =============================================================================
// ViewData.hpp
//
// What any renderer needs to know about one view for one frame: its matrices
// and where it looks from. Feature renderers fill it in from their own camera
// types (Renderer2D from Camera2D; a 3D renderer would from a perspective
// camera) and publish it with RenderCore::set_view(). RenderCore uploads it to
// the view's slot in ViewBindings (bind group 0).
//
// Today the GPU only sees view_proj (see ViewBindings). The other fields are
// here so a 3D renderer can grow the uniform block without changing callers.
// =============================================================================

#pragma once
#include <glm/glm.hpp> // mat4, vec3

namespace Nezumi::Render {

struct ViewData {
    glm::mat4 view      = glm::mat4(1.0f); // world -> view space
    glm::mat4 proj      = glm::mat4(1.0f); // view  -> clip space
    glm::mat4 view_proj = glm::mat4(1.0f); // proj * view
    glm::vec3 position  = { 0.0f, 0.0f, 0.0f }; // camera position, world space
};

} // namespace Nezumi::Render
