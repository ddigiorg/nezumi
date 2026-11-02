// =============================================================================
// SpritePacker.cpp
// =============================================================================

#include "Nezumi/Render/2D/Sprite/SpritePacker.hpp"

#include <algorithm> // max, min, stable_sort
#include <cstring>   // memcpy
#include <numeric>   // iota
#include <stdexcept> // runtime_error
#include <utility>   // move

namespace Nezumi::Render {

namespace {

[[noreturn]] void fail(const std::string& what) {
    throw std::runtime_error("SpritePacker: " + what);
}

std::uint32_t round_up(std::uint32_t x, std::uint32_t a) {
    return (x + a - 1) / a * a;
}

// Pixels of `image` inside [x0, x1) x [y0, y1)
Image crop(const Image& image, std::uint32_t x0, std::uint32_t y0, std::uint32_t x1, std::uint32_t y1) {
    Image out = Image::blank(x1 - x0, y1 - y0);
    for (std::uint32_t y = y0; y < y1; ++y) {
        std::memcpy(out.at(0, y - y0), image.at(x0, y), std::size_t(out.width) * 4);
    }
    return out;
}

// Copies `src` onto `dst` with its top-left at (x, y)
void blit(Image& dst, const Image& src, std::uint32_t x, std::uint32_t y) {
    for (std::uint32_t row = 0; row < src.height; ++row) {
        std::memcpy(dst.at(x, y + row), src.at(0, row), std::size_t(src.width) * 4);
    }
}

// FNV-1a over the size and pixels
std::uint64_t hash_image(const Image& image) {
    std::uint64_t h = 14695981039346656037ull;
    const auto mix = [&](std::uint8_t b) { h = (h ^ b) * 1099511628211ull; };
    for (int i = 0; i < 4; ++i) mix(std::uint8_t(image.width  >> (8 * i)));
    for (int i = 0; i < 4; ++i) mix(std::uint8_t(image.height >> (8 * i)));
    for (std::uint8_t b : image.pixels) mix(b);
    return h;
}

} // namespace

// -----------------------------------------------------------------------------
// Adding
// -----------------------------------------------------------------------------

SpriteID SpritePacker::add_sprite(std::string name, const Image& image, const SpriteOptions& options) {
    if (image.empty()) fail("sprite '" + name + "' has an empty image");
    if (taken_sprites_.contains(name)) fail("duplicate sprite '" + name + "'");

    // The frame: the whole image, or options.box of it
    std::uint32_t x0 = 0, y0 = 0, x1 = image.width, y1 = image.height;
    if (options.box != glm::vec4(0.0f)) {
        if (options.box.x < 0.0f || options.box.y < 0.0f ||
            options.box.z > float(image.width) || options.box.w > float(image.height) ||
            options.box.z <= options.box.x || options.box.w <= options.box.y)
        {
            fail("sprite '" + name + "': box is empty or outside its " + std::to_string(image.width) +
                 "x" + std::to_string(image.height) + " image");
        }
        x0 = std::uint32_t(options.box.x); y0 = std::uint32_t(options.box.y);
        x1 = std::uint32_t(options.box.z); y1 = std::uint32_t(options.box.w);
    }
    const glm::vec2 frame_size = { float(x1 - x0), float(y1 - y0) };

    // Trim to the opaque pixels; a fully transparent frame keeps one pixel
    std::uint32_t tx0 = x0, ty0 = y0, tx1 = x1, ty1 = y1;
    if (options.trim) {
        tx0 = x1; ty0 = y1; tx1 = x0; ty1 = y0;
        for (std::uint32_t y = y0; y < y1; ++y) {
            for (std::uint32_t x = x0; x < x1; ++x) {
                if (image.at(x, y)[3] == 0) continue;
                tx0 = std::min(tx0, x);     ty0 = std::min(ty0, y);
                tx1 = std::max(tx1, x + 1); ty1 = std::max(ty1, y + 1);
            }
        }
        if (tx0 >= tx1) { tx0 = x0; ty0 = y0; tx1 = x0 + 1; ty1 = y0 + 1; }
    }

    Entry e;
    e.name   = name;
    e.slot   = slot_for_(crop(image, tx0, ty0, tx1, ty1));
    e.offset = { float(tx0 - x0), float(ty0 - y0) };
    e.pivot  = options.pivot ? *options.pivot : glm::vec2(frame_size.x * 0.5f, frame_size.y);

    const auto id = static_cast<SpriteID>(sprites_.size());
    sprites_.push_back(std::move(e));
    sprite_names_.push_back(name);
    taken_sprites_.insert(std::move(name));
    return id;
}

SpriteID SpritePacker::add_sprite(std::string name, const Image& image) {
    return add_sprite(std::move(name), image, SpriteOptions{});
}

AnimationID SpritePacker::add_animation(std::string name, const Image& sheet, const SheetOptions& options) {
    if (options.frames == 0 || options.directions == 0) fail("animation '" + name + "' has no frames");
    if (taken_animations_.contains(name)) fail("duplicate animation '" + name + "'");

    const std::uint32_t fw = options.frame_width  ? options.frame_width  : sheet.width  / options.frames;
    const std::uint32_t fh = options.frame_height ? options.frame_height : sheet.height / options.directions;
    if (fw == 0 || fh == 0 || fw * options.frames > sheet.width || fh * options.directions > sheet.height) {
        fail("animation '" + name + "': " + std::to_string(options.frames) + " frames x " +
             std::to_string(options.directions) + " directions of " + std::to_string(fw) + "x" +
             std::to_string(fh) + " px don't fit its " + std::to_string(sheet.width) + "x" +
             std::to_string(sheet.height) + " image");
    }

    Animation anim;
    anim.first      = static_cast<SpriteID>(sprites_.size());
    anim.frames     = options.frames;
    anim.directions = options.directions;
    anim.headings   = options.headings ? options.headings : options.directions;
    anim.fps        = options.fps;
    anim.loop       = options.loop;
    if (anim.mirrored() && anim.headings != 2 * (anim.directions - 1)) {
        fail("animation '" + name + "': mirrored headings must be 2 * (directions - 1)");
    }

    for (std::uint32_t d = 0; d < options.directions; ++d) {
        for (std::uint32_t f = 0; f < options.frames; ++f) {
            add_sprite(name + "/" + std::to_string(d) + "/" + std::to_string(f), sheet, {
                .box   = { float(f * fw), float(d * fh), float((f + 1) * fw), float((d + 1) * fh) },
                .pivot = options.pivot,
                .trim  = options.trim,
            });
        }
    }
    return add_animation(std::move(name), anim);
}

AnimationID SpritePacker::add_animation(std::string name, const Animation& animation) {
    if (taken_animations_.contains(name)) fail("duplicate animation '" + name + "'");
    if (animation.frames == 0 || animation.directions == 0) fail("animation '" + name + "' has no frames");
    if (animation.mirrored() && animation.headings != 2 * (animation.directions - 1)) {
        fail("animation '" + name + "': mirrored headings must be 2 * (directions - 1)");
    }
    const std::uint64_t last = std::uint64_t(animation.first) + std::uint64_t(animation.frames) * animation.directions;
    if (last > sprites_.size()) fail("animation '" + name + "' runs past the last sprite");

    const auto id = static_cast<AnimationID>(animations_.size());
    animations_.push_back(animation);
    animation_names_.push_back(name);
    taken_animations_.insert(std::move(name));
    return id;
}

std::uint32_t SpritePacker::slot_for_(Image image) {
    const std::uint64_t h = hash_image(image);
    const auto [begin, end] = slot_index_.equal_range(h);
    for (auto it = begin; it != end; ++it) {
        const Image& other = slots_[it->second].image;
        if (other.width == image.width && other.height == image.height && other.pixels == image.pixels) {
            return it->second;
        }
    }
    const auto i = static_cast<std::uint32_t>(slots_.size());
    slots_.push_back({ std::move(image) });
    slot_index_.emplace(h, i);
    return i;
}

// -----------------------------------------------------------------------------
// Packing
// -----------------------------------------------------------------------------

SpritePacker::Packed SpritePacker::pack(std::uint32_t grid) const {
    if (grid == 0 || (grid & (grid - 1)) != 0) fail("grid must be a power of two");
    if (slots_.empty()) fail("nothing to pack");
    const std::uint32_t size = config_.page_size;

    // Shelf packing, tallest first (ties: widest, then first added). A slot's
    // cell is its size rounded up to the grid, plus one grid of gap; the
    // first row and column start one grid in.
    std::vector<std::uint32_t> order(slots_.size());
    std::iota(order.begin(), order.end(), 0u);
    std::stable_sort(order.begin(), order.end(), [&](std::uint32_t a, std::uint32_t b) {
        const Image& ia = slots_[a].image;
        const Image& ib = slots_[b].image;
        return ia.height != ib.height ? ia.height > ib.height : ia.width > ib.width;
    });

    struct Place { std::uint32_t page = 0, x = 0, y = 0; };
    std::vector<Place> places(slots_.size());
    std::uint32_t pages = 0, x = grid, y = grid, shelf = 0;
    std::uint32_t used_w = 0, used_h = 0; // extent on the last page, gaps included

    for (std::uint32_t i : order) {
        const Image& img = slots_[i].image;
        const std::uint32_t w = round_up(img.width, grid);
        const std::uint32_t h = round_up(img.height, grid);
        if (w + 2 * grid > size || h + 2 * grid > size) {
            fail("a " + std::to_string(img.width) + "x" + std::to_string(img.height) +
                 " px sprite doesn't fit a " + std::to_string(size) + " px page");
        }
        if (pages > 0 && x + w + grid > size) {
            x = grid; y += shelf + grid; shelf = 0;
        }
        if (pages == 0 || y + h + grid > size) {
            ++pages; x = grid; y = grid; shelf = 0; used_w = 0; used_h = 0;
        }
        places[i] = { pages - 1, x, y };
        used_w = std::max(used_w, x + w + grid);
        used_h = std::max(used_h, y + h + grid);
        x += w + grid;
        shelf = std::max(shelf, h);
    }

    // One page: only as big as it needs (still on the grid, so every mip
    // level halves evenly). More: every page full size.
    const std::uint32_t page_w = (pages == 1) ? used_w : size;
    const std::uint32_t page_h = (pages == 1) ? used_h : size;

    Packed out;
    out.pages.resize(pages);
    for (Image& p : out.pages) p = Image::blank(page_w, page_h);
    for (std::uint32_t i = 0; i < slots_.size(); ++i) {
        blit(out.pages[places[i].page], slots_[i].image, places[i].x, places[i].y);
    }

    const glm::vec4 page_size = { float(page_w), float(page_h), float(page_w), float(page_h) };
    out.regions.reserve(sprites_.size());
    for (const Entry& e : sprites_) {
        const Image& img = slots_[e.slot].image;
        const Place& at  = places[e.slot];
        SpriteRegion r;
        r.tex_box = glm::vec4(float(at.x), float(at.y), float(at.x + img.width), float(at.y + img.height)) / page_size;
        r.size    = { float(img.width), float(img.height) };
        r.offset  = e.offset;
        r.pivot   = e.pivot;
        r.layer   = at.page;
        out.regions.push_back(r);
    }
    out.sprite_names    = sprite_names_;
    out.animations      = animations_;
    out.animation_names = animation_names_;
    return out;
}

} // namespace Nezumi::Render
