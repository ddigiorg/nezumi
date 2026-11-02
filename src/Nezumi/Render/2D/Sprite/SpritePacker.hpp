// =============================================================================
// SpritePacker.hpp
//
// Packs loose images into SpriteSheet pages at load time, so a game can ship
// its art as plain PNGs (one per sprite, or one per animation) and build its
// sheets when it starts, with no packing tool or generated files.
//
//   SpritePacker packer;
//   packer.add_animation("knight/walk", load_image("data/knight/walk.png"), {
//       .frame_width = 64, .frame_height = 128,
//       .frames = 8, .directions = 5, .headings = 8, .fps = 12.0f,
//       .pivot = glm::vec2{ 32.0f, 96.0f } });
//   packer.add_sprite("rock", load_image("data/rock.png"));
//
//   SpriteSheet& sheet = *renderer.create_sprite_sheet(
//       { .name = "sprites", .mip_levels = 4 }, packer).object;
//   const AnimationID walk = sheet.animation("knight/walk");
//
// ## Animation sheets
//
// One image per animation: a grid of frame_width x frame_height cells, one
// column per frame and one row per stored direction, top to bottom:
//
//          frame 0   frame 1   frame 2  ...
//   dir 0  [     ]   [     ]   [     ]
//   dir 1  [     ]   [     ]   [     ]
//   ...
//
// Each cell becomes the sprite "<name>/<direction>/<frame>", direction-major
// (see Animation.hpp), so headings mirror as usual: 8 headings store the 5
// rows down, down-right, right, up-right, up.
//
// ## Packing
//
// Each sprite is trimmed to its opaque pixels (its region keeps the trimmed
// box's offset in the frame, so the pivot stays put), identical trimmed
// pixels are stored once, and the results are shelf-packed, tallest first,
// onto pages of at most page_size. Every sprite sits on a `grid` px grid
// with at least `grid` px of transparent space around it: the sheet passes
// 2^(mip_levels - 1), so no mip level bleeds one sprite into another (see
// SpriteSheetConfig.hpp). A sheet that fits one page gets a page only as
// large as it needs.
//
// UI pieces measured by their full image (nine-slices, icons) pass
// trim = false.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Sprite/Animation.hpp"
#include "Nezumi/Render/2D/Sprite/SpriteID.hpp"
#include "Nezumi/Render/2D/Sprite/SpriteRegion.hpp"
#include "Nezumi/Render/Core/Image/Image.hpp"
#include <cstddef>       // size_t
#include <cstdint>       // uint32_t, uint64_t
#include <glm/glm.hpp>   // vec2, vec4
#include <optional>      // optional
#include <string>        // string
#include <unordered_map> // unordered_multimap
#include <unordered_set> // unordered_set
#include <vector>        // vector

namespace Nezumi::Render {

class SpritePacker {
public:

    // -------------------------------------------------------------------------
    // Options
    // -------------------------------------------------------------------------

    struct Config {
        std::uint32_t page_size = 2048; // largest page, px (square)
    };

    struct SpriteOptions {
        // Pixels of the image to take { x0, y0, x1, y1 }, y down; all zero
        // takes the whole image
        glm::vec4                 box   = {};

        // Anchor point in the box, px, y down. Default: bottom center.
        std::optional<glm::vec2>  pivot = {};

        // Cut away transparent borders (see the header comment)
        bool                      trim  = true;
    };

    struct SheetOptions {
        std::uint32_t            frame_width  = 0; // cell size, px; 0 = the image's
        std::uint32_t            frame_height = 0; // width / frames, height / directions
        std::uint32_t            frames       = 1; // columns
        std::uint32_t            directions   = 1; // rows
        std::uint32_t            headings     = 0; // drawn; 0 = directions (no mirroring)
        float                    fps          = 0.0f;
        bool                     loop         = true;
        std::optional<glm::vec2> pivot        = {}; // in a cell, px, y down; default bottom center
        bool                     trim         = true;
    };

    // -------------------------------------------------------------------------
    // Result
    //
    // Pages, all the same size, and tables in the order things were added
    // -------------------------------------------------------------------------

    struct Packed {
        std::vector<Image>        pages;
        std::vector<SpriteRegion> regions;
        std::vector<std::string>  sprite_names;
        std::vector<Animation>    animations;
        std::vector<std::string>  animation_names;
    };

    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    SpritePacker() = default;
    explicit SpritePacker(const Config& config) : config_(config) {}

    // -------------------------------------------------------------------------
    // Adding (each throws on a duplicate name or a box outside the image)
    // -------------------------------------------------------------------------

    // One sprite. The pixels are copied: the image needn't outlive the call.
    SpriteID add_sprite(std::string name, const Image& image, const SpriteOptions& options);
    SpriteID add_sprite(std::string name, const Image& image);

    // An animation sheet (see the header comment)
    AnimationID add_animation(std::string name, const Image& sheet, const SheetOptions& options);

    // An animation over sprites already added: animation.first and the
    // frames x directions after it
    AnimationID add_animation(std::string name, const Animation& animation);

    // -------------------------------------------------------------------------
    // Packing
    // -------------------------------------------------------------------------

    // Packs everything added so far on a `grid` px grid (a power of two).
    // Throws if a sprite doesn't fit a page.
    Packed pack(std::uint32_t grid) const;

    const Config& config()          const { return config_; }
    std::size_t   sprite_count()    const { return sprites_.size(); }
    std::size_t   animation_count() const { return animations_.size(); }
    std::size_t   unique_count()    const { return slots_.size(); } // after dedupe

private:

    // Trimmed pixels, stored once however many sprites use them
    struct Slot {
        Image image;
    };

    struct Entry {
        std::string   name;
        std::uint32_t slot   = 0;
        glm::vec2     offset = {}; // trimmed box's top-left in the frame
        glm::vec2     pivot  = {};
    };

    std::uint32_t slot_for_(Image image);

    Config                   config_;
    std::vector<Slot>        slots_;
    std::vector<Entry>       sprites_;
    std::vector<std::string> sprite_names_;
    std::vector<Animation>   animations_;
    std::vector<std::string> animation_names_;

    std::unordered_set<std::string>                       taken_sprites_;
    std::unordered_set<std::string>                       taken_animations_;
    std::unordered_multimap<std::uint64_t, std::uint32_t> slot_index_; // pixel hash -> slot
};

} // namespace Nezumi::Render
