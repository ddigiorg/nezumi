// =============================================================================
// FormatInfo.hpp
//
// Properties of each Format: size per pixel, channel count, and whether it
// is sRGB, depth, or stencil. Plus the size calculations that depend on them.
//
// All sizing of texture data goes through these helpers, so when compressed
// formats are added (where "bytes per pixel" becomes "bytes per 4x4 block")
// only this file changes.
//
// The switch below has no default case on purpose: with -Werror=switch,
// adding a Format without describing it here is a compile error.
// =============================================================================

#pragma once
#include <cstdint>

#include "Nezumi/RHI/Common/Format.hpp"

namespace Nezumi::RHI {

struct FormatInfo {
    std::uint32_t bytes_per_pixel = 0;   // 0 for Format::Undefined
    std::uint32_t channel_count   = 0;
    bool          is_srgb         = false;
    bool          has_depth       = false;
    bool          has_stencil     = false;
};

// -----------------------------------------------------------------------------
// Get Format Info
// -----------------------------------------------------------------------------

constexpr FormatInfo get_format_info(Format format) noexcept {
    switch (format) {
        case Format::Undefined:
            return {};

        case Format::R8Unorm:
            return {
                .bytes_per_pixel = 1,
                .channel_count   = 1,
            };

        case Format::RG8Unorm:
            return {
                .bytes_per_pixel = 2,
                .channel_count   = 2,
            };

        case Format::RGBA8Unorm:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 4,
            };

        case Format::RGBA8UnormSrgb:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 4,
                .is_srgb         = true
            };

        case Format::BGRA8Unorm:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 4
            };

        case Format::BGRA8UnormSrgb:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 4,
                .is_srgb         = true
            };

        case Format::R16Float:
            return {
                .bytes_per_pixel = 2,
                .channel_count   = 1
            };

        case Format::RG16Float:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 2
            };

        case Format::RGBA16Float:
            return {
                .bytes_per_pixel = 8,
                .channel_count   = 4
            };

        case Format::R32Float:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 1
            };

        case Format::RG32Float:
            return {
                .bytes_per_pixel = 8,
                .channel_count   = 2
            };

        case Format::RGBA32Float:
            return {
                .bytes_per_pixel = 16,
                .channel_count   = 4
            };

        case Format::Depth16Unorm:
            return {
                .bytes_per_pixel = 2,
                .channel_count   = 1,
                .has_depth       = true
            };

        case Format::Depth24UnormStencil8:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 2,
                .has_depth       = true,
                .has_stencil     = true
            };

        case Format::Depth32Float:
            return {
                .bytes_per_pixel = 4,
                .channel_count   = 1,
                .has_depth       = true
            };

        // 32-bit depth + 8-bit stencil + 24 bits padding, as GL and most
        // Vulkan drivers store it.
        case Format::Depth32FloatStencil8:
            return {
                .bytes_per_pixel = 8,
                .channel_count   = 2,
                .has_depth       = true,
                .has_stencil     = true
            };
    }

    return {};
}

// -----------------------------------------------------------------------------
// Convenience queries
// -----------------------------------------------------------------------------

constexpr bool is_depth_format(Format format) noexcept {
    return get_format_info(format).has_depth;
}

constexpr bool has_stencil(Format format) noexcept {
    return get_format_info(format).has_stencil;
}

constexpr bool is_srgb(Format format) noexcept {
    return get_format_info(format).is_srgb;
}

// -----------------------------------------------------------------------------
// Get Sizes
//
// For tightly packed data. Returned as 64-bit because width * height * layers
// * bytes can exceed 32 bits for large array textures.
// -----------------------------------------------------------------------------

// Bytes in one row of `width` pixels.
constexpr std::uint64_t get_row_size(
    Format format,
    std::uint32_t width
) noexcept {
    return std::uint64_t{get_format_info(format).bytes_per_pixel} * width;
}

// Bytes in a width x height x depth_or_layers block of pixels.
constexpr std::uint64_t get_image_size(
    Format        format,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t depth_or_layers = 1
) noexcept {
    return get_row_size(format, width) * height * depth_or_layers;
}

// -----------------------------------------------------------------------------
// Sanity checks (evaluated at compile time wherever this header is included)
// -----------------------------------------------------------------------------

static_assert(get_format_info(Format::Undefined).bytes_per_pixel == 0);
static_assert(get_format_info(Format::RGBA8Unorm).bytes_per_pixel == 4);
static_assert(is_srgb(Format::RGBA8UnormSrgb) && !is_srgb(Format::RGBA8Unorm));
static_assert(is_depth_format(Format::Depth32Float) && !has_stencil(Format::Depth32Float));
static_assert(get_image_size(Format::RGBA8Unorm, 256, 256) == 256u * 256u * 4u);

} // namespace Nezumi::RHI
