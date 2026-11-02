// =============================================================================
// tests/test_sprite_packer/main.cpp
//
// Headless checks for SpritePacker: trimming keeps pivots in the untrimmed
// frame, identical pixels are stored once, animation sheets slice into
// direction-major sprites, every sprite sits on the grid with a full grid of
// transparent space around it, pages shrink to fit or overflow to more, and
// bad input throws. Images are built in memory.
// =============================================================================

#include "Nezumi/Render/2D/Sprite/SpritePacker.hpp"
#include <cmath>     // round
#include <cstdint>   // uint8_t, uint32_t
#include <cstdio>    // printf
#include <cstdlib>   // EXIT_FAILURE, EXIT_SUCCESS
#include <stdexcept> // runtime_error

using namespace Nezumi;
using Render::Image;
using Render::SpritePacker;

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
// Images
// -----------------------------------------------------------------------------

// Transparent w x h with an opaque rect [x0, x1) x [y0, y1) of value v
static Image with_rect(std::uint32_t w, std::uint32_t h,
                       std::uint32_t x0, std::uint32_t y0, std::uint32_t x1, std::uint32_t y1,
                       std::uint8_t v = 200)
{
    Image img = Image::blank(w, h);
    for (std::uint32_t y = y0; y < y1; ++y) {
        for (std::uint32_t x = x0; x < x1; ++x) {
            std::uint8_t* p = img.at(x, y);
            p[0] = v; p[1] = std::uint8_t(x); p[2] = std::uint8_t(y); p[3] = 255;
        }
    }
    return img;
}

// Page pixel box { x0, y0, x1, y1 } of a region
static glm::uvec4 page_box(const SpritePacker::Packed& p, Render::SpriteID id) {
    const Image&    page = p.pages[p.regions[id].layer];
    const glm::vec4 t    = p.regions[id].tex_box;
    return { std::uint32_t(std::round(t.x * page.width)),  std::uint32_t(std::round(t.y * page.height)),
             std::uint32_t(std::round(t.z * page.width)),  std::uint32_t(std::round(t.w * page.height)) };
}

