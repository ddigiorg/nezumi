// =============================================================================
// Ktx2.cpp
// =============================================================================

#include "Nezumi/Render/Core/Image/Ktx2.hpp"

#include "Nezumi/Core/BasePath.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <algorithm> // min
#include <array>     // array
#include <cstring>   // memcmp, memcpy
#include <fstream>   // ifstream
#include <iterator>  // istreambuf_iterator
#include <stdexcept> // runtime_error
#include <string>    // string
#include <utility>   // move

// bcdec: header-only BCn block decoder. The implementation lives in this
// translation unit only.
#define BCDEC_IMPLEMENTATION
#include <bcdec.h>

namespace Nezumi::Render {

namespace {

[[noreturn]] void fail(std::string_view name, const std::string& what) {
    throw std::runtime_error("KTX2 '" + std::string(name) + "': " + what);
}

// Little-endian reads from the file, bounds-checked
struct Reader {
    std::span<const std::byte> file;
    std::string_view           name;

    template <typename T>
    T at(std::size_t offset) const {
        if (offset + sizeof(T) > file.size()) fail(name, "truncated");
        T v{};
        std::memcpy(&v, file.data() + offset, sizeof(T)); // KTX2 is little-endian, as is every target
        return v;
    }
};

// The vkFormat values Nezumi reads (VkFormat in the Vulkan spec)
RHI::Format from_vk_format(std::uint32_t vk) {
    switch (vk) {
        case 9:   return RHI::Format::R8Unorm;           // VK_FORMAT_R8_UNORM
        case 37:  return RHI::Format::RGBA8Unorm;        // VK_FORMAT_R8G8B8A8_UNORM
        case 43:  return RHI::Format::RGBA8UnormSrgb;    // VK_FORMAT_R8G8B8A8_SRGB
        case 139: return RHI::Format::BC4RUnorm;         // VK_FORMAT_BC4_UNORM_BLOCK
        case 145: return RHI::Format::BC7RGBAUnorm;      // VK_FORMAT_BC7_UNORM_BLOCK
        case 146: return RHI::Format::BC7RGBAUnormSrgb;  // VK_FORMAT_BC7_SRGB_BLOCK
        default:  return RHI::Format::Undefined;
    }
}

constexpr std::array<std::uint8_t, 12> IDENTIFIER = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A, // «KTX 20»\r\n\x1A\n
};

constexpr std::size_t HEADER_SIZE      = 80; // identifier + 9 u32 + index (4 u32 + 2 u64)
constexpr std::size_t LEVEL_INDEX_SIZE = 24; // byteOffset, byteLength, uncompressedByteLength
constexpr std::uint8_t DF_FLAG_ALPHA_PREMULTIPLIED = 1;

} // namespace

// -----------------------------------------------------------------------------
// Read
// -----------------------------------------------------------------------------

Ktx2Texture read_ktx2(std::span<const std::byte> file, std::string_view name) {
    const Reader r{ file, name };

    if (file.size() < HEADER_SIZE || std::memcmp(file.data(), IDENTIFIER.data(), IDENTIFIER.size()) != 0)
        fail(name, "not a KTX2 file");

    const std::uint32_t vk_format     = r.at<std::uint32_t>(12);
    const std::uint32_t pixel_width   = r.at<std::uint32_t>(20);
    const std::uint32_t pixel_height  = r.at<std::uint32_t>(24);
    const std::uint32_t pixel_depth   = r.at<std::uint32_t>(28);
    const std::uint32_t layer_count   = r.at<std::uint32_t>(32);
    const std::uint32_t face_count    = r.at<std::uint32_t>(36);
    const std::uint32_t level_count   = r.at<std::uint32_t>(40);
    const std::uint32_t supercompress = r.at<std::uint32_t>(44);
    const std::uint32_t dfd_offset    = r.at<std::uint32_t>(48);
    const std::uint32_t dfd_length    = r.at<std::uint32_t>(52);

    Ktx2Texture t;
    t.format = from_vk_format(vk_format);
    if (t.format == RHI::Format::Undefined) fail(name, "unsupported vkFormat " + std::to_string(vk_format));
    if (pixel_width == 0 || pixel_height == 0) fail(name, "not a 2D texture");
    if (pixel_depth != 0)    fail(name, "3D textures aren't supported");
    if (face_count != 1)     fail(name, "cube maps aren't supported");
    if (supercompress != 0)  fail(name, "supercompression isn't supported");

    t.width  = pixel_width;
    t.height = pixel_height;
    t.layers = layer_count ? layer_count : 1;

    // Data format descriptor: the basic block's flags byte holds
    // KHR_DF_FLAG_ALPHA_PREMULTIPLIED. Layout: dfdTotalSize (u32), then the
    // block: vendor/type (u32), version (u16), size (u16), colorModel,
    // colorPrimaries, transferFunction, flags (u8 each).
    if (dfd_length >= 4 + 12) {
        const auto flags = r.at<std::uint8_t>(std::size_t{dfd_offset} + 4 + 11);
        t.premultiplied = (flags & DF_FLAG_ALPHA_PREMULTIPLIED) != 0;
    }

    // Levels: the index lists level 0 first; each level's data holds every
    // layer, in order
    const std::uint32_t levels = level_count ? level_count : 1;
    t.levels.resize(levels);
    for (std::uint32_t m = 0; m < levels; ++m) {
        const std::size_t   entry  = HEADER_SIZE + std::size_t{m} * LEVEL_INDEX_SIZE;
        const auto          offset = r.at<std::uint64_t>(entry);
        const auto          length = r.at<std::uint64_t>(entry + 8);
        const std::uint64_t expect = RHI::get_image_size(t.format, t.level_width(m), t.level_height(m), t.layers);

        if (length != expect)
            fail(name, "level " + std::to_string(m) + " is " + std::to_string(length) +
                       " bytes, expected " + std::to_string(expect));
        if (offset > file.size() || length > file.size() - offset)
            fail(name, "level " + std::to_string(m) + " runs past the end of the file");

        const std::byte* begin = file.data() + offset;
        t.levels[m].assign(begin, begin + length);
    }
    return t;
}

