// =============================================================================
// tests/test_ktx2/main.cpp
//
// Headless checks for the KTX2 reader and the CPU block decoder: header and
// level parsing, the premultiplied flag, BC7 and BC4 decoding (including the
// partial blocks at the edge of a level that isn't a multiple of 4), and the
// errors a broken file must raise. Files are built in memory.
// =============================================================================

#include "Nezumi/Render/Core/Image/Ktx2.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <array>     // array
#include <cstdint>   // uint8_t, uint32_t, uint64_t
#include <cstdio>    // printf
#include <cstdlib>   // EXIT_FAILURE, EXIT_SUCCESS, abs
#include <cstring>   // memcpy
#include <stdexcept> // runtime_error
#include <vector>    // vector

using namespace Nezumi;
using Render::Ktx2Texture;

// -----------------------------------------------------------------------------
// Harness
// -----------------------------------------------------------------------------

static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

template <typename F>
static bool throws(F&& f) {
    try { f(); } catch (const std::runtime_error&) { return true; }
    return false;
}

// -----------------------------------------------------------------------------
// A minimal KTX2 writer: header, level index, a DFD holding only the flags
// byte the reader looks at, then the levels
// -----------------------------------------------------------------------------

using Block = std::array<std::uint8_t, 16>;

// Solid 4x4 BC7 blocks (etcpak, mode 6) and what they decode to
static constexpr Block BC7_ORANGE = { 0x40, 0x32, 0x59, 0x26, 0xCB, 0x64, 0xFE, 0x7F, 0x12, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }; // 200 100  50 255
static constexpr Block BC7_GREEN  = { 0xC0, 0x42, 0xC1, 0xED, 0x7E, 0x3C, 0xFE, 0x7F, 0x12, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }; //  10 220  30 255
static constexpr Block BC7_CLEAR  = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }; //   0   0   0   0
static constexpr Block BC7_PURPLE = { 0x40, 0x10, 0x08, 0x02, 0x01, 0x02, 0x81, 0x40, 0x12, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }; //  64  32 128 128

static constexpr std::array<std::uint8_t, 8> BC4_180 = { 0xB4, 0, 0, 0, 0, 0, 0, 0 };

static void put32(std::vector<std::byte>& f, std::size_t at, std::uint32_t v) { std::memcpy(f.data() + at, &v, 4); }
static void put64(std::vector<std::byte>& f, std::size_t at, std::uint64_t v) { std::memcpy(f.data() + at, &v, 8); }

static std::vector<std::byte> make_ktx2(std::uint32_t vk_format, std::uint32_t w, std::uint32_t h,
                                        std::uint32_t layers, bool premultiplied,
                                        const std::vector<std::vector<std::uint8_t>>& levels)
{
    const std::size_t n          = levels.size();
    const std::size_t dfd_offset = 80 + 24 * n;
    const std::size_t dfd_size   = 4 + 24;
    std::size_t       offset     = dfd_offset + dfd_size;
    offset = (offset + 15) / 16 * 16;

    std::size_t total = offset;
    for (const auto& l : levels) total += (l.size() + 15) / 16 * 16;
    std::vector<std::byte> f(total);

    const std::uint8_t id[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };
    std::memcpy(f.data(), id, 12);
    put32(f, 12, vk_format);
    put32(f, 16, 1);
    put32(f, 20, w);
    put32(f, 24, h);
    put32(f, 28, 0);
    put32(f, 32, layers == 1 ? 0 : layers);
    put32(f, 36, 1);
    put32(f, 40, static_cast<std::uint32_t>(n));
    put32(f, 44, 0);
    put32(f, 48, static_cast<std::uint32_t>(dfd_offset));
    put32(f, 52, static_cast<std::uint32_t>(dfd_size));
    put32(f, dfd_offset, static_cast<std::uint32_t>(dfd_size));
    f[dfd_offset + 4 + 11] = std::byte{ premultiplied ? std::uint8_t(1) : std::uint8_t(0) };

    for (std::size_t m = 0; m < n; ++m) {
        put64(f, 80 + 24 * m,      offset);
        put64(f, 80 + 24 * m + 8,  levels[m].size());
        put64(f, 80 + 24 * m + 16, levels[m].size());
        std::memcpy(f.data() + offset, levels[m].data(), levels[m].size());
        offset += (levels[m].size() + 15) / 16 * 16;
    }
    return f;
}

static std::vector<std::uint8_t> blocks(std::initializer_list<Block> list) {
    std::vector<std::uint8_t> out;
    for (const Block& b : list) out.insert(out.end(), b.begin(), b.end());
    return out;
}

