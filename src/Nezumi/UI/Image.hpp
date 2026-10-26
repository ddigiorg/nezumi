// =============================================================================
// UI/Image.hpp
//
// A picture the UI can draw: one sprite from a Render::Atlas. A slot's frame,
// an item's icon, a button's glyph, a decoration. Small and copyable, so a
// game keeps its UI images in a table and hands them out every frame.
//
//   const UI::Image eagle = UI::Image::from(*atlas, { 0, 0, 192, 208 }, 2.0f);
//   ui.image(eagle);                                // at its natural size
//   ui.draw_image(rect, eagle.flipped_x());         // anywhere, any size
//
// `density` is source pixels per UI point: art painted at 2x for crisp edges
// on 150% and 200% screens has density 2, so a 192x208 px sprite is 96x104
// points. Its natural size is what image() lays out when no size is given.
//
// A NineSlice stretches an image to any size while keeping its corners and
// edges sharp: the corners are drawn as they are, the edges stretch along
// one axis, and the middle stretches both ways. For panel backplates, slot
// frames and buttons.
//
//   const UI::NineSlice plate = { UI::Image::from(*atlas, box, 2.0f), { 24, 24, 24, 24 } };
//   ui.draw_nine_slice(rect, plate);
//
// Images and text use different textures, so a panel that mixes them draws
// in a few runs instead of one (see Context.hpp, "Drawing order").
// =============================================================================

#pragma once
#include "Nezumi/Render/Atlas/Atlas.hpp"
#include "Nezumi/Render/Material/MaterialID.hpp"
#include <glm/glm.hpp> // vec2, vec4
#include <utility>     // swap

namespace Nezumi::UI {

struct Image {
    Render::MaterialID material = Render::Material::None; // None: no image
    glm::vec4          tex_box  = { 0.0f, 0.0f, 1.0f, 1.0f }; // atlas UVs { min_u, min_v, max_u, max_v }, v down
    glm::vec2          size     = { 0.0f, 0.0f };             // natural size, points

    // `pixel_box`: { x0, y0, x1, y1 } in the atlas image, y down
    static Image from(const Render::Atlas& atlas, glm::vec4 pixel_box, float density = 1.0f) {
        return {
            .material = atlas.material(),
            .tex_box  = atlas.tex_box(pixel_box),
            .size     = glm::vec2(pixel_box.z - pixel_box.x, pixel_box.w - pixel_box.y) / density,
        };
    }

    bool is_valid() const { return material != Render::Material::None; }
    explicit operator bool() const { return is_valid(); }

    // Mirrored left to right (or top to bottom): the same sprite facing the
    // other way, e.g. the right one of two flanking decorations
    Image flipped_x() const { Image i = *this; std::swap(i.tex_box.x, i.tex_box.z); return i; }
    Image flipped_y() const { Image i = *this; std::swap(i.tex_box.y, i.tex_box.w); return i; }

    // A sub-rectangle, in fractions of this image: { 0, 0, 1, 1 } is all of it
    Image part(glm::vec4 box) const {
        const glm::vec2 a = { tex_box.x, tex_box.y };
        const glm::vec2 d = glm::vec2(tex_box.z, tex_box.w) - a;
        return {
            .material = material,
            .tex_box  = { a.x + box.x * d.x, a.y + box.y * d.y, a.x + box.z * d.x, a.y + box.w * d.y },
            .size     = size * glm::vec2(box.z - box.x, box.w - box.y),
        };
    }
};

struct NineSlice {
    Image     image   = {};
    glm::vec4 margins = { 0.0f, 0.0f, 0.0f, 0.0f }; // points kept unstretched: left, top, right, bottom

    bool is_valid() const { return image.is_valid(); }
    explicit operator bool() const { return is_valid(); }
};

} // namespace Nezumi::UI
