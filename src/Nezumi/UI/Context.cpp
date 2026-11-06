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
#include "Nezumi/Render/2D/Renderer2D.hpp"
#include <algorithm> // clamp, find_if, max, min, rotate
#include <chrono>    // steady_clock
#include <cmath>     // ceil, floor, round
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
    Render::Renderer2D& renderer,
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

    // Screen size and scale from the view's camera (sized to the backbuffer
    // and given the display scale by the renderer). Mouse events are window
    // pixels; the UI works in the view's units (points on HiDPI screens).
    if (const Render::Camera2D* camera = renderer_.get_view_camera(config_.view)) {
        scale_       = camera->get_scale();
        screen_size_ = camera->get_size() / scale_;
    }
    mouse_ = mouse_pixels_ / scale_;
    if (mouse_pressed_) press_pos_ = mouse_;

    now_ = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

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

    // A drag released since the last frame lands this frame: the target
    // under the cursor claims it in drop_target()
    drop_frame_   = drag_active_ && mouse_released_ && !mouse_down_;
    drop_claimed_ = false;

    ui_cursor_    = CursorStyle::Arrow;
    focus_seen_   = false;
    popup_drawn_  = false;
    tooltip_seen_ = false;
    last_rect_    = {};
    last_hovered_ = false;
    overlay_.clear();
}