static bool near(const std::byte* px, int r, int g, int b, int a) {
    const auto c = [&](int i) { return static_cast<int>(std::to_integer<std::uint8_t>(px[i])); };
    return std::abs(c(0) - r) <= 2 && std::abs(c(1) - g) <= 2 && std::abs(c(2) - b) <= 2 && std::abs(c(3) - a) <= 2;
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

// 6x5 BC7, 3 levels: 2x2 blocks (the right and bottom ones partial), then
// 3x2 and 1x1, one block each
static void test_bc7() {
    std::printf("bc7\n");
    const auto file = make_ktx2(145, 6, 5, 1, true, {
        blocks({ BC7_ORANGE, BC7_GREEN, BC7_CLEAR, BC7_PURPLE }),
        blocks({ BC7_GREEN }),
        blocks({ BC7_PURPLE }),
    });

    const Ktx2Texture t = Render::read_ktx2(std::as_bytes(std::span(file)), "bc7");
    CHECK(t.format == RHI::Format::BC7RGBAUnorm);
    CHECK(t.width == 6 && t.height == 5 && t.layers == 1);
    CHECK(t.premultiplied);
    CHECK(t.levels.size() == 3);
    CHECK(t.levels[0].size() == 64 && t.levels[1].size() == 16 && t.levels[2].size() == 16);
    CHECK(t.level_width(1) == 3 && t.level_height(1) == 2);
    CHECK(t.level_width(2) == 1 && t.level_height(2) == 1);

    const Ktx2Texture d = Render::decompress(t);
    CHECK(d.format == RHI::Format::RGBA8Unorm);
    CHECK(d.levels[0].size() == 6 * 5 * 4);
    CHECK(d.levels[1].size() == 3 * 2 * 4);
    CHECK(d.levels[2].size() == 4);

    const auto at = [&](std::size_t level, std::uint32_t x, std::uint32_t y) {
        return d.levels[level].data() + (std::size_t{y} * d.level_width(static_cast<std::uint32_t>(level)) + x) * 4;
    };
    CHECK(near(at(0, 0, 0), 200, 100,  50, 255)); // block (0, 0)
    CHECK(near(at(0, 3, 3), 200, 100,  50, 255));
    CHECK(near(at(0, 4, 0),  10, 220,  30, 255)); // block (1, 0), partial: x 4..5
    CHECK(near(at(0, 5, 3),  10, 220,  30, 255));
    CHECK(near(at(0, 0, 4),   0,   0,   0,   0)); // block (0, 1), partial: y 4
    CHECK(near(at(0, 5, 4),  64,  32, 128, 128)); // block (1, 1), the corner pixel
    CHECK(near(at(1, 2, 1),  10, 220,  30, 255));
    CHECK(near(at(2, 0, 0),  64,  32, 128, 128));
}

static void test_bc4_layers() {
    std::printf("bc4, 2 layers\n");
    std::vector<std::uint8_t> level(BC4_180.begin(), BC4_180.end());
    level.insert(level.end(), BC4_180.begin(), BC4_180.end()); // layer 1
    const auto file = make_ktx2(139, 4, 4, 2, false, { level });

    const Ktx2Texture t = Render::read_ktx2(std::as_bytes(std::span(file)), "bc4");
    CHECK(t.format == RHI::Format::BC4RUnorm);
    CHECK(t.layers == 2);
    CHECK(!t.premultiplied);
    CHECK(t.levels[0].size() == 16);

    const Ktx2Texture d = Render::decompress(t);
    CHECK(d.format == RHI::Format::R8Unorm);
    CHECK(d.levels[0].size() == 4 * 4 * 2);
    bool all = true;
    for (std::byte b : d.levels[0]) all = all && std::to_integer<int>(b) == 180;
    CHECK(all);
}

static void test_errors() {
    std::printf("errors\n");
    auto good = make_ktx2(145, 4, 4, 1, true, { blocks({ BC7_ORANGE }) });

    auto bad_id = good;
    bad_id[1] = std::byte{ 'X' };
    CHECK(throws([&] { Render::read_ktx2(std::as_bytes(std::span(bad_id)), "id"); }));

    auto bad_format = good;
    put32(bad_format, 12, 131); // VK_FORMAT_BC1_RGB_UNORM_BLOCK: unsupported
    CHECK(throws([&] { Render::read_ktx2(std::as_bytes(std::span(bad_format)), "format"); }));

    auto bad_length = good;
    put64(bad_length, 80 + 8, 15); // level 0 one byte short
    CHECK(throws([&] { Render::read_ktx2(std::as_bytes(std::span(bad_length)), "length"); }));

    auto truncated = good;
    truncated.resize(truncated.size() - 16); // level data cut off
    CHECK(throws([&] { Render::read_ktx2(std::as_bytes(std::span(truncated)), "truncated"); }));

    auto supercompressed = good;
    put32(supercompressed, 44, 2); // zstd
    CHECK(throws([&] { Render::read_ktx2(std::as_bytes(std::span(supercompressed)), "zstd"); }));

    CHECK(!throws([&] { Render::read_ktx2(std::as_bytes(std::span(good)), "good"); }));
}

static void test_format_sizes() {
    std::printf("format sizes\n");
    CHECK(RHI::get_row_size(RHI::Format::BC7RGBAUnorm, 6) == 32);         // 2 blocks
    CHECK(RHI::get_image_size(RHI::Format::BC7RGBAUnorm, 6, 5) == 64);    // 2x2 blocks
    CHECK(RHI::get_image_size(RHI::Format::BC7RGBAUnorm, 1, 1) == 16);    // one partial block
    CHECK(RHI::get_image_size(RHI::Format::BC4RUnorm, 1024, 1024) == 1024 * 1024 / 2);
    CHECK(RHI::get_image_size(RHI::Format::RGBA8Unorm, 6, 5, 2) == 6 * 5 * 4 * 2);
}

int main() {
    test_bc7();
    test_bc4_layers();
    test_errors();
    test_format_sizes();

    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("all passed\n");
    return EXIT_SUCCESS;
}