// Every sprite on the grid, with `grid` px of clear space around it
static void check_layout(const SpritePacker::Packed& p, std::uint32_t grid) {
    for (std::size_t i = 0; i < p.regions.size(); ++i) {
        const glm::uvec4 b    = page_box(p, Render::SpriteID(i));
        const Image&     page = p.pages[p.regions[i].layer];
        CHECK(b.x % grid == 0 && b.y % grid == 0);
        CHECK(b.x >= grid && b.y >= grid);
        CHECK(b.z + grid <= page.width && b.w + grid <= page.height);

        // No other sprite's box comes within `grid` of this one's
        for (std::size_t j = 0; j < p.regions.size(); ++j) {
            if (j == i || p.regions[j].layer != p.regions[i].layer) continue;
            const glm::uvec4 o = page_box(p, Render::SpriteID(j));
            if (o == b) continue; // a duplicate, stored once
            const bool apart = o.z + grid <= b.x || b.z + grid <= o.x ||
                               o.w + grid <= b.y || b.w + grid <= o.y;
            CHECK(apart);
        }
    }
    for (const Image& page : p.pages) {
        CHECK(page.width % grid == 0 && page.height % grid == 0);
    }
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

static void test_trim_and_pivot() {
    std::printf("trim and pivot\n");
    SpritePacker packer;
    // 32x64 frame, opaque 8..20 x 10..60
    const auto a = packer.add_sprite("a", with_rect(32, 64, 8, 10, 20, 60));
    const auto b = packer.add_sprite("b", with_rect(32, 64, 8, 10, 20, 60), { .pivot = glm::vec2{ 4.0f, 5.0f } });
    const auto c = packer.add_sprite("c", with_rect(32, 64, 8, 10, 20, 60), { .trim = false });
    const auto d = packer.add_sprite("d", Image::blank(16, 16)); // fully transparent

    const SpritePacker::Packed p = packer.pack(8);
    CHECK(p.sprite_names[a] == "a" && p.sprite_names[d] == "d");

    CHECK(p.regions[a].size   == glm::vec2(12.0f, 50.0f));
    CHECK(p.regions[a].offset == glm::vec2(8.0f, 10.0f));
    CHECK(p.regions[a].pivot  == glm::vec2(16.0f, 64.0f)); // bottom center of the frame
    CHECK(p.regions[b].pivot  == glm::vec2(4.0f, 5.0f));
    CHECK(p.regions[c].size   == glm::vec2(32.0f, 64.0f));
    CHECK(p.regions[c].offset == glm::vec2(0.0f));
    CHECK(p.regions[d].size   == glm::vec2(1.0f));

    // a and b are the same pixels: stored once
    CHECK(packer.unique_count() == 3);
    CHECK(page_box(p, a) == page_box(p, b));

    // The trimmed pixels landed on the page
    const glm::uvec4 box  = page_box(p, a);
    const Image&     page = p.pages[0];
    CHECK(page.at(box.x, box.y)[0] == 200 && page.at(box.x, box.y)[1] == 8 && page.at(box.x, box.y)[2] == 10);
    CHECK(page.at(box.x - 1, box.y)[3] == 0);
    check_layout(p, 8);
}

static void test_animation_sheet() {
    std::printf("animation sheet\n");
    // 3 frames x 5 directions of 16x32; cell (f, d) has a rect whose size
    // depends on both, so none dedupe
    Image sheet = Image::blank(48, 160);
    for (std::uint32_t d = 0; d < 5; ++d) {
        for (std::uint32_t f = 0; f < 3; ++f) {
            for (std::uint32_t y = 0; y < 4 + d; ++y) {
                for (std::uint32_t x = 0; x < 2 + f; ++x) {
                    std::uint8_t* px = sheet.at(f * 16 + 4 + x, d * 32 + 20 + y);
                    px[0] = std::uint8_t(10 * d + f); px[3] = 255;
                }
            }
        }
    }

    SpritePacker packer;
    packer.add_sprite("before", with_rect(8, 8, 0, 0, 8, 8));
    const auto id = packer.add_animation("walk", sheet, {
        .frame_width = 16, .frame_height = 32, .frames = 3, .directions = 5, .headings = 8,
        .fps = 10.0f, .pivot = glm::vec2{ 8.0f, 28.0f },
    });

    const SpritePacker::Packed p = packer.pack(4);
    const Render::Animation& a = p.animations[id];
    CHECK(p.animation_names[id] == "walk");
    CHECK(a.first == 1 && a.frames == 3 && a.directions == 5 && a.headings == 8 && a.mirrored());
    CHECK(a.fps == 10.0f && a.loop);

    // Direction-major: first + d * frames + f, named walk/<d>/<f>
    for (std::uint32_t d = 0; d < 5; ++d) {
        for (std::uint32_t f = 0; f < 3; ++f) {
            const Render::SpriteID s = a.first + d * a.frames + f;
            CHECK(p.sprite_names[s] == "walk/" + std::to_string(d) + "/" + std::to_string(f));
            CHECK(p.regions[s].size   == glm::vec2(float(2 + f), float(4 + d)));
            CHECK(p.regions[s].offset == glm::vec2(4.0f, 20.0f)); // within its own cell
            CHECK(p.regions[s].pivot  == glm::vec2(8.0f, 28.0f));
            const glm::uvec4 b = page_box(p, s);
            CHECK(p.pages[p.regions[s].layer].at(b.x, b.y)[0] == 10 * d + f);
        }
    }
    check_layout(p, 4);

    // Cell size defaults to the image split evenly
    SpritePacker even;
    even.add_animation("walk", sheet, { .frames = 3, .directions = 5 });
    CHECK(even.pack(4).regions[14].offset == glm::vec2(4.0f, 20.0f));
}

static void test_pages() {
    std::printf("pages\n");
    // One page: shrunk to what it uses
    SpritePacker small({ .page_size = 256 });
    small.add_sprite("a", with_rect(20, 20, 0, 0, 20, 20));
    const SpritePacker::Packed one = small.pack(8);
    CHECK(one.pages.size() == 1);
    CHECK(one.pages[0].width == 8 + 24 + 8 && one.pages[0].height == 8 + 24 + 8);

    // Ten 100x100 sprites on 256 px pages (2 x 2 per page): three full pages
    SpritePacker many({ .page_size = 256 });
    for (int i = 0; i < 10; ++i) {
        many.add_sprite("s" + std::to_string(i), with_rect(100, 100, 0, 0, 100, 100, std::uint8_t(i)));
    }
    const SpritePacker::Packed three = many.pack(8);
    CHECK(three.pages.size() == 3);
    for (const Image& page : three.pages) CHECK(page.width == 256 && page.height == 256);
    check_layout(three, 8);
}

static void test_errors() {
    std::printf("errors\n");
    SpritePacker packer({ .page_size = 64 });
    packer.add_sprite("a", with_rect(8, 8, 0, 0, 8, 8));
    CHECK(throws([&] { packer.add_sprite("a", with_rect(8, 8, 0, 0, 8, 8)); }));                // duplicate
    CHECK(throws([&] { packer.add_sprite("e", Image{}); }));                                     // empty
    CHECK(throws([&] { packer.add_sprite("b", with_rect(8, 8, 0, 0, 8, 8), { .box = { 0, 0, 9, 8 } }); }));
    CHECK(throws([&] { packer.add_animation("w", with_rect(32, 32, 0, 0, 32, 32),
                                            { .frame_width = 16, .frames = 3 }); }));             // too wide
    CHECK(throws([&] { packer.add_animation("w", with_rect(32, 32, 0, 0, 32, 32),
                                            { .directions = 2, .headings = 3 }); }));             // bad mirroring
    CHECK(throws([&] { packer.add_animation("x", Render::Animation{ .first = 0, .frames = 2 }); })); // past the end
    CHECK(throws([&] { packer.pack(3); }));                                                       // not a power of two

    packer.add_sprite("big", with_rect(60, 60, 0, 0, 60, 60));
    CHECK(throws([&] { packer.pack(8); }));                                                       // doesn't fit
    CHECK(throws([&] { SpritePacker{}.pack(8); }));                                               // nothing
}

int main() {
    test_trim_and_pivot();
    test_animation_sheet();
    test_pages();
    test_errors();

    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("all passed\n");
    return EXIT_SUCCESS;
}