Ktx2Texture load_ktx2(std::string_view path) {
    const std::string full = BasePath::get(path).string();
    std::ifstream in(full, std::ios::binary);
    if (!in) fail(path, "can't open '" + full + "'");

    std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return read_ktx2(std::as_bytes(std::span(bytes)), path);
}

// -----------------------------------------------------------------------------
// Decompress
// -----------------------------------------------------------------------------

Ktx2Texture decompress(Ktx2Texture t) {
    RHI::Format out_format = RHI::Format::Undefined;
    switch (t.format) {
        case RHI::Format::BC4RUnorm:        out_format = RHI::Format::R8Unorm;        break;
        case RHI::Format::BC7RGBAUnorm:     out_format = RHI::Format::RGBA8Unorm;     break;
        case RHI::Format::BC7RGBAUnormSrgb: out_format = RHI::Format::RGBA8UnormSrgb; break;
        default: return t;
    }

    const std::uint32_t in_block  = RHI::get_format_info(t.format).bytes_per_block;
    const std::uint32_t out_pixel = RHI::get_format_info(out_format).bytes_per_block;
    const bool          bc4       = t.format == RHI::Format::BC4RUnorm;

    for (std::uint32_t m = 0; m < t.levels.size(); ++m) {
        const std::uint32_t w  = t.level_width(m);
        const std::uint32_t h  = t.level_height(m);
        const std::uint32_t bw = RHI::get_block_count(t.format, w);
        const std::uint32_t bh = RHI::get_block_count(t.format, h);

        std::vector<std::byte> out(std::size_t{w} * h * out_pixel * t.layers);
        const std::byte* src = t.levels[m].data();

        for (std::uint32_t layer = 0; layer < t.layers; ++layer) {
            std::byte* dst = out.data() + std::size_t{w} * h * out_pixel * layer;
            for (std::uint32_t by = 0; by < bh; ++by) {
                for (std::uint32_t bx = 0; bx < bw; ++bx, src += in_block) {
                    // Decode to a full 4x4 block, then copy the part inside
                    // the level (edge blocks of a small level are partial)
                    std::array<std::uint8_t, 4 * 4 * 4> block{};
                    const int pitch = static_cast<int>(4 * out_pixel);
                    if (bc4) bcdec_bc4(src, block.data(), pitch);
                    else     bcdec_bc7(src, block.data(), pitch);

                    const std::uint32_t cols = std::min(4u, w - bx * 4);
                    const std::uint32_t rows = std::min(4u, h - by * 4);
                    for (std::uint32_t y = 0; y < rows; ++y) {
                        std::memcpy(dst + ((std::size_t{by} * 4 + y) * w + bx * 4) * out_pixel,
                                    block.data() + std::size_t{y} * pitch,
                                    std::size_t{cols} * out_pixel);
                    }
                }
            }
        }
        t.levels[m] = std::move(out);
    }

    t.format = out_format;
    return t;
}

} // namespace Nezumi::Render
