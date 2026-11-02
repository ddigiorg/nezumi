// =============================================================================
// Ktx2.hpp
//
// Reads KTX2 texture files (Khronos, https://registry.khronos.org/KTX/): the
// GPU-ready format sprite sheet pages ship in, with their mip chain and
// block compression baked in, so loading is a copy instead of a PNG decode
// plus mip generation.
//
//   Ktx2Texture page = load_ktx2("assets/graphics/sprites-0.ktx2");
//   if (!device.info().supports_bc7) page = decompress(std::move(page));
//
// Supported: 2D textures and 2D arrays (no cube maps, no 3D, no
// supercompression), in R8, RGBA8 (UNORM or sRGB), BC4 and BC7 (UNORM or
// sRGB). Anything else throws, naming the file.
//
// The data descriptor's alpha-premultiplied flag is read into
// `premultiplied`; Nezumi's sprite pipeline expects it set on RGBA pages
// (see SpriteSheetConfig.hpp).
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Format.hpp"
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view
#include <vector>      // vector

namespace Nezumi::Render {

struct Ktx2Texture {
    RHI::Format   format        = RHI::Format::Undefined;
    std::uint32_t width         = 0;
    std::uint32_t height        = 0;
    std::uint32_t layers        = 1;     // 1 for a plain 2D texture
    bool          premultiplied = false; // color is premultiplied by alpha

    // Level 0 (full size) first. Each level holds every layer back to back,
    // tightly packed, as RHI texture uploads expect.
    std::vector<std::vector<std::byte>> levels;

    std::uint32_t level_width (std::uint32_t level) const { return width  >> level ? width  >> level : 1; }
    std::uint32_t level_height(std::uint32_t level) const { return height >> level ? height >> level : 1; }
};

// Reads a KTX2 file (relative paths resolve against the base path). Throws
// std::runtime_error if it can't be read or isn't supported.
Ktx2Texture load_ktx2(std::string_view path);

// Parses a KTX2 file already in memory. `name` is used in error messages.
Ktx2Texture read_ktx2(std::span<const std::byte> file, std::string_view name = "ktx2");

// Decodes block-compressed data on the CPU, for GPUs that can't sample it:
// BC4 becomes R8Unorm, BC7 becomes RGBA8Unorm (or RGBA8UnormSrgb). Other
// formats are returned unchanged.
Ktx2Texture decompress(Ktx2Texture texture);

} // namespace Nezumi::Render
