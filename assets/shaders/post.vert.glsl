// =============================================================================
// post.vert.glsl
//
// One triangle over the whole target, made from gl_VertexID (no vertex
// buffers). v_uv is 0..1 over the target, (0, 0) at its bottom-left.
// =============================================================================

#version 330 core

out vec2 v_uv;

void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2)); // (0,0) (2,0) (0,2)
    v_uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
