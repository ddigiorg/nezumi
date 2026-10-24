// =============================================================================
// UI/Widgets.cpp
//
// The widgets. Each one places itself (place_), handles the mouse
// (behavior_ or hovered_), changes the caller's value, and draws.
// =============================================================================

#include "Nezumi/UI/Context.hpp"
#include "Nezumi/Render/Text/Font.hpp"
#include <algorithm> // clamp, max, min
#include <cmath>     // round
#include <cstdio>    // snprintf

namespace Nezumi::UI {

using Render::UIQuadShape;

// -----------------------------------------------------------------------------
// Output Widgets
// -----------------------------------------------------------------------------

void Context::text(std::string_view text) {
    this->text(text, style_.text);
}

void Context::text(std::string_view text, const glm::vec4& color) {
    if (!layout_.panel) return;

    // Lines are as tall as a button, so text lines up with widgets beside it
    const Rect r = place_({ text_width_(text), frame_height_() });
    draw_text_({ r.min.x, r.min.y + snap_((r.height() - text_height_) * 0.5f) },
               text, color);
}

void Context::heading(std::string_view text) {
    this->text(text, style_.heading);
}

// -----------------------------------------------------------------------------
// Button
// -----------------------------------------------------------------------------

bool Context::button(std::string_view label, float width) {
    if (!layout_.panel) return false;

    const ID               id      = id_(label);
    const std::string_view visible = visible_(label);

    // A fill button is at least as wide as its label, and that much counts
    // toward an AutoSize panel's width
    const float fit = snap_(text_width_(visible) + 2.0f * style_.frame_padding.x);
    const float w   = width > 0.0f ? width
                    : width < 0.0f ? std::max(fit, avail_width_())
                    : fit;
    const Rect r = place_({ snap_(w), frame_height_() }, width < 0.0f);
    if (width < 0.0f) layout_.max_x = std::max(layout_.max_x, r.min.x + fit);

    bool hovered = false, held = false;
    const bool clicked = behavior_(id, r, &hovered, &held);

    const glm::vec4& bg = held ? style_.button_active : hovered ? style_.button_hovered : style_.button;
    draw_box_(r, bg, style_.frame_rounding, style_.frame_border, style_.border_width);
    draw_text_in_(r, visible, style_.text, style_.frame_padding.x, true);

    return clicked;
}

// -----------------------------------------------------------------------------
// Checkbox
// -----------------------------------------------------------------------------

bool Context::checkbox(std::string_view label, bool& value) {
    if (!layout_.panel) return false;

    const ID               id      = id_(label);
    const std::string_view visible = visible_(label);

    const float h = frame_height_();
    const float s = snap_(h - 2.0f * style_.frame_padding.y + 2.0f); // box side
    const Rect  r = place_({ s + label_width_(visible), h });
    const Rect  box = Rect::from_size({ r.min.x, r.min.y + snap_((h - s) * 0.5f) }, glm::vec2(s));

    // The label is part of the target, as in any desktop toolkit
    bool hovered = false, held = false;
    const bool clicked = behavior_(id, r, &hovered, &held);
    if (clicked) value = !value;

    const glm::vec4& bg = held ? style_.frame_bg_active : hovered ? style_.frame_bg_hovered : style_.frame_bg;
    draw_box_(box, bg, style_.frame_rounding, style_.frame_border, style_.border_width);
    if (value) {
        draw_shape_(box.shrink(glm::vec2(snap_(s * 0.18f))), UIQuadShape::Check,
                    style_.accent, std::max(2.0f, s * 0.14f));
    }
    draw_label_(r, box.max.x, visible);

    return clicked;
}

// -----------------------------------------------------------------------------
// Toggle
// -----------------------------------------------------------------------------

bool Context::toggle(std::string_view label, bool& value) {
    if (!layout_.panel) return false;

    const ID               id      = id_(label);
    const std::string_view visible = visible_(label);

    const float h  = frame_height_();
    const float th = snap_(h - 2.0f * style_.frame_padding.y + 2.0f); // track height
    const float tw = 2.0f * th;
    const Rect  r  = place_({ tw + label_width_(visible), h });
    const Rect  track = Rect::from_size({ r.min.x, r.min.y + snap_((h - th) * 0.5f) }, { tw, th });

    bool hovered = false;
    const bool clicked = behavior_(id, r, &hovered);
    if (clicked) value = !value;

    glm::vec4 bg = value ? style_.accent_dim : style_.frame_bg;
    if (hovered && !value) bg = style_.frame_bg_hovered;
    draw_box_(track, bg, th * 0.5f, style_.frame_border, style_.border_width);

    const float     inset = 3.0f;
    const float     knob  = th - 2.0f * inset;
    const glm::vec2 at    = value ? glm::vec2(track.max.x - inset - knob, track.min.y + inset)
                                  : track.min + glm::vec2(inset);
    draw_shape_(Rect::from_size(at, glm::vec2(knob)), UIQuadShape::Circle,
                value ? style_.accent : style_.text_dim);

    draw_label_(r, track.max.x, visible);

    return clicked;
}

// -----------------------------------------------------------------------------
// Slider
// -----------------------------------------------------------------------------

bool Context::slider(std::string_view label, float& value, float min, float max,
                     const char* format)
{
    return slider_(label, value, min, max, false, format);
}

bool Context::slider(std::string_view label, int& value, int min, int max) {
    float v = static_cast<float>(value);
    slider_(label, v, static_cast<float>(min), static_cast<float>(max), true, "%.0f");

    const int rounded = static_cast<int>(std::round(v));
    if (rounded == value) return false;
    value = rounded;
    return true;
}

bool Context::slider_(std::string_view label, float& value, float min, float max,
                      bool integer, const char* format)
{
    if (!layout_.panel) return false;

    const ID               id      = id_(label);
    const std::string_view visible = visible_(label);

    const float w     = control_width_();
    const Rect  r     = place_({ w + label_width_(visible), frame_height_() });
    const Rect  track = { r.min, { r.min.x + w, r.max.y } };

    bool hovered = false, held = false;
    behavior_(id, track, &hovered, &held);

    // While held (including the press itself), the grab follows the cursor
    const float grab_w = std::max(10.0f, snap_(frame_height_() * 0.5f));
    const float range  = max - min;
    bool changed = false;
    if (active_id_ == id || held) {
        const float t = std::clamp((mouse_.x - track.min.x - grab_w * 0.5f) / (w - grab_w), 0.0f, 1.0f);
        float v = min + t * range;
        if (integer) v = std::round(v);
        if (v != value) {
            value   = v;
            changed = true;
        }
    }

    const float t = range != 0.0f ? std::clamp((value - min) / range, 0.0f, 1.0f) : 0.0f;
    const glm::vec4& bg = held ? style_.frame_bg_active : hovered ? style_.frame_bg_hovered : style_.frame_bg;
    draw_box_(track, bg, style_.frame_rounding, style_.frame_border, style_.border_width);

    const Rect grab = Rect::from_size(
        { track.min.x + snap_(t * (w - grab_w)), track.min.y + 2.0f },
        { grab_w, track.height() - 4.0f });
    draw_box_(grab, (held || hovered) ? style_.accent : style_.accent_dim, style_.frame_rounding);

    char buf[64];
    std::snprintf(buf, sizeof buf, format, static_cast<double>(value));
    draw_text_in_(track, buf, style_.text, 0.0f, true);

    draw_label_(r, track.max.x, visible);

    return changed;
}

// -----------------------------------------------------------------------------
// Text Field
// -----------------------------------------------------------------------------

bool Context::text_field(std::string_view label, std::string& value, std::size_t max_length) {
    if (!layout_.panel) return false;

    const ID               id      = id_(label);
    const std::string_view visible = visible_(label);

    const float w   = control_width_();
    const Rect  r   = place_({ w + label_width_(visible), frame_height_() });
    const Rect  box = { r.min, { r.min.x + w, r.max.y } };
    const Rect  inner = { { box.min.x + style_.frame_padding.x, box.min.y },
                          { box.max.x - style_.frame_padding.x, box.max.y } };

    const bool hovered = hovered_(id, box);
    if (hovered) ui_cursor_ = CursorStyle::Text;

    // Text x offset when focused (scrolled so the caret stays in view)
    const float scroll = focus_id_ == id ? field_scroll_ : 0.0f;

    // Click to focus (or move the caret); click anywhere else to unfocus
    if (mouse_pressed_) {
        if (hovered) {
            if (focus_id_ != id) field_scroll_ = 0.0f;
            focus_id_ = id;

            // Caret goes to the gap nearest the click
            const float x = mouse_.x - inner.min.x + scroll;
            float pen = 0.0f;
            caret_ = value.size();
            for (std::size_t i = 0; i < value.size(); i++) {
                const Render::FontGlyph* g = font_->glyph_or_fallback(value[i]);
                const float adv = g ? g->advance * text_scale_ : 0.0f;
                if (x < pen + adv * 0.5f) { caret_ = i; break; }
                pen += adv;
            }
        } else if (focus_id_ == id) {
            focus_id_ = 0;
        }
    }

    const bool focused = focus_id_ == id;
    bool changed = false;

    if (focused) {
        focus_seen_ = true;
        caret_ = std::min(caret_, value.size());

        for (char c : typed_) {
            if (value.size() >= max_length) break;
            value.insert(value.begin() + static_cast<std::ptrdiff_t>(caret_), c);
            caret_++;
            changed = true;
        }
        typed_.clear();

        for (ScanCode key : keys_) {
            switch (key) {
                case ScanCode::Backspace:
                    if (caret_ > 0) { value.erase(--caret_, 1); changed = true; }
                    break;
                case ScanCode::Delete:
                    if (caret_ < value.size()) { value.erase(caret_, 1); changed = true; }
                    break;
                case ScanCode::Left:  if (caret_ > 0) caret_--;            break;
                case ScanCode::Right: if (caret_ < value.size()) caret_++; break;
                case ScanCode::Home:  caret_ = 0;                          break;
                case ScanCode::End:   caret_ = value.size();               break;
                case ScanCode::Return:
                case ScanCode::Escape:
                    focus_id_ = 0;
                    break;
                default:
                    break;
            }
        }
        keys_.clear();
    }

    // Keep the caret in view
    const float caret_x = text_width_(std::string_view(value).substr(0, std::min(caret_, value.size())));
    if (focus_id_ == id) {
        if (caret_x - field_scroll_ > inner.width()) field_scroll_ = caret_x - inner.width();
        if (caret_x - field_scroll_ < 0.0f)          field_scroll_ = caret_x;
    }
    const float offset = focus_id_ == id ? field_scroll_ : 0.0f;

    const glm::vec4& bg     = hovered ? style_.frame_bg_hovered : style_.frame_bg;
    const glm::vec4& border = focus_id_ == id ? style_.accent : style_.frame_border;
    draw_box_(box, bg, style_.frame_rounding, border, style_.border_width);

    const Rect saved = clip_;
    clip_ = clip_.intersect(inner);
    const float y = box.min.y + snap_((box.height() - text_height_) * 0.5f);
    draw_text_({ inner.min.x - offset, y }, value, style_.text);
    if (focus_id_ == id) {
        draw_box_(Rect::from_size({ snap_(inner.min.x - offset + caret_x), y },
                                  { 1.5f, text_height_ }), style_.text);
    }
    clip_ = saved;

    draw_label_(r, box.max.x, visible);

    return changed;
}

// -----------------------------------------------------------------------------
// Dropdown
// -----------------------------------------------------------------------------

bool Context::dropdown(std::string_view label, int& selected,
                       std::span<const std::string_view> items)
{
    if (!layout_.panel) return false;

    const ID               id      = id_(label);
    const std::string_view visible = visible_(label);

    const float h   = frame_height_();
    const float w   = control_width_();
    const Rect  r   = place_({ w + label_width_(visible), h });
    const Rect  box = { r.min, { r.min.x + w, r.max.y } };

    bool hovered = false, held = false;
    if (behavior_(id, box, &hovered, &held))
        popup_id_ = popup_id_ == id ? 0 : id;
    const bool open = popup_id_ == id;

    // The box: current choice and an arrow
    const glm::vec4& bg = (held || open) ? style_.frame_bg_active
                        : hovered        ? style_.frame_bg_hovered
                        :                  style_.frame_bg;
    draw_box_(box, bg, style_.frame_rounding, style_.frame_border, style_.border_width);

    const float arrow = snap_(h * 0.3f);
    const Rect  text_box = { box.min, { box.max.x - arrow - 2.0f * style_.frame_padding.x, box.max.y } };
    if (selected >= 0 && static_cast<std::size_t>(selected) < items.size())
        draw_text_in_(text_box, items[static_cast<std::size_t>(selected)], style_.text, style_.frame_padding.x);
    draw_shape_(Rect::from_size({ box.max.x - style_.frame_padding.x - arrow,
                                  box.min.y + snap_((h - arrow * 0.6f) * 0.5f) },
                                { arrow, snap_(arrow * 0.6f) }),
                UIQuadShape::TriangleDown, style_.text_dim);

    draw_label_(r, box.max.x, visible);

    if (!open) return false;

    // The list: below the box, or above it if it would run off the screen.
    // Drawn into the overlay so it covers every panel, and not clipped.
    const float pad    = 3.0f;
    const float list_h = static_cast<float>(items.size()) * h + 2.0f * pad;
    Rect list = { { box.min.x, box.max.y + 2.0f }, { box.max.x, box.max.y + 2.0f + list_h } };
    if (list.max.y > screen_size_.y && box.min.y - 2.0f - list_h >= 0.0f)
        list = { { box.min.x, box.min.y - 2.0f - list_h }, { box.max.x, box.min.y - 2.0f } };

    // Fits neither way (a long list on a short screen): slide it on screen,
    // over its box if need be. Panels stay on screen, so x already fits.
    if (screen_size_.y > 0.0f) {
        const float y = std::clamp(list.min.y, 0.0f, std::max(0.0f, screen_size_.y - list_h));
        list = Rect::from_size({ list.min.x, y }, list.size());
    }

    popup_rect_  = list;
    popup_owner_ = box;
    popup_drawn_ = true;

    DrawList* saved_list = list_;
    const Rect saved_clip = clip_;
    list_ = &overlay_;
    clip_ = { { -1e9f, -1e9f }, { 1e9f, 1e9f } };

    // Opaque, so the panel underneath doesn't show through the choices
    glm::vec4 list_bg = style_.panel_bg;
    list_bg.a = 1.0f;
    draw_box_(list, list_bg, style_.frame_rounding, style_.frame_border, style_.border_width);

    bool changed = false;
    for (std::size_t i = 0; i < items.size(); i++) {
        const Rect item = Rect::from_size({ list.min.x + pad, list.min.y + pad + static_cast<float>(i) * h },
                                          { list.width() - 2.0f * pad, h });
        const bool item_hovered = on_popup_ && item.contains(mouse_);

        if (item_hovered) {
            ui_cursor_ = CursorStyle::Hand;
            draw_box_(item, style_.button_hovered, style_.frame_rounding);
        } else if (static_cast<int>(i) == selected) {
            draw_box_(item, style_.selection, style_.frame_rounding);
        }
        draw_text_in_(item, items[i], style_.text, style_.frame_padding.x - pad);

        if (item_hovered && mouse_pressed_) {
            changed   = static_cast<int>(i) != selected;
            selected  = static_cast<int>(i);
            popup_id_ = 0;
        }
    }

    list_ = saved_list;
    clip_ = saved_clip;
    return changed;
}

} // namespace Nezumi::UI