void Context::end_frame() {
    assert(in_frame_ && "UI: end_frame without begin_frame");
    assert(!layout_.panel && "UI: begin_panel without end_panel");

    // Forget a dropdown or text field whose panel wasn't drawn this frame
    if (!popup_drawn_)               popup_id_ = 0;
    if (focus_id_ && !focus_seen_)   focus_id_ = 0;
    if (!tooltip_seen_)              tooltip_id_ = 0;

    // An unclaimed drop lasts one frame after its release
    if (unclaimed_ && unclaimed_frame_ < frame_) unclaimed_.reset();

    // The dragged image follows the cursor, above everything. A drag that
    // landed this frame and wasn't claimed is kept for take_unclaimed_drop().
    if (drag_active_) {
        if (drop_frame_) {
            if (!drop_claimed_) {
                unclaimed_ = UnclaimedDrop{
                    .payload  = drag_,
                    .position = mouse_,
                    .on_panel = panel_at_(mouse_) != nullptr,
                };
                unclaimed_frame_ = frame_;
            }
            drag_active_ = false;
            drag_id_     = 0;
        } else {
            draw_drag_();
        }
    }

    // A release ends any hold, even on a widget that wasn't drawn this frame
    if (mouse_released_ && !mouse_down_) {
        active_id_   = 0;
        drag_panel_  = 0;
        scroll_drag_ = 0;
    }

    // Back to front, then the open dropdown on top. One layer and kind for
    // all of it, and a new draw order whenever the texture changes, so it
    // stays in this order and draws in one instanced call per texture run
    // (see UIQuad.hpp and "Drawing order" in Context.hpp).
    if (font_) {
        submit_order_    = 0;
        submit_material_ = font_->material();
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
    right_pressed_  = false;
    wheel_          = 0.0f;
    text_in_.clear();

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

    // A tab picked from outside (set_panel_tab)
    for (auto it = pending_tabs_.begin(); it != pending_tabs_.end(); ++it) {
        if (it->first != id) continue;
        if (panel->tab != it->second) panel->scroll = 0.0f;
        panel->tab = it->second;
        pending_tabs_.erase(it);
        break;
    }

    Panel& p   = *panel;
    p.flags    = config.flags;
    p.min_size = config.min_size;
    p.scroll_step = std::max(0.0f, config.scroll_step);
    p.anchor   = config.position;
    p.pivot    = config.pivot;
    p.hit_box  = config.hit_box;
    p.icon     = config.icon;
    p.tab_placement = config.tab_placement;
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

        // Tabs in the title bar are for clicking, not dragging
        bool on_tab = false;
        if (tabs_in_title_(p))
            for (std::size_t i = 0; i < p.tab_x.size(); i++) on_tab = on_tab || tab_rect_(p, i).contains(mouse_);

        if (edges != EdgeNone || (can_move && title_bar.contains(mouse_) && !on_close && !on_tab)) {
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
    update_tabs_(p, config.tabs, config.tab_icons);

    // Layout: content inside the padding and below the title, scrolled.
    // AutoSize panels grow to fit, so they never clip or scroll.
    const float title_h = header_height_(p);
    const float border  = p.has(PanelFlags::NoBackground) ? 0.0f : thickness_(style_.panel_border_width);
    const Rect  inner   = Rect{ { p.pos.x, p.pos.y + title_h }, p.pos + p.size }
                              .shrink(glm::vec2(border));
    const Rect  content = Rect{ { p.pos.x, p.pos.y + title_h }, p.pos + p.size }
                              .shrink(padding_(p));
    const bool  auto_size = p.has(PanelFlags::AutoSize);

    if (!auto_size) {
        const float step = p.scroll_step;

        // The scrollbar: press the thumb to drag it, or the track to jump
        // the thumb's middle there first
        Scrollbar bar;
        if (scrollbar_(p, bar)) {
            const Rect hit = { { bar.track.min.x - thickness_(3.0f), bar.track.min.y },
                               { bar.track.max.x + thickness_(3.0f), bar.track.max.y } };
            if (scroll_drag_ == 0 && hot_panel_ == id && !on_popup_ && active_id_ == 0 &&
                mouse_pressed_ && hit.contains(mouse_)) {
                scroll_drag_ = id;
                active_id_   = hash_("#scrollbar", id);
                scroll_grab_ = bar.thumb.contains(mouse_) ? mouse_.y - bar.thumb.min.y
                                                          : bar.thumb.height() * 0.5f;
            }
            if (scroll_drag_ == id) {
                if (mouse_down_) {
                    const float travel = bar.track.height() - bar.thumb.height();
                    const float t      = travel > 0.0f
                        ? std::clamp((mouse_.y - scroll_grab_ - bar.track.min.y) / travel, 0.0f, 1.0f) : 0.0f;
                    p.scroll = t * bar.max_scroll;
                    if (step > 0.0f) p.scroll = std::round(p.scroll / step) * step;
                } else {
                    scroll_drag_ = 0;
                }
            }
        } else if (scroll_drag_ == id) {
            scroll_drag_ = 0;
        }

        // The wheel: one step per notch, or scroll_speed points
        if (hot_panel_ == id && !on_popup_ && wheel_ != 0.0f) {
            if (step > 0.0f) {
                const float notches = wheel_ > 0.0f ? std::ceil(wheel_) : std::floor(wheel_);
                p.scroll = (std::round(p.scroll / step) - notches) * step;
            } else {
                p.scroll -= wheel_ * style_.scroll_speed;
            }
            wheel_ = 0.0f; // one panel scrolls per notch
        }
        p.scroll = std::clamp(p.scroll, 0.0f, max_scroll_(p));
    } else {
        p.scroll = 0.0f;
    }

    layout_          = {};
    layout_.panel    = &p;
    layout_.content  = content;
    layout_.clip     = auto_size ? NO_CLIP : inner;

    // Stepped scrolling shows whole rows, so nothing peeks into the padding
    // above or below them
    if (!auto_size && p.scroll_step > 0.0f) {
        layout_.clip.min.y = content.min.y;
        layout_.clip.max.y = content.max.y;
    }
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
        p.size = p.content + 2.0f * padding_(p) + glm::vec2(0.0f, header_height_(p));
        p.size.x = std::max(p.size.x, header_width_(p, tabs_in_title_(p)));
        p.size   = glm::max(p.size, p.min_size);

        // A centered (pivoted) panel can only be placed once its size is
        // known, and a panel that grew may now hang off the screen. Shift
        // what was already laid out by however far it moved.
        if (p.place) p.pos = p.anchor - p.pivot * p.size;
        clamp_to_screen_(p);
        const glm::vec2 d = p.pos - old_pos;
        if (d != glm::vec2(0.0f)) {
            for (DrawItem& item : p.body) {
                UIQuad& q = item.quad;
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
    const float y = snap_floor_(r.center().y);
    draw_box_({ { r.min.x, y }, { r.max.x, y + thickness_(1.0f) } }, style_.separator);
}

void Context::set_next_pos(glm::vec2 offset) {
    if (!layout_.panel) return;
    layout_.cursor    = layout_.origin + offset;
    layout_.same_line = false;
}

Rect Context::allocate(glm::vec2 size) {
    if (!layout_.panel) return {};
    const Rect r = place_(size);
    set_last_item_(r);
    return r;
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
    if (panel.hit_box.width() > 0.0f && panel.hit_box.height() > 0.0f)
        return { panel.pos + panel.hit_box.min, panel.pos + panel.hit_box.max };
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

    // Resize: move the dragged edges, keep the others still. A dragged edge
    // stops at the screen's edge (clamping afterwards would move the panel
    // instead, dragging the opposite edge along).
    glm::vec2 lo = drag_pos_;
    glm::vec2 hi = drag_pos_ + drag_size_;
    const glm::vec2 min = panel.min_size;
    const glm::vec2 max = screen_size_;
    if (l) lo.x = std::min(std::max(lo.x + d.x, 0.0f),  hi.x - min.x);
    if (r) hi.x = std::max(std::min(hi.x + d.x, max.x), lo.x + min.x);
    if (t) lo.y = std::min(std::max(lo.y + d.y, 0.0f),  hi.y - min.y);
    if (b) hi.y = std::max(std::min(hi.y + d.y, max.y), lo.y + min.y);
    panel.pos  = lo;
    panel.size = hi - lo;
}

void Context::clamp_to_screen_(Panel& panel) const {
    if (screen_size_.x <= 0.0f || screen_size_.y <= 0.0f) return;

    // A sized panel bigger than the screen (say, after the window shrank)
    // shrinks to fit, down to its min_size. AutoSize panels fit their
    // content instead, so they keep their size.
    if (!panel.has(PanelFlags::AutoSize))
        panel.size = glm::max(glm::min(panel.size, screen_size_), panel.min_size);

    // Keep the whole panel on screen. One still bigger than the screen sits
    // at the top-left, where its title bar stays reachable.
    panel.pos = glm::clamp(panel.pos, glm::vec2(0.0f),
                           glm::max(screen_size_ - panel.size, glm::vec2(0.0f)));

    // On whole pixels, so everything inside is too (a pivot, AutoSize text
    // widths and the display scale all make fractions)
    panel.pos  = snap_(panel.pos);
    panel.size = snap_(panel.size);
}

// -----------------------------------------------------------------------------
// Header
//
// One row along the top of every panel (but NoTitleBar ones), left to right:
// the icon and the title (both optional), the tabs (when they fit, see
// TabPlacement), empty space to drag the panel by, and the close button.
// Tabs that don't fit get a strip of their own, as tall as the header,
// right under it. Every x here is from the panel's left edge.
// -----------------------------------------------------------------------------

float Context::title_height_(const Panel& panel) const {
    if (panel.has(PanelFlags::NoTitleBar)) return 0.0f;
    return snap_(text_height_ + 2.0f * style_.title_padding + 2.0f * style_.frame_padding.y);
}

Rect Context::icon_rect_(const Panel& panel) const {
    const float h = title_height_(panel);
    const float s = snap_(text_height_ + 2.0f * style_.frame_padding.y - 4.0f);
    return Rect::from_size(snap_(panel.pos + glm::vec2(padding_(panel).x, (h - s) * 0.5f)), glm::vec2(s));
}

float Context::lead_end_(const Panel& p) const {
    if (p.has(PanelFlags::NoTitleBar)) return 0.0f;
    float x = padding_(p).x;
    bool  any = false;
    if (p.icon) { x = icon_rect_(p).max.x - p.pos.x; any = true; }
    if (shows_title_(p)) {
        if (any) x += style_.title_padding;
        x  += text_width_(p.title);
        any = true;
    }
    return any ? snap_(x) : 0.0f;
}

float Context::tab_width_(const Panel& p, std::size_t i) const {
    const bool  has_icon  = i < p.tab_icons.size() && p.tab_icons[i];
    const bool  has_label = !p.tabs[i].empty();
    float w = 2.0f * style_.tab_padding;
    if (has_icon)              w += snap_(text_height_);
    if (has_icon && has_label) w += style_.title_padding;
    if (has_label)             w += text_width_(p.tabs[i]);
    return snap_(w);
}

float Context::header_width_(const Panel& p, bool tabs_inline) const {
    if (p.has(PanelFlags::NoTitleBar)) return 0.0f;
    const float h     = title_height_(p);
    const float bw    = thickness_(style_.panel_border_width);
    float       tabs  = 0.0f;
    for (std::size_t i = 0; i < p.tabs.size(); i++) tabs += tab_width_(p, i);

    // Some room left to drag the panel by, and the close button
    const float lead  = lead_end_(p);
    const float right = (p.closable ? h : 0.0f) + bw + (p.closable ? h * 0.5f : padding_(p).x);
    const float in_header = tabs_inline ? (lead > 0.0f ? lead + style_.title_padding * 0.5f : bw) + tabs
                                        : std::max(lead, padding_(p).x);
    float w = in_header + right;

    // A strip as wide as its tabs
    if (!tabs_inline && !p.tabs.empty())
        w = std::max(w, std::max(bw, padding_(p).x - style_.tab_padding) * 2.0f + tabs);
    return snap_(w);
}

// -----------------------------------------------------------------------------
// Tabs
// -----------------------------------------------------------------------------

int Context::panel_tab() const {
    return layout_.panel ? layout_.panel->tab : 0;
}

void Context::set_panel_tab(std::string_view title, int tab) {
    const ID id = hash_(title, PANEL_SEED);
    for (auto& pending : pending_tabs_) {
        if (pending.first == id) { pending.second = tab; return; }
    }
    pending_tabs_.emplace_back(id, tab);
}

float Context::tabs_height_(const Panel& panel) const {
    if (panel.tabs.empty() || tabs_in_title_(panel)) return 0.0f;
    return title_height_(panel) > 0.0f ? title_height_(panel)
                                       : snap_(text_height_ + 2.0f * style_.title_padding + 2.0f * style_.frame_padding.y);
}

void Context::update_tabs_(Panel& p, std::span<const std::string_view> tabs, std::span<const Image> icons) {
    p.tabs.resize(tabs.size());
    p.tab_x.resize(tabs.size());
    p.tab_icons.assign(icons.begin(), icons.begin() + std::min(icons.size(), tabs.size()));
    p.tab_hot = -1;
    if (tabs.empty()) { p.tab = 0; p.tabs_inline = false; return; }
    for (std::size_t i = 0; i < tabs.size(); i++) p.tabs[i] = std::string(visible_(tabs[i]));

    // In the header or in a strip. Auto asks whether the header fits them
    // in the width the panel has without them: its content's (AutoSize, as
    // measured last frame) or its own. Placing the tabs never changes that
    // width, so the choice doesn't flip back and forth between frames.
    switch (p.tab_placement) {
        case TabPlacement::Header: p.tabs_inline = true;  break;
        case TabPlacement::Strip:  p.tabs_inline = false; break;
        case TabPlacement::Auto: {
            const float width = p.has(PanelFlags::AutoSize)
                ? std::max(p.content.x + 2.0f * padding_(p).x, p.min_size.x)
                : p.size.x;
            p.tabs_inline = header_width_(p, true) <= width + 0.5f;
            break;
        }
    }
    if (p.has(PanelFlags::NoTitleBar)) p.tabs_inline = false;

    // Left to right, side by side: after the icon and title, or from the
    // panel's edge in a strip (so the first label lines up with the content)
    const float bw   = thickness_(style_.panel_border_width);
    const float lead = lead_end_(p);
    p.tab_left = tabs_in_title_(p) ? (lead > 0.0f ? lead + snap_(style_.title_padding * 0.5f) : bw)
                                   : std::max(bw, padding_(p).x - style_.tab_padding);
    float x = p.tab_left;
    for (std::size_t i = 0; i < tabs.size(); i++) {
        x         += tab_width_(p, i);
        p.tab_x[i] = x;
    }
    p.tab = std::clamp(p.tab, 0, static_cast<int>(tabs.size()) - 1);

    // Hover and click; a click on another tab picks it and scrolls the
    // content back to the top
    if (hot_panel_ != p.id || on_popup_ || active_id_ != 0) return;
    for (std::size_t i = 0; i < tabs.size(); i++) {
        if (!tab_rect_(p, i).contains(mouse_)) continue;
        p.tab_hot  = static_cast<int>(i);
        ui_cursor_ = CursorStyle::Hand;
        if (mouse_pressed_ && p.tab != static_cast<int>(i)) {
            p.tab    = static_cast<int>(i);
            p.scroll = 0.0f;
        }
    }
}

Rect Context::tab_rect_(const Panel& p, std::size_t i) const {
    // The full height of the header (or the strip), down to the line under it
    const float bw   = p.has(PanelFlags::NoTitleBar) ? 0.0f : thickness_(style_.panel_border_width);
    const float top  = p.pos.y + (tabs_in_title_(p) ? bw : title_height_(p));
    const float bot  = p.pos.y + header_height_(p);
    const float left = i == 0 ? p.tab_left : p.tab_x[i - 1];
    return { { p.pos.x + left, top }, { p.pos.x + p.tab_x[i], bot } };
}

void Context::draw_tabs_(const Panel& p) {
    if (p.tabs.empty()) return;
    const float line = thickness_(1.0f);
    const float mark = thickness_(2.0f);
    const bool  in_title = tabs_in_title_(p);

    for (std::size_t i = 0; i < p.tabs.size(); i++) {
        const Rect tab    = tab_rect_(p, i);
        const bool picked = static_cast<int>(i) == p.tab;
        const bool hot    = static_cast<int>(i) == p.tab_hot;

        // A wash on hover (and on the picked tab in a strip), and the picked
        // one underlined in the accent, over the line under the header
        const Rect body = { tab.min, { tab.max.x, tab.max.y - line } };
        if (picked && !in_title) draw_box_(body, style_.tab_picked);
        else if (hot && !picked) draw_box_(body, style_.tab_hovered);
        if (picked) draw_box_({ { tab.min.x, tab.max.y - mark }, tab.max }, style_.accent);

        // Icon, then label, centered in the tab as a group
        const glm::vec4& color = (picked || hot) ? style_.text : style_.text_dim;
        float x = tab.min.x + style_.tab_padding;
        if (i < p.tab_icons.size() && p.tab_icons[i]) {
            const float s = snap_(text_height_);
            const float b = (picked || hot) ? 1.0f : 0.7f;
            draw_image_(Rect::from_size({ x, snap_(tab.center().y - s * 0.5f) }, glm::vec2(s)),
                        p.tab_icons[i], { b, b, b, (picked || hot) ? 1.0f : 0.75f });
            x += s + style_.title_padding;
        }
        if (!p.tabs[i].empty())
            draw_text_({ x, tab.min.y + snap_((body.height() - text_height_) * 0.5f) }, p.tabs[i], color);
    }
}

float Context::max_scroll_(const Panel& p) const {
    const float view_h = p.size.y - header_height_(p) - 2.0f * padding_(p).y;
    const float max    = std::max(0.0f, p.content.y - view_h);

    // Stepped: far enough that the last step shows whole (a row pitch
    // includes the spacing after the last row, which the content doesn't)
    if (p.scroll_step > 0.0f && max > 0.0f) return std::ceil(max / p.scroll_step - 1e-3f) * p.scroll_step;
    return max;
}

bool Context::scrollbar_(const Panel& p, Scrollbar& out) const {
    if (p.has(PanelFlags::AutoSize)) return false;
    const float title_h = header_height_(p);
    const float view_h  = p.size.y - title_h - 2.0f * padding_(p).y;
    if (view_h <= 0.0f || p.content.y <= view_h) return false;

    const Rect  r       = p.rect();
    const float bw      = p.has(PanelFlags::NoBackground) ? 0.0f : thickness_(style_.panel_border_width);
    const float w       = snap_(style_.scrollbar_width);
    const float x       = r.max.x - bw - thickness_(2.0f) - w;
    const float top     = r.min.y + title_h + padding_(p).y;

    out.max_scroll      = max_scroll_(p);
    out.track           = Rect::from_size({ x, top }, { w, view_h });
    const float thumb_h = std::max(16.0f, view_h * view_h / (view_h + out.max_scroll));
    const float t       = out.max_scroll > 0.0f ? std::clamp(p.scroll / out.max_scroll, 0.0f, 1.0f) : 0.0f;
    out.thumb           = Rect::from_size({ x, snap_(top + (view_h - thumb_h) * t) }, { w, snap_(thumb_h) });
    return true;
}

Rect Context::close_rect_(const Panel& panel) const {
    // A square cell at the header's right end, inside the outline
    const float bw = thickness_(style_.panel_border_width);
    const float h  = title_height_(panel) - bw - thickness_(1.0f);
    return Rect::from_size({ panel.pos.x + panel.size.x - bw - h, panel.pos.y + bw }, glm::vec2(h));
}

void Context::draw_chrome_(Panel& p) {
    list_ = &p.chrome;
    clip_ = NO_CLIP;

    const Rect  r       = p.rect();
    const float title_h = title_height_(p);
    const float bw      = thickness_(style_.panel_border_width);
    const float line    = thickness_(1.0f);
    const float radius  = style_.panel_rounding;
    const bool  front   = p.id == front_panel_;

    if (p.has(PanelFlags::NoBackground)) return;

    // Hard drop shadow, two steps (none unless the style gives it a color)
    if (style_.panel_shadow.a > 0.0f) {
        const float s1 = thickness_(1.0f), s2 = thickness_(2.0f);
        glm::vec4 far = style_.panel_shadow;
        far.a *= 0.7f;
        draw_box_(Rect{ r.min + glm::vec2(0.0f, s2), r.max + glm::vec2(s2, 2.0f * s2) }, far, radius + s2);
        draw_box_(Rect{ r.min + glm::vec2(0.0f, s1), r.max + glm::vec2(s1, 2.0f * s1) }, style_.panel_shadow, radius + s1);
    }

    // Background and outline: the front panel's in a gradient
    if (front && style_.border_front.a + style_.border_front_end.a > 0.0f) {
        draw_box_(r, style_.panel_bg, radius);
        list_->push_back({ UIQuad{
            .pos_box      = { r.min.x, r.min.y, r.max.x, r.max.y },
            .color        = style_.border_front,
            .border_color = style_.border_front_end,
            .clip_box     = { NO_CLIP.min.x, NO_CLIP.min.y, NO_CLIP.max.x, NO_CLIP.max.y },
            .params       = { radius, bw, float(UIQuadShape::GradientRing),
                              glm::radians(style_.border_gradient_angle) },
        } });
    } else {
        draw_box_(r, style_.panel_bg, radius, style_.panel_border, style_.panel_border_width);
    }

    // Header: a fill if the style has one, and a line under it (under the
    // tab strip instead, when there is one)
    if (title_h > 0.0f) {
        const glm::vec4& bg = front ? style_.title_bg_front : style_.title_bg;
        const Rect bar = Rect{ r.min, { r.max.x, r.min.y + title_h } }.shrink(glm::vec2(bw));
        if (bg.a > 0.0f) {
            draw_box_(bar, bg, std::max(0.0f, radius - bw));
            if (radius > bw) draw_box_({ { bar.min.x, bar.center().y }, bar.max }, bg);
        }

        float x = r.min.x + padding_(p).x;
        if (p.icon) {
            const Rect ir = icon_rect_(p);
            draw_image_(ir, p.icon, glm::vec4(1.0f));
            x = ir.max.x + style_.title_padding;
        }
        if (shows_title_(p)) {
            clip_ = bar;
            draw_text_({ x, r.min.y + snap_((title_h - text_height_) * 0.5f) }, p.title, style_.heading);
            clip_ = NO_CLIP;
        }

        if (p.closable) {
            const Rect cr = close_rect_(p);
            if (p.close_hot) draw_box_(cr, style_.close_hovered, style_.frame_rounding);
            const Rect x = cr.shrink(glm::vec2(snap_(cr.width() * 0.34f)));
            // Outlined like the text, so it reads with no panel behind it
            if (style_.text_outline_width > 0.0f && style_.text_outline.a > 0.0f && !p.close_hot)
                draw_shape_(x, UIQuadShape::Cross, style_.text_outline, 1.5f + 2.0f * style_.text_outline_width);
            draw_shape_(x, UIQuadShape::Cross, p.close_hot ? style_.close_text_hovered : style_.text_dim, 1.5f);
        }
    }

    const float hh = header_height_(p);
    if (hh > 0.0f) draw_box_({ { r.min.x + bw, r.min.y + hh - line }, { r.max.x - bw, r.min.y + hh } }, style_.header_line);

    draw_tabs_(p);

    // Scrollbar: a thumb showing which part of the content is in view,
    // brighter while held
    Scrollbar bar;
    if (scrollbar_(p, bar)) {
        glm::vec4 color = style_.scrollbar;
        if (scroll_drag_ == p.id) color = style_.accent;
        draw_box_(bar.thumb, color, std::min(style_.frame_rounding, style_.scrollbar_width * 0.5f));
    }

    // Resize grip in the bottom-right corner
    if (p.has(PanelFlags::Resizable) && !p.has(PanelFlags::AutoSize)) {
        const float g = 8.0f;
        draw_shape_(Rect{ r.max - glm::vec2(g + bw + 2.0f), r.max - glm::vec2(bw + 2.0f) },
                    UIQuadShape::Corner, style_.separator);
    }
}

// -----------------------------------------------------------------------------
// Helpers: layout and interaction
// -----------------------------------------------------------------------------

Rect Context::place_(glm::vec2 size, bool fill) {
    Layout& L = layout_;
    L.next_width = 0.0f; // used up by whatever's placed next

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

    return Rect::from_size(snap_(pos), size);
}

float Context::frame_height_() const {
    return snap_(text_height_ + 2.0f * style_.frame_padding.y);
}

float Context::control_width_() const {
    if (layout_.next_width > 0.0f) return snap_(layout_.next_width);
    const Panel* p = layout_.panel;
    if (!p || p->has(PanelFlags::AutoSize)) return style_.control_width;
    return std::max(60.0f, snap_(layout_.content.width() * style_.control_fraction));
}

float Context::avail_width_() const {
    const Panel* p = layout_.panel;
    if (!p) return 0.0f;

    // An AutoSize panel is as wide as its widest widget last frame (or its
    // minimum size)
    const float right = p->has(PanelFlags::AutoSize)
        ? layout_.origin.x + std::max(p->content.x, p->size.x - 2.0f * padding_(*p).x)
        : layout_.content.max.x;
    const float x = layout_.same_line ? layout_.prev_end.x + layout_.same_space
                                      : layout_.cursor.x;
    return std::max(0.0f, right - x);
}

glm::vec2 Context::padding_(const Panel& panel) const {
    return panel.has(PanelFlags::NoBackground) ? glm::vec2(0.0f) : style_.panel_padding;
}

float Context::label_width_(std::string_view visible) const {
    return visible.empty() ? 0.0f : style_.item_spacing.x + text_width_(visible);
}

void Context::draw_label_(const Rect& row, float x, std::string_view visible) {
    if (visible.empty()) return;
    draw_text_({ x + style_.item_spacing.x, row.min.y + snap_((row.height() - text_height_) * 0.5f) },
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
    // and releasing elsewhere cancels, like every desktop button, and so
    // does dragging a payload out of it and back.
    bool clicked = false;
    if (active_id_ == id && mouse_released_) {
        clicked    = hov && drag_id_ != id;
        active_id_ = 0;
    }

    if (hov)     ui_cursor_ = CursorStyle::Hand;
    if (hovered) *hovered   = hov;
    if (held)    *held      = active_id_ == id && mouse_down_;
    set_last_item_(r);
    return clicked;
}

void Context::set_last_item_(const Rect& r) {
    last_rect_    = r;
    last_hovered_ = over_(r);
}

bool Context::over_(const Rect& r) const {
    if (!layout_.panel || hot_panel_ != layout_.panel->id) return false;
    if (on_popup_) return false;
    return r.contains(mouse_) && clip_.contains(mouse_);
}

// -----------------------------------------------------------------------------
// Custom Widgets
// -----------------------------------------------------------------------------

Context::Interaction Context::interact(ID id, const Rect& r) {
    Interaction it;
    if (!layout_.panel) return it;
    it.clicked       = behavior_(id, r, &it.hovered, &it.held);
    it.right_clicked = it.hovered && right_pressed_;
    it.wheel         = it.hovered ? wheel_ : 0.0f;
    return it;
}

void Context::tooltip(std::string_view text) {
    if (!layout_.panel || text.empty()) return;

    // Only while resting on it: not while a button is held or dragging
    if (!last_hovered_ || mouse_down_ || drag_active_) return;

    // Timed per widget (where it sits in its panel), not per text, so a
    // tooltip whose text changes while it's up (a countdown) stays up
    const glm::vec4 where = { last_rect_.min - layout_.panel->pos, last_rect_.max - layout_.panel->pos };
    const ID id = hash_(std::string_view(reinterpret_cast<const char*>(&where), sizeof where), layout_.panel->id);
    tooltip_seen_ = true;
    if (tooltip_id_ != id) {
        tooltip_id_    = id;
        tooltip_start_ = now_;
    }
    if (now_ - tooltip_start_ >= style_.tooltip_delay)
        draw_tooltip_(text, last_rect_);
}

void Context::draw_tooltip_(std::string_view text, const Rect& anchor) {
    DrawList* saved_list = list_;
    const Rect saved_clip = clip_;
    list_ = &overlay_;
    clip_ = NO_CLIP;

    // Above the widget, centered on it, or below it if there is no room;
    // always on screen
    // One line per '\n'
    const float line_h = snap_(text_height_ + style_.item_spacing.y * 0.5f);
    float       text_w = 0.0f;
    int         lines  = 0;
    for (std::size_t at = 0; at <= text.size(); lines++) {
        const std::size_t end = std::min(text.find('\n', at), text.size());
        text_w = std::max(text_w, text_width_(text.substr(at, end - at)));
        at = end + 1;
    }

    const glm::vec2 pad  = style_.frame_padding;
    const glm::vec2 size = snap_(glm::vec2(text_w, text_height_ + line_h * static_cast<float>(lines - 1)) + 2.0f * pad);
    const float     gap  = 6.0f;
    glm::vec2 pos = { anchor.center().x - size.x * 0.5f, anchor.min.y - gap - size.y };
    if (pos.y < 0.0f) pos.y = anchor.max.y + gap;
    if (screen_size_.x > 0.0f) {
        pos.x = std::clamp(pos.x, 2.0f, std::max(2.0f, screen_size_.x - size.x - 2.0f));
        pos.y = std::clamp(pos.y, 2.0f, std::max(2.0f, screen_size_.y - size.y - 2.0f));
    }
    const Rect box = Rect::from_size(snap_(pos), size);

    draw_box_(box, style_.tooltip_bg, style_.frame_rounding, style_.tooltip_border, style_.border_width);
    glm::vec2 at = box.min + pad;
    for (std::size_t from = 0; from <= text.size(); at.y += line_h) {
        const std::size_t end = std::min(text.find('\n', from), text.size());
        draw_text_(at, text.substr(from, end - from), style_.text);
        from = end + 1;
    }

    list_ = saved_list;
    clip_ = saved_clip;
}

// -----------------------------------------------------------------------------
// Drag and Drop
// -----------------------------------------------------------------------------

bool Context::begin_drag(ID id, const DragPayload& payload) {
    if (drag_active_) return drag_id_ == id;
    if (payload.type == 0 || active_id_ != id || !mouse_down_) return false;

    const glm::vec2 d = mouse_ - press_pos_;
    if (glm::dot(d, d) < style_.drag_threshold * style_.drag_threshold) return false;

    drag_active_ = true;
    drag_id_     = id;
    drag_        = payload;
    if (drag_.source == 0) drag_.source = id;
    return true;
}

DropResult Context::drop_target(const Rect& r, std::uint32_t type) {
    DropResult result;
    if (!drag_active_ || type == 0 || drag_.type != type) return result;
    if (!over_(r)) return result;

    result.hovered = true;
    if (drop_frame_ && !drop_claimed_) {
        drop_claimed_  = true;
        result.payload = drag_;
    }
    return result;
}

std::optional<UnclaimedDrop> Context::take_unclaimed_drop(std::uint32_t type) {
    if (!unclaimed_ || unclaimed_->payload.type != type) return std::nullopt;
    std::optional<UnclaimedDrop> drop = unclaimed_;
    unclaimed_.reset();
    return drop;
}

void Context::draw_drag_() {
    if (!drag_.image) return;
    list_ = &overlay_;
    clip_ = NO_CLIP;

    // At the image's natural size, centered on the cursor, a little see-through
    const glm::vec2 size = drag_.image.size.x > 0.0f ? drag_.image.size : glm::vec2(32.0f);
    draw_image_(Rect::from_size(snap_(mouse_ - size * 0.5f), snap_(size)), drag_.image,
                { 1.0f, 1.0f, 1.0f, 0.85f }, 3.0f);

    list_ = nullptr;
}

// -----------------------------------------------------------------------------
// Drawing
// -----------------------------------------------------------------------------

void Context::draw_rect(const Rect& r, const glm::vec4& color, float radius,
                        const glm::vec4& border, float border_width)
{
    if (!layout_.panel) return;
    draw_box_(r, color, radius, border, border_width);
}

void Context::draw_image(const Rect& r, const Image& image, const glm::vec4& tint, float radius) {
    if (!layout_.panel) return;
    draw_image_(r, image, tint, radius);
}

void Context::draw_nine_slice(const Rect& r, const NineSlice& slice, const glm::vec4& tint) {
    if (!layout_.panel || !slice.image) return;

    // Margins no bigger than half the rect, so small rects still look right
    const glm::vec4 m = {
        std::min(slice.margins.x, r.width()  * 0.5f), std::min(slice.margins.y, r.height() * 0.5f),
        std::min(slice.margins.z, r.width()  * 0.5f), std::min(slice.margins.w, r.height() * 0.5f),
    };
    const glm::vec2 size = glm::max(slice.image.size, glm::vec2(1e-3f));

    // Column and row edges in the rect, and in the image (fractions)
    const float xs[4] = { r.min.x, r.min.x + m.x, r.max.x - m.z, r.max.x };
    const float ys[4] = { r.min.y, r.min.y + m.y, r.max.y - m.w, r.max.y };
    const float us[4] = { 0.0f, m.x / size.x, 1.0f - m.z / size.x, 1.0f };
    const float vs[4] = { 0.0f, m.y / size.y, 1.0f - m.w / size.y, 1.0f };

    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            const Rect cell = { { xs[i], ys[j] }, { xs[i + 1], ys[j + 1] } };
            if (cell.width() <= 0.0f || cell.height() <= 0.0f) continue;
            draw_image_(cell, slice.image.part({ us[i], vs[j], us[i + 1], vs[j + 1] }), tint);
        }
    }
}

float Context::draw_text(glm::vec2 pos, std::string_view text, const glm::vec4& color, float size) {
    if (!layout_.panel) return 0.0f;
    return draw_text_(pos, text, color, size);
}

float Context::text_width(std::string_view text, float size) const {
    return text_width_(text, size);
}

float Context::text_height(float size) const {
    if (!font_) return 0.0f;
    if (size <= 0.0f) return text_height_;
    return (font_->ascender() - font_->descender()) * size / font_->pixel_size();
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

    list_->push_back({ UIQuad{
        .pos_box      = box4(r),
        .color        = color,
        .border_color = border,
        .clip_box     = box4(clip_),
        .params       = { radius, thickness_(border_width), float(UIQuadShape::Box), 0.0f },
    } });
}

void Context::draw_shape_(const Rect& r, UIQuadShape shape, const glm::vec4& color,
                          float stroke)
{
    const Rect visible = r.intersect(clip_);
    if (visible.width() <= 0.0f || visible.height() <= 0.0f) return;

    list_->push_back({ UIQuad{
        .pos_box  = box4(r),
        .color    = color,
        .clip_box = box4(clip_),
        .params   = { 0.0f, stroke, float(shape), 0.0f },
    } });
}

void Context::draw_image_(const Rect& r, const Image& image, const glm::vec4& tint, float radius) {
    if (!image || tint.a <= 0.0f) return;
    const Rect visible = r.intersect(clip_);
    if (visible.width() <= 0.0f || visible.height() <= 0.0f) return;

    list_->push_back({ UIQuad{
        .pos_box  = box4(r),
        .tex_box  = image.tex_box,
        .color    = tint,
        .clip_box = box4(clip_),
        .params   = { radius, 0.0f, float(UIQuadShape::Image), float(image.layer) },
    }, image.material });
}

float Context::draw_text_(glm::vec2 pos, std::string_view text, const glm::vec4& color,
                          float size)
{
    if (!font_ || text.empty()) return 0.0f;
    if (size <= 0.0f) size = style_.font_size;
    const float text_scale = size / font_->pixel_size();

    const Render::TextStyle style = {
        .size   = size,
        .valign = Render::TextVAlign::Baseline,
    };

    // The outline (Style::text_outline_width, in points) in SDF value units:
    // points -> atlas pixels -> 1 / (2 * spread) per atlas pixel
    const bool  outlined = style_.text_outline_width > 0.0f && style_.text_outline.a > 0.0f;
    const float outline  = outlined
        ? style_.text_outline_width / text_scale / (2.0f * font_->sdf_spread()) : 0.0f;

    // The baseline on a whole pixel, so the bottoms of letters (and, at a
    // given size, their x-height and cap height) land the same way on every
    // line instead of straddling two pixels
    const float baseline = snap_(pos.y + font_->ascender() * text_scale);
    const float x        = snap_(pos.x);

    // layout_text works y up: lay out at -y, then negate back to y down
    const glm::vec4 bounds = Render::layout_text(*font_, text, { x, -baseline }, style,
        [&](const Render::FontGlyph& glyph, const glm::vec4& box) {
            const Rect r = { { box.x, -box.w }, { box.z, -box.y } };
            const Rect visible = r.intersect(clip_);
            if (visible.width() <= 0.0f || visible.height() <= 0.0f) return;

            list_->push_back({ UIQuad{
                .pos_box  = box4(r),
                .tex_box      = glyph.tex_box,
                .color        = color,
                .border_color = outlined ? style_.text_outline : glm::vec4(0.0f),
                .clip_box     = box4(clip_),
                .params       = { 0.0f, outline, float(UIQuadShape::Glyph), 0.0f },
            }, font_->material() });
        });

    return bounds.z - bounds.x;
}

float Context::text_width_(std::string_view text, float size) const {
    if (!font_ || text.empty()) return 0.0f;
    const float text_scale = size > 0.0f ? size / font_->pixel_size() : text_scale_;
    float w = 0.0f;
    for (char c : text) {
        if (const Render::FontGlyph* g = font_->glyph_or_fallback(c))
            w += g->advance * text_scale;
    }
    return w;
}

void Context::draw_text_in_(const Rect& r, std::string_view text, const glm::vec4& color,
                            float pad, bool center)
{
    const float y = r.min.y + snap_((r.height() - text_height_) * 0.5f);
    const float x = center ? snap_(r.center().x - text_width_(text) * 0.5f)
                           : r.min.x + pad;

    const Rect saved = clip_;
    clip_ = clip_.intersect(r);
    draw_text_({ x, y }, text, color);
    clip_ = saved;
}

void Context::submit_(const DrawList& list) {
    // UI pixels (y down) -> screen view (y up)
    const float h = screen_size_.y;
    for (const DrawItem& item : list) {
        // A new texture starts a new run, one draw order later, so the
        // renderer can't pull this run's quads in front of the last one's
        if (item.material != Render::Material::None && item.material != submit_material_) {
            submit_material_ = item.material;
            submit_order_++;
        }

        const Render::DrawParams params = {
            .view     = config_.view,
            .layer    = config_.layer,
            .order    = submit_order_,
            .material = submit_material_,
        };

        const UIQuad& q = item.quad;
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
            mouse_pixels_ = glm::vec2(e.pos_x, e.pos_y);
        }
    );

    listeners_[1] = events_.subscribe<OnMouseButtonDown>(
        [this](const OnMouseButtonDown& e) {
            if (e.button == MouseButton::Right) right_pressed_ = true;
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
                if (*c >= 32 && *c <= 126) text_in_.push_back({ ScanCode::Unknown, *c });
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
                    text_in_.push_back({ e.scancode, 0 });
                    break;
                default:
                    break;
            }
        }
    );
}

} // namespace Nezumi::UI
