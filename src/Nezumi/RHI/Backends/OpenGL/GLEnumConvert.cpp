// =============================================================================
// GLEnumConvert.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLEnumConvert.hpp"

namespace Nezumi::RHI::GL {

// =============================================================================
// Formats
// =============================================================================

GLenum to_gl_internal_format(Format format) noexcept {
    switch (format) {
        case Format::Undefined:            return GL_NONE;

        case Format::R8Unorm:              return GL_R8;
        case Format::RG8Unorm:             return GL_RG8;
        case Format::RGBA8Unorm:           return GL_RGBA8;
        case Format::RGBA8UnormSrgb:       return GL_SRGB8_ALPHA8;
        // GL has no BGRA storage formats; BGRA only exists as an upload
        // layout. Store as RGBA and swizzle on upload (GL_BGRA below).
        case Format::BGRA8Unorm:           return GL_RGBA8;
        case Format::BGRA8UnormSrgb:       return GL_SRGB8_ALPHA8;

        case Format::R16Float:             return GL_R16F;
        case Format::RG16Float:            return GL_RG16F;
        case Format::RGBA16Float:          return GL_RGBA16F;

        case Format::R32Float:             return GL_R32F;
        case Format::RG32Float:            return GL_RG32F;
        case Format::RGBA32Float:          return GL_RGBA32F;

        case Format::Depth16Unorm:         return GL_DEPTH_COMPONENT16;
        case Format::Depth24UnormStencil8: return GL_DEPTH24_STENCIL8;
        case Format::Depth32Float:         return GL_DEPTH_COMPONENT32F;
        case Format::Depth32FloatStencil8: return GL_DEPTH32F_STENCIL8;
    }
    return GL_NONE;
}

GLUploadFormat to_gl_upload_format(Format format) noexcept {
    switch (format) {
        case Format::Undefined:            return {};

        case Format::R8Unorm:              return { GL_RED,  GL_UNSIGNED_BYTE };
        case Format::RG8Unorm:             return { GL_RG,   GL_UNSIGNED_BYTE };
        case Format::RGBA8Unorm:           return { GL_RGBA, GL_UNSIGNED_BYTE };
        case Format::RGBA8UnormSrgb:       return { GL_RGBA, GL_UNSIGNED_BYTE };
        case Format::BGRA8Unorm:           return { GL_BGRA, GL_UNSIGNED_BYTE };
        case Format::BGRA8UnormSrgb:       return { GL_BGRA, GL_UNSIGNED_BYTE };

        case Format::R16Float:             return { GL_RED,  GL_HALF_FLOAT };
        case Format::RG16Float:            return { GL_RG,   GL_HALF_FLOAT };
        case Format::RGBA16Float:          return { GL_RGBA, GL_HALF_FLOAT };

        case Format::R32Float:             return { GL_RED,  GL_FLOAT };
        case Format::RG32Float:            return { GL_RG,   GL_FLOAT };
        case Format::RGBA32Float:          return { GL_RGBA, GL_FLOAT };

        case Format::Depth16Unorm:         return { GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT };
        case Format::Depth24UnormStencil8: return { GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8 };
        case Format::Depth32Float:         return { GL_DEPTH_COMPONENT, GL_FLOAT };
        case Format::Depth32FloatStencil8: return { GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV };
    }
    return {};
}

// =============================================================================
// Textures and samplers
// =============================================================================

GLenum to_gl_texture_target(TextureType type) noexcept {
    switch (type) {
        case TextureType::Texture1D:      return GL_TEXTURE_1D;
        case TextureType::Texture2D:      return GL_TEXTURE_2D;
        case TextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
        case TextureType::TextureCube:    return GL_TEXTURE_CUBE_MAP;
        case TextureType::Texture3D:      return GL_TEXTURE_3D;
    }
    return GL_NONE;
}

GLenum to_gl_min_filter(SamplerFilterMode min, SamplerFilterMode mip) noexcept {
    const bool linear_mip = mip == SamplerFilterMode::Linear;
    switch (min) {
        case SamplerFilterMode::Nearest:
            return linear_mip ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
        case SamplerFilterMode::Linear:
            return linear_mip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST;
    }
    return GL_NEAREST;
}

GLenum to_gl_mag_filter(SamplerFilterMode mag) noexcept {
    switch (mag) {
        case SamplerFilterMode::Nearest: return GL_NEAREST;
        case SamplerFilterMode::Linear:  return GL_LINEAR;
    }
    return GL_NEAREST;
}

GLenum to_gl_address_mode(SamplerAddressMode mode) noexcept {
    switch (mode) {
        case SamplerAddressMode::ClampToEdge:  return GL_CLAMP_TO_EDGE;
        case SamplerAddressMode::Repeat:       return GL_REPEAT;
        case SamplerAddressMode::MirrorRepeat: return GL_MIRRORED_REPEAT;
    }
    return GL_CLAMP_TO_EDGE;
}

// =============================================================================
// Shaders
// =============================================================================

GLenum to_gl_shader_stage(ShaderStageFlags stage) noexcept {
    switch (stage) {
        case ShaderStageFlags::None:     return GL_NONE;
        case ShaderStageFlags::Vertex:   return GL_VERTEX_SHADER;
        case ShaderStageFlags::Fragment: return GL_FRAGMENT_SHADER;
        case ShaderStageFlags::Compute:  return GL_NONE;   // compute shaders are 4.3
    }
    return GL_NONE;   // combinations of bits
}

// =============================================================================
// Vertex input
// =============================================================================

GLVertexFormat to_gl_vertex_format(VertexFormat format) noexcept {
    switch (format) {
        case VertexFormat::Float32:   return { 1, GL_FLOAT,          GL_FALSE, false };
        case VertexFormat::Float32x2: return { 2, GL_FLOAT,          GL_FALSE, false };
        case VertexFormat::Float32x3: return { 3, GL_FLOAT,          GL_FALSE, false };
        case VertexFormat::Float32x4: return { 4, GL_FLOAT,          GL_FALSE, false };

        case VertexFormat::Uint32:    return { 1, GL_UNSIGNED_INT,   GL_FALSE, true  };
        case VertexFormat::Uint32x2:  return { 2, GL_UNSIGNED_INT,   GL_FALSE, true  };
        case VertexFormat::Uint32x3:  return { 3, GL_UNSIGNED_INT,   GL_FALSE, true  };
        case VertexFormat::Uint32x4:  return { 4, GL_UNSIGNED_INT,   GL_FALSE, true  };

        case VertexFormat::Sint32:    return { 1, GL_INT,            GL_FALSE, true  };
        case VertexFormat::Sint32x2:  return { 2, GL_INT,            GL_FALSE, true  };
        case VertexFormat::Sint32x3:  return { 3, GL_INT,            GL_FALSE, true  };
        case VertexFormat::Sint32x4:  return { 4, GL_INT,            GL_FALSE, true  };

        case VertexFormat::Unorm8x4:  return { 4, GL_UNSIGNED_BYTE,  GL_TRUE,  false };
        case VertexFormat::Uint8x4:   return { 4, GL_UNSIGNED_BYTE,  GL_FALSE, true  };
        case VertexFormat::Unorm16x2: return { 2, GL_UNSIGNED_SHORT, GL_TRUE,  false };
        case VertexFormat::Uint16x2:  return { 2, GL_UNSIGNED_SHORT, GL_FALSE, true  };
    }
    return {};
}

GLenum to_gl_index_type(IndexFormat format) noexcept {
    switch (format) {
        case IndexFormat::Uint16: return GL_UNSIGNED_SHORT;
        case IndexFormat::Uint32: return GL_UNSIGNED_INT;
    }
    return GL_UNSIGNED_SHORT;
}

std::uint32_t index_size(IndexFormat format) noexcept {
    switch (format) {
        case IndexFormat::Uint16: return 2;
        case IndexFormat::Uint32: return 4;
    }
    return 2;
}

// =============================================================================
// Fixed-function state
// =============================================================================

GLenum to_gl_primitive_topology(PrimitiveTopology topology) noexcept {
    switch (topology) {
        case PrimitiveTopology::TriangleList:  return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveTopology::LineList:      return GL_LINES;
        case PrimitiveTopology::LineStrip:     return GL_LINE_STRIP;
        case PrimitiveTopology::PointList:     return GL_POINTS;
    }
    return GL_TRIANGLES;
}

GLenum to_gl_cull_face(CullMode mode) noexcept {
    switch (mode) {
        case CullMode::None:  return GL_BACK;
        case CullMode::Front: return GL_FRONT;
        case CullMode::Back:  return GL_BACK;
    }
    return GL_BACK;
}

GLenum to_gl_front_face(FrontFace face) noexcept {
    switch (face) {
        case FrontFace::CounterClockwise: return GL_CCW;
        case FrontFace::Clockwise:        return GL_CW;
    }
    return GL_CCW;
}

GLenum to_gl_compare_op(CompareOp op) noexcept {
    switch (op) {
        case CompareOp::Never:        return GL_NEVER;
        case CompareOp::Less:         return GL_LESS;
        case CompareOp::Equal:        return GL_EQUAL;
        case CompareOp::LessEqual:    return GL_LEQUAL;
        case CompareOp::Greater:      return GL_GREATER;
        case CompareOp::NotEqual:     return GL_NOTEQUAL;
        case CompareOp::GreaterEqual: return GL_GEQUAL;
        case CompareOp::Always:       return GL_ALWAYS;
    }
    return GL_ALWAYS;
}

GLenum to_gl_stencil_op(StencilOp op) noexcept {
    switch (op) {
        case StencilOp::Keep:           return GL_KEEP;
        case StencilOp::Zero:           return GL_ZERO;
        case StencilOp::Replace:        return GL_REPLACE;
        case StencilOp::IncrementClamp: return GL_INCR;
        case StencilOp::DecrementClamp: return GL_DECR;
        case StencilOp::Invert:         return GL_INVERT;
        case StencilOp::IncrementWrap:  return GL_INCR_WRAP;
        case StencilOp::DecrementWrap:  return GL_DECR_WRAP;
    }
    return GL_KEEP;
}

GLenum to_gl_blend_factor(BlendFactor factor) noexcept {
    switch (factor) {
        case BlendFactor::Zero:                  return GL_ZERO;
        case BlendFactor::One:                   return GL_ONE;
        case BlendFactor::SrcColor:              return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:      return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::SrcAlpha:              return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:      return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstColor:              return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:      return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::DstAlpha:              return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:      return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantColor:         return GL_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::SrcAlphaSaturated:     return GL_SRC_ALPHA_SATURATE;
    }
    return GL_ONE;
}

GLenum to_gl_blend_op(BlendOp op) noexcept {
    switch (op) {
        case BlendOp::Add:             return GL_FUNC_ADD;
        case BlendOp::Subtract:        return GL_FUNC_SUBTRACT;
        case BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
        case BlendOp::Min:             return GL_MIN;
        case BlendOp::Max:             return GL_MAX;
    }
    return GL_FUNC_ADD;
}

} // namespace Nezumi::RHI::GL
