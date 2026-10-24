// =============================================================================
// UI/Context.cpp
//
// Frame, panels, layout, interaction and drawing helpers. The widgets
// themselves are in Widgets.cpp.
// =============================================================================

#include "Nezumi/UI/Context.hpp"
#include "Nezumi/Core/Window.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonUp.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Input/Events/OnMouseWheel.hpp"
#include "Nezumi/Input/Events/OnTextInput.hpp"
#include "Nezumi/Render/Renderer.hpp"
#include <algorithm> // clamp, find_if, max, min, rotate
#include <cmath>     // floor, round
#include <cassert>   // assert

namespace Nezumi::UI {

using Render::UIQuad;
using Render::UIQuadShape;

// Seed for panel ids (FNV-1a offset basis), so a panel and a widget with the
// same label in no panel can't collide by construction
static constexpr Context::ID PANEL_SEED = 2166136261u;

// Clip box that clips nothing
static constexpr Rect NO_CLIP = { { -1e9f, -1e9f }, { 1e9f, 1e9f } };

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Context::Context(
    EventBus&         events,
    Render::Renderer& renderer,
    Window*           window,
    const UIConfig&   config)
    : events_  (events)
    , renderer_(renderer)
    , window_  (window)
    , config_  (config)
    , style_   (config.style)
{
    if (!config_.font_path.empty()) {
        font_ = renderer_.create_font(
            Render::FontConfig{
                .name       = "ui-font",
                .filepath   = config_.font_path,
                .pixel_size = config_.font_pixel_size,
            }
        ).object;
    }

    subscribe_();
}

Context::~Context() {
    for (EventListenerID id : listeners_)
        events_.unsubscribe(id);

    if (window_) window_->set_text_input(false);

    // font_ belongs to the renderer, which frees it with the rest of its fonts
}

// -----------------------------------------------------------------------------
// Frame
// -----------------------------------------------------------------------------

void Context::begin_frame() {
    assert(!in_frame_ && "UI: begin_frame called twice");
    in_frame_ = true;
    frame_++;

    // Screen size from the view's camera (sized to the backbuffer by the
    // renderer), and how window points map to it (they differ on HiDPI)
    if (const Render::Camera* camera = renderer_.get_view_camera(config_.view))
        screen_size_ = camera->get_size();
    if (window_ && window_->get_width() > 0 && window_->get_height() > 0) {
        mouse_scale_ = screen_size_ / glm::vec2(float(window_->get_width()),
                                                float(window_->get_height()));
    }

    if (font_) {
        text_scale_  = style_.font_size / font_->pixel_size();
        text_height_ = (font_->ascender() - font_->descender()) * text_scale_;
    }

    // An open dropdown sits above every panel. A press anywhere but on it or
    // on its own box closes it (its box toggles it in dropdown()).
    on_popup_ = popup_id_ != 0 && popup_rect_.contains(mouse_);
    if (popup_id_ && mouse_pressed_ && !on_popup_ && !popup_owner_.contains(mouse_))
        popup_id_ = 0;

    // The panel under the cursor gets this frame's hover and clicks. A
    // panel being dragged keeps them even when the cursor outruns it.
    hot_panel_ = 0;
    if (drag_panel_) {
        hot_panel_ = drag_panel_;
    } else if (!on_popup_) {
        if (Panel* panel = panel_at_(mouse_)) {
            hot_panel_ = panel->id;
            if (mouse_pressed_) bring_to_front_(*panel);
        }
    }

    ui_cursor_   = CursorStyle::Arrow;
    focus_seen_  = false;
    popup_drawn_ = false;
    overlay_.clear();
}

void Context::end_frame() {
    assert(in_frame_ && "UI: end_frame without begin_frame");
    assert(!layout_.panel && "UI: begin_panel without end_panel");

    // Forget a dropdown or text field whose panel wasn't drawn this frame
    if (!popup_drawn_)               popup_id_ = 0;
    if (focus_id_ && !focus_seen_)   focus_id_ = 0;

    // A release ends any hold, even on a widget that wasn't drawn this frame
    if (mouse_released_ && !mouse_down_) {
        active_id_  = 0;
        drag_panel_ = 0;
    }

    // Back to front, then the open dropdown on top. Same layer, order, kind
    // and material for all of it, so it stays in this order and is a single
    // instanced draw (see UIQuad.hpp).
    if (font_) {
        for (const auto& panel : panels_) {
            if (panel->drawn != frame_) continue;
            submit_(panel->chrome);
            submit_(panel->body);
        }
        submit_(overlay_);
    }

    if (window_) {
        window_->set_cursor(wants_mouse() ? ui_cursor_ : game_cursor_);
        window_->set_text_input(focus_id_ != 0);
    }

    // This frame's input is used up
    mouse_pressed_  = false;
    mouse_released_ = false;
    wheel_          = 0.0f;
    typed_.clear();
    keys_.clear();

    in_frame_ = false;
}

// -----------------------------------------------------------------------------
// Panels
// -----------------------------------------------------------------------------

bool Context::begin_panel(std::string_view title, bool* open, const PanelConfig& config) {
    assert(in_frame_ && "UI: begin_panel outside a frame (call it from on_draw)");
    assert(!layout_.panel && "UI: panels can't nest; call end_panel first");

    if (!font_)         return false;
    if (open && !*open) return false;

    const ID id    = hash_(title, PANEL_SEED);
    Panel*   panel = find_panel_(id);

    // First appearance: create it in front of everything
    if (!panel) {
        panels_.push_back(std::make_unique<Panel>());
        panel        = panels_.back().get();
        panel->id    = id;
        panel->title = std::string(visible_(title));
        panel->size  = config.size;
        panel->place = true;
        front_panel_ = id;
    }
    // Reappearing after being closed: in front again
    else if (panel->drawn + 1 < frame_) {
        bring_to_front_(*panel);
    }

    Panel& p   = *panel;
    p.flags    = config.flags;
    p.min_size = config.min_size;
    p.anchor   = config.position;
    p.pivot    = config.pivot;
    p.closable = open != nullptr && !p.has(PanelFlags::NoTitleBar);
    p.drawn    = frame_;
    p.chrome.clear();
    p.body.clear();

    if (p.has(PanelFlags::PinPosition)) p.place = true;
    if (p.place) p.pos = p.anchor - p.pivot * p.size;

    // Move and resize. Hover alone sets the cursor; a press starts a drag.
    const bool can_move = p.has(PanelFlags::Movable) &&
                          !p.has(PanelFlags::NoTitleBar) &&
                          !p.has(PanelFlags::PinPosition);
    if (drag_panel_ == id) {
        update_panel_drag_(p);
    } else if (hot_panel_ == id && active_id_ == 0) {
        const std::uint8_t edges = edges_at_(p, mouse_);
        const Rect title_bar = Rect::from_size(p.pos, { p.size.x, title_height_(p) });
        const bool on_close  = p.closable && close_rect_(p).contains(mouse_);

        if (edges != EdgeNone || (can_move && title_bar.contains(mouse_) && !on_close)) {
            if (edges != EdgeNone) update_panel_drag_(p); // sets the resize cursor
            if (mouse_pressed_) {
                drag_panel_ = id;
                drag_edges_ = edges;
                drag_mouse_ = mouse_;
                drag_pos_   = p.pos;
                drag_size_  = p.size;
                active_id_  = id;
            }
        }
    }

    clamp_to_screen_(p);

    // Layout: content inside the padding and below the title, scrolled.
    // AutoSize panels grow to fit, so they never clip or scroll.
    const float title_h = title_height_(p);
    const Rect  inner   = Rect{ { p.pos.x, p.pos.y + title_h }, p.pos + p.size }
                              .shrink(glm::vec2(style_.border_width));
    const Rect  content = Rect{ { p.pos.x, p.pos.y + title_h }, p.pos + p.size }
                              .shrink(style_.panel_padding);
    const bool  auto_size = p.has(PanelFlags::AutoSize);

    if (!auto_size) {
        if (hot_panel_ == id && !on_popup_ && wheel_ != 0.0f) {
            p.scroll -= wheel_ * style_.scroll_speed;
            wheel_ = 0.0f; // one panel scrolls per notch
        }
        const float max_scroll = std::max(0.0f, p.content.y - content.height());
        p.scroll = std::clamp(p.scroll, 0.0f, max_scroll);
    } else {
        p.scroll = 0.0f;
    }

    layout_          = {};
    layout_.panel    = &p;
    layout_.content  = content;
    layout_.clip     = auto_size ? NO_CLIP : inner;
    layout_.origin   = content.min - glm::vec2(0.0f, p.scroll);
    layout_.cursor   = layout_.origin;
    layout_.max_x    = layout_.origin.x;
    layout_.max_y    = layout_.origin.y;

    // Close button: in the title bar, so outside the content clip
    p.close_hot = false;
    if (p.closable) {
        list_ = &p.chrome;
        clip_ = p.rect();
        bool hovered = false;
        if (behavior_(hash_("#close", id), close_rect_(p), &hovered)) *open = false;
        p.close_hot = hovered;
    }

    list_ = &p.body;
    clip_ = layout_.clip;
    return true;
}

void Context::end_panel() {
    assert(layout_.panel && "UI: end_panel without begin_panel");
    Panel& p = *layout_.panel;

    // Measure the content for scrolling and AutoSize
    p.content = { layout_.max_x - layout_.origin.x, layout_.max_y - layout_.origin.y };

    if (p.has(PanelFlags::AutoSize)) {
        const glm::vec2 old_pos = p.pos;
        const float title_w = p.has(PanelFlags::NoTitleBar) ? 0.0f
            : text_width_(p.title) + 2.0f * style_.panel_padding.x +
              (p.closable ? title_height_(p) : 0.0f);

        p.size = p.content + 2.0f * style_.panel_padding + glm::vec2(0.0f, title_height_(p));
        p.size.x = std::max(p.size.x, title_w);
        p.size   = glm::max(p.size, p.min_size);

        // A centered (pivoted) panel can only be placed once its size is
        // known. Shift what was already laid out by however far it moved.
        if (p.place) {
            p.pos = p.anchor - p.pivot * p.size;
            clamp_to_screen_(p);
        }
        const glm::vec2 d = p.pos - old_pos;
        if (d != glm::vec2(0.0f)) {
            for (UIQuad& q : p.body) {
                q.pos_box += glm::vec4(d, d);
                if (q.clip_box.x > NO_CLIP.min.x) q.clip_box += glm::vec4(d, d);
            }
        }
    }

    if (!p.has(PanelFlags::PinPosition)) p.place = false;

    draw_chrome_(p);

    layout_ = {};
    list_   = nullptr;
}

Rect Context::get_panel_rect(std::string_view title) const {
    const Panel* panel = find_panel_(hash_(title, PANEL_SEED));
    return panel ? panel->rect() : Rect{};
}

// -----------------------------------------------------------------------------
// Layout
// -----------------------------------------------------------------------------

void Context::same_line(float spacing) {
    layout_.same_line  = true;
    layout_.same_space = spacing < 0.0f ? style_.item_spacing.x : spacing;
}

void Context::spacing(float height) {
    if (!layout_.panel) return;
    layout_.cursor.y += height < 0.0f ? style_.item_spacing.y : height;
    layout_.max_y     = std::max(layout_.max_y, layout_.cursor.y);
}

void Context::separator() {
    if (!layout_.panel) return;
    const Rect r = place_({ avail_width_(), style_.item_spacing.y }, true);
    const float y = std::floor(r.center().y);
    draw_box_({ { r.min.x, y }, { r.max.x, y + 1.0f } }, style_.separator);
}

// -----------------------------------------------------------------------------
// Input Capture
// -----------------------------------------------------------------------------

bool Context::wants_mouse() const {
    if (!font_) return false;
    if (active_id_ || drag_panel_) return true;
    if (popup_id_ && popup_rect_.contains(mouse_)) return true;
    return panel_at_(mouse_) != nullptr;
}

// -----------------------------------------------------------------------------
// Helpers: ids and panels
// -----------------------------------------------------------------------------

Context::ID Context::hash_(std::string_view text, ID seed) {
    // FNV-1a, 32-bit
    ID h = seed;
    for (char c : text) {
        h ^= static_cast<unsigned char>(c);
        h *= 16777619u;
    }
    return h ? h : 1u; // 0 means "none"
}

std::string_view Context::visible_(std::string_view label) {
    const std::size_t hidden = label.find("##");
    return hidden == std::string_view::npos ? label : label.substr(0, hidden);
}

Context::ID Context::id_(std::string_view label) const {
    return hash_(label, layout_.panel ? layout_.panel->id : PANEL_SEED + 1u);
}

Context::Panel* Context::find_panel_(ID id) const {
    const auto it = std::find_if(panels_.begin(), panels_.end(),
        [id](const auto& panel) { return panel->id == id; });
    return it == panels_.end() ? nullptr : it->get();
}

Context::Panel* Context::panel_at_(glm::vec2 p) const {
    // Front to back. "Drawn" means this frame or the last one, so this works
    // both before begin_frame (from on_tick or an event) and during it.
    for (auto it = panels_.rbegin(); it != panels_.rend(); ++it) {
        const Panel& panel = **it;
        if (panel.drawn + 1 < frame_ || panel.drawn == 0) continue;
        if (hit_rect_(panel).contains(p)) return it->get();
    }
    return nullptr;
}

Rect Context::hit_rect_(const Panel& panel) const {
    const bool resizable = panel.has(PanelFlags::Resizable) && !panel.has(PanelFlags::AutoSize);
    return resizable ? panel.rect().expand(style_.resize_margin) : panel.rect();
}

std::uint8_t Context::edges_at_(const Panel& panel, glm::vec2 p) const {
    if (!panel.has(PanelFlags::Resizable) || panel.has(PanelFlags::AutoSize)) return EdgeNone;

    const Rect  r = panel.rect();
    const float m = style_.resize_margin;
    if (!r.expand(m).contains(p)) return EdgeNone;

    std::uint8_t edges = EdgeNone;
    if (p.x <  r.min.x + m) edges |= EdgeLeft;
    if (p.x >= r.max.x - m) edges |= EdgeRight;
    if (p.y <  r.min.y + m) edges |= EdgeTop;
    if (p.y >= r.max.y - m) edges |= EdgeBottom;
    return edges;
}

void Context::bring_to_front_(Panel& panel) {
    front_panel_ = panel.id;
    const auto it = std::find_if(panels_.begin(), panels_.end(),
        [&](const auto& p) { return p.get() == &panel; });
    if (it != panels_.end()) std::rotate(it, it + 1, panels_.end());
}

void Context::update_panel_drag_(Panel& panel) {
    const std::uint8_t edges = drag_panel_ == panel.id ? drag_edges_ : edges_at_(panel, mouse_);

    // Cursor for the edge (hovered or dragged)
    const bool l = edges & EdgeLeft, r = edges & EdgeRight;
    const bool t = edges & EdgeTop,  b = edges & EdgeBottom;
    if      ((l && t) || (r && b)) ui_cursor_ = CursorStyle::ResizeNWSE;
    else if ((r && t) || (l && b)) ui_cursor_ = CursorStyle::ResizeNESW;
    else if (l || r)               ui_cursor_ = CursorStyle::ResizeEW;
    else if (t || b)               ui_cursor_ = CursorStyle::ResizeNS;
    else if (drag_panel_ == panel.id) ui_cursor_ = CursorStyle::Move;

    if (drag_panel_ != panel.id) return;

    // Released: the drag is over (the panel stays where it was dropped)
    if (!mouse_down_) {
        drag_panel_ = 0;
        active_id_  = 0;
        return;
    }

    const glm::vec2 d = mouse_ - drag_mouse_;
    if (edges == EdgeNone) {
        panel.pos = drag_pos_ + d;
        return;
    }

    // Resize: move the dragged edges, keep the others still
    glm::vec2 lo = drag_pos_;
    glm::vec2 hi = drag_pos_ + drag_size_;
    const glm::vec2 min = panel.min_size;
    if (l) lo.x = std::min(lo.x + d.x, hi.x - min.x);
    if (r) hi.x = std::max(hi.x + d.x, lo.x + min.x);
    if (t) lo.y = std::min(lo.y + d.y, hi.y - min.y);
    if (b) hi.y = std::max(hi.y + d.y, lo.y + min.y);
    panel.pos  = lo;
    panel.size = hi - lo;
}

void Context::clamp_to_screen_(Panel& panel) const {
    if (screen_size_.x <= 0.0f || screen_size_.y <= 0.0f) return;

    // Keep a grabbable piece of the title bar on screen, so a panel can't
    // be lost off an edge (or above the top, where the title would hide)
    const float keep = std::min(48.0f, panel.size.x);
    panel.pos.x = std::clamp(panel.pos.x, keep - panel.size.x, screen_size_.x - keep);
    panel.pos.y = std::clamp(panel.pos.y, 0.0f,
                             std::max(0.0f, screen_size_.y - std::max(title_height_(panel), 24.0f)));
}

float Context::title_height_(const Panel& panel) const {
    if (panel.has(PanelFlags::NoTitleBar)) return 0.0f;
    return std::round(text_height_ + 2.0f * style_.title_padding);
}

Rect Context::close_rect_(const Panel& panel) const {
    const float h = title_height_(panel);
    const float s = std::round(h * 0.7f);
    const glm::vec2 min = { panel.pos.x + panel.size.x - (h + s) * 0.5f,
                            panel.pos.y + (h - s) * 0.5f };
    return Rect::from_size(min, glm::vec2(s));
}

void Context::draw_chrome_(Panel& p) {
    list_ = &p.chrome;
    clip_ = NO_CLIP;

    const Rect  r       = p.rect();
    const float title_h = title_height_(p);
    const float bw      = style_.border_width;
    const float radius  = style_.panel_rounding;

    // Hard drop shadow, two steps
    draw_box_(Rect{ r.min + glm::vec2(0.0f, 2.0f), r.max + glm::vec2(2.0f, 4.0f) },
              { 0.0f, 0.0f, 0.0f, 0.18f }, radius + 2.0f);
    draw_box_(Rect{ r.min + glm::vec2(0.0f, 1.0f), r.max + glm::vec2(1.0f, 2.0f) },
              { 0.0f, 0.0f, 0.0f, 0.25f }, radius + 1.0f);

    // Background and border
    draw_box_(r, style_.panel_bg, radius, style_.panel_border, bw);

    // Title bar: rounded on top, square where it meets the content
    if (title_h > 0.0f) {
        const glm::vec4& bg = p.id == front_panel_ ? style_.title_bg_front : style_.title_bg;
        const Rect bar = Rect{ r.min, { r.max.x, r.min.y + title_h } }.shrink(glm::vec2(bw));
        draw_box_(bar, bg, radius - bw);
        draw_box_({ { bar.min.x, bar.center().y }, bar.max }, bg);
        draw_box_({ { r.min.x, bar.max.y }, { r.max.x, bar.max.y + bw } }, style_.panel_border);

        clip_ = bar;
        draw_text_({ r.min.x + style_.panel_padding.x, r.min.y + style_.title_padding },
                   p.title, style_.heading);
        clip_ = NO_CLIP;

        if (p.closable) {
            const Rect cr = close_rect_(p);
            if (p.close_hot) draw_box_(cr, style_.close_hovered, style_.frame_rounding);
            draw_shape_(cr.shrink(glm::vec2(std::round(cr.width() * 0.28f))),
                        UIQuadShape::Cross, style_.text, 2.0f);
        }
    }

    // Scrollbar: a thumb showing which part of the content is in view
    const float view_h = p.size.y - title_h - 2.0f * style_.panel_padding.y;
    if (!p.has(PanelFlags::AutoSize) && p.content.y > view_h && view_h > 0.0f) {
        const float track_top = r.min.y + title_h + style_.panel_padding.y;
        const float thumb_h   = std::max(16.0f, view_h * view_h / p.content.y);
        const float max_off   = p.content.y - view_h;
        const float thumb_y   = track_top + (view_h - thumb_h) * (p.scroll / max_off);
        const float x         = r.max.x - bw - 2.0f - style_.scrollbar_width;
        draw_box_(Rect::from_size({ x, thumb_y }, { style_.scrollbar_width, thumb_h }),
                  style_.scrollbar, style_.scrollbar_width * 0.5f);
    }

    // Resize grip in the bottom-right corner
    if (p.has(PanelFlags::Resizable) && !p.has(PanelFlags::AutoSize)) {
        const float g = 10.0f;
        draw_shape_(Rect{ r.max - glm::vec2(g + 3.0f), r.max - glm::vec2(3.0f) },
                    UIQuadShape::Corner, style_.separator);
    }
}

// -----------------------------------------------------------------------------
// Helpers: layout and interaction
// -----------------------------------------------------------------------------

Rect Context::place_(glm::vec2 size, bool fill) {
    Layout& L = layout_;

    glm::vec2 pos;
    if (L.same_line) {
        pos       = { L.prev_end.x + L.same_space, L.prev_end.y };
        L.line_h  = std::max(L.line_h, size.y);
    } else {
        pos       = L.cursor;
        L.line_h  = size.y;
    }
    L.same_line = false;

    // The next widget starts below the tallest widget on this line
    L.prev_end = { pos.x + size.x, pos.y };
    L.cursor   = { L.origin.x, pos.y + L.line_h + style_.item_spacing.y };

    if (!fill) L.max_x = std::max(L.max_x, pos.x + size.x);
    L.max_y = std::max(L.max_y, pos.y + L.line_h);

    return Rect::from_size(pos, size);
}

float Context::frame_height_() const {
    return std::round(text_height_ + 2.0f * style_.frame_padding.y);
}

float Context::control_width_() const {
    const Panel* p = layout_.panel;
    if (!p || p->has(PanelFlags::AutoSize)) return style_.control_width;
    return std::max(60.0f, std::round(layout_.content.width() * style_.control_fraction));
}

float Context::avail_width_() const {
    const Panel* p = layout_.panel;
    if (!p) return 0.0f;

    // An AutoSize panel is as wide as its widest widget last frame (or its
    // minimum size)
    const float right = p->has(PanelFlags::AutoSize)
        ? layout_.origin.x + std::max(p->content.x, p->size.x - 2.0f * style_.panel_padding.x)
        : layout_.content.max.x;
    const float x = layout_.same_line ? layout_.prev_end.x + layout_.same_space
                                      : layout_.cursor.x;
    return std::max(0.0f, right - x);
}

float Context::label_width_(std::string_view visible) const {
    return visible.empty() ? 0.0f : style_.item_spacing.x + text_width_(visible);
}

void Context::draw_label_(const Rect& row, float x, std::string_view visible) {
    if (visible.empty()) return;
    draw_text_({ x + style_.item_spacing.x, row.min.y + (row.height() - text_height_) * 0.5f },
               visible, style_.text);
}

bool Context::hovered_(ID id, const Rect& r) const {
    if (!layout_.panel || hot_panel_ != layout_.panel->id) return false;
    if (on_popup_) return false;
    if (active_id_ && active_id_ != id) return false;
    return r.contains(mouse_) && clip_.contains(mouse_);
}

bool Context::behavior_(ID id, const Rect& r, bool* hovered, bool* held) {
    const bool hov = hovered_(id, r);

    if (hov && mouse_pressed_) active_id_ = id;

    // Clicked: released over the widget it was pressed on. Dragging off
    // and releasing elsewhere cancels, like every desktop button.
    bool clicked = false;
    if (active_id_ == id && mouse_released_) {
        clicked    = hov;
        active_id_ = 0;
    }

    if (hov)     ui_cursor_ = CursorStyle::Hand;
    if (hovered) *hovered   = hov;
    if (held)    *held      = active_id_ == id && mouse_down_;
    return clicked;
}

// -----------------------------------------------------------------------------
// Helpers: drawing
// -----------------------------------------------------------------------------

static glm::vec4 box4(const Rect& r) {
    return { r.min.x, r.min.y, r.max.x, r.max.y };
}

void Context::draw_box_(const Rect& r, const glm::vec4& color, float radius,
                        const glm::vec4& border, float border_width)
{
    const Rect visible = r.intersect(clip_);
    if (visible.width() <= 0.0f || visible.height() <= 0.0f) return;
    if (color.a <= 0.0f && (border.a <= 0.0f || border_width <= 0.0f)) return;

    list_->push_back(UIQuad{
        .pos_box      = box4(r),
        .color        = color,
        .border_color = border,
        .clip_box     = box4(clip_),
        .params       = { radius, border_width, float(UIQuadShape::Box), 0.0f },
    });
}

void Context::draw_shape_(const Rect& r, UIQuadShape shape, const glm::vec4& color,
                          float stroke)
{
    const Rect visible = r.intersect(clip_);
    if (visible.width() <= 0.0f || visible.height() <= 0.0f) return;

    list_->push_back(UIQuad{
        .pos_box  = box4(r),
        .color    = color,
        .clip_box = box4(clip_),
        .params   = { 0.0f, stroke, float(shape), 0.0f },
    });
}

float Context::draw_text_(glm::vec2 pos, std::string_view text, const glm::vec4& color) {
    if (!font_ || text.empty()) return 0.0f;

    const Render::TextStyle style = {
        .size   = style_.font_size,
        .valign = Render::TextVAlign::Top,
    };

    // layout_text works y up: lay out at -y, then negate back to y down
    const glm::vec4 bounds = Render::layout_text(*font_, text, { pos.x, -pos.y }, style,
        [&](const Render::FontGlyph& glyph, const glm::vec4& box) {
            const Rect r = { { box.x, -box.w }, { box.z, -box.y } };
            const Rect visible = r.intersect(clip_);
            if (visible.width() <= 0.0f || visible.height() <= 0.0f) return;

            list_->push_back(UIQuad{
                .pos_box  = box4(r),
                .tex_box  = glyph.tex_box,
                .color    = color,
                .clip_box = box4(clip_),
                .params   = { 0.0f, 0.0f, float(UIQuadShape::Glyph), 0.0f },
            });
        });

    return bounds.z - bounds.x;
}

float Context::text_width_(std::string_view text) const {
    if (!font_ || text.empty()) return 0.0f;
    float w = 0.0f;
    for (char c : text) {
        if (const Render::FontGlyph* g = font_->glyph_or_fallback(c))
            w += g->advance * text_scale_;
    }
    return w;
}

void Context::draw_text_in_(const Rect& r, std::string_view text, const glm::vec4& color,
                            float pad, bool center)
{
    const float y = r.min.y + std::round((r.height() - text_height_) * 0.5f);
    const float x = center ? std::round(r.center().x - text_width_(text) * 0.5f)
                           : r.min.x + pad;

    const Rect saved = clip_;
    clip_ = clip_.intersect(r);
    draw_text_({ x, y }, text, color);
    clip_ = saved;
}

void Context::submit_(const DrawList& list) {
    // UI pixels (y down) -> screen view (y up)
    const float h = screen_size_.y;
    const Render::DrawParams params = {
        .view     = config_.view,
        .layer    = config_.layer,
        .material = font_->material(),
    };

    for (const UIQuad& q : list) {
        UIQuad out   = q;
        out.pos_box  = { q.pos_box.x,  h - q.pos_box.w,  q.pos_box.z,  h - q.pos_box.y  };
        out.clip_box = { q.clip_box.x, h - q.clip_box.w, q.clip_box.z, h - q.clip_box.y };
        renderer_.draw<Render::UIQuadKind>(out, params);
    }
}

// -----------------------------------------------------------------------------
// Helpers: events
// -----------------------------------------------------------------------------

void Context::subscribe_() {

    listeners_[0] = events_.subscribe<OnMouseMotion>(
        [this](const OnMouseMotion& e) {
            mouse_ = glm::vec2(e.pos_x, e.pos_y) * mouse_scale_;
        }
    );

    listeners_[1] = events_.subscribe<OnMouseButtonDown>(
        [this](const OnMouseButtonDown& e) {
            if (e.button != MouseButton::Left) return;
            mouse_down_    = true;
            mouse_pressed_ = true;
        }
    );

    listeners_[2] = events_.subscribe<OnMouseButtonUp>(
        [this](const OnMouseButtonUp& e) {
            if (e.button != MouseButton::Left) return;
            mouse_down_     = false;
            mouse_released_ = true;
        }
    );

    listeners_[3] = events_.subscribe<OnMouseWheel>(
        [this](const OnMouseWheel& e) {
            wheel_ += e.rel_y;
        }
    );

    // Text and editing keys only matter while a text field has focus.
    // The font covers printable ASCII, so anything else is dropped.
    listeners_[4] = events_.subscribe<OnTextInput>(
        [this](const OnTextInput& e) {
            if (!focus_id_) return;
            for (const char* c = e.text; *c; c++)
                if (*c >= 32 && *c <= 126) typed_.push_back(*c);
        }
    );

    listeners_[5] = events_.subscribe<OnKeyDown>(
        [this](const OnKeyDown& e) {
            if (!focus_id_) return;
            switch (e.scancode) {
                case ScanCode::Backspace:
                case ScanCode::Delete:
                case ScanCode::Left:
                case ScanCode::Right:
                case ScanCode::Home:
                case ScanCode::End:
                case ScanCode::Return:
                case ScanCode::Escape:
                    keys_.push_back(e.scancode);
                    break;
                default:
                    break;
            }
        }
    );
}

} // namespace Nezumi::UI
