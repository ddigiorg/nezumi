// =============================================================================
// UI/Context.hpp
//
// Immediate-mode UI: panels (windows, menus, dialogs) and the widgets inside
// them, declared every frame in on_draw. A widget call draws the widget and
// returns what the player did with it this frame, so UI state never has to
// be synced with game state: the game's own variables are the state.
//
//   void on_draw(Engine& engine) override {
//       UI::Context& ui = *engine.ui();
//
//       if (show_debug_ && ui.begin_panel("Debug", &show_debug_, config)) {
//           ui.heading("Rendering");
//           ui.checkbox("show-world-grid", world_grid_);
//           if (ui.button("Restart")) restart_requested_ = true;
//           ui.slider("zoom", zoom_, 0.25f, 4.0f);
//           ui.end_panel();
//       }
//   }
//
// What is retained (by id, between frames): each panel's position, size,
// scroll and stacking order, which widget is being dragged, which text field
// has focus, and which dropdown is open. Everything else is rebuilt every
// frame, which costs a few microseconds for a menu and allocates nothing
// after the first frame.
//
// ## Ids
//
// A widget's id is a hash of its label, mixed with its panel's id, so labels
// must be unique within a panel. Text after "##" is part of the id but isn't
// shown: two "Delete##save1" and "Delete##save2" buttons both read "Delete".
// A panel's title works the same way, across all panels.
//
// ## Layout
//
// Widgets stack top to bottom. same_line() puts the next widget to the right
// of the previous one instead. Sliders, text fields and dropdowns draw the
// control first and their label to its right.
//
// ## Frame
//
// The Engine calls begin_frame() before on_draw and end_frame() after it, and
// end_frame() submits every panel back to front as one batch (see UIQuad.hpp).
// So all UI calls belong in on_draw. A button that changes the simulation
// should record the request and let on_tick act on it, the same rule as for
// the rest of the draw code.
//
// ## Input
//
// The UI reads raw input events itself and claims the mouse while the cursor
// is over a panel (or dragging one of its widgets), and the keyboard while a
// text field has focus. The InputMap then keeps those presses from the game's
// actions; see IInputCapture.hpp. Only the left mouse button interacts.
//
// ## Coordinates
//
// Screen pixels, top-left origin, y down, like every UI toolkit. The context
// flips to the renderer's y-up screen view when it submits.
//
// Panels stay inside the screen: dragging stops at its edges, and when the
// window shrinks, panels slide back on (and sized ones shrink, down to their
// min_size). Open dropdown lists stay on screen too.
//
// ## Not handled (yet)
//
// Keyboard and gamepad navigation, tooltips, tables, images, word wrap,
// clipboard and text selection, UTF-8 (the font covers ASCII), HiDPI scale.
// =============================================================================

#pragma once
#include "Nezumi/Core/CursorStyle.hpp"
#include "Nezumi/Event/EventListenerID.hpp"
#include "Nezumi/Input/IInputCapture.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include "Nezumi/Render/Kinds/UIQuad.hpp"
#include "Nezumi/UI/PanelConfig.hpp"
#include "Nezumi/UI/Rect.hpp"
#include "Nezumi/UI/Style.hpp"
#include "Nezumi/UI/UIConfig.hpp"
#include <algorithm>   // max
#include <array>       // array
#include <cmath>       // floor, round
#include <cstddef>     // size_t
#include <cstdint>     // uint32_t, uint64_t
#include <glm/glm.hpp> // vec2, vec4
#include <memory>      // unique_ptr
#include <span>        // span
#include <string>      // string
#include <string_view> // string_view
#include <vector>      // vector

namespace Nezumi {

class EventBus;
class Window;

namespace Render {
    class Font;
    class Renderer;
}

namespace UI {

class Context final : public IInputCapture {
public:

    using ID = std::uint32_t;

    // -------------------------------------------------------------------------
    // Construction & Destruction
    //
    // Loads the UI font (UIConfig::font_path) through the renderer. `window`
    // is optional: without it the cursor never changes and text input isn't
    // switched on.
    // -------------------------------------------------------------------------

    Context(
        EventBus&         events,
        Render::Renderer& renderer,
        Window*           window,
        const UIConfig&   config = {});

    ~Context() override;

    // Non-copyable and non-movable: listeners capture `this`
    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&)                 = delete;
    Context& operator=(Context&&)      = delete;

    // -------------------------------------------------------------------------
    // Frame (called by the Engine around on_draw)
    // -------------------------------------------------------------------------

    void begin_frame();
    void end_frame();

    // -------------------------------------------------------------------------
    // Panels
    //
    // begin_panel() returns false when the panel isn't shown (closed, or the
    // UI has no font). Call end_panel() only when it returned true:
    //
    //   if (ui.begin_panel("Settings", &open)) { ...; ui.end_panel(); }
    //
    // With `open`, the panel gets a close button that sets *open = false,
    // and a closed panel (*open == false) returns false. Panels can't nest.
    // -------------------------------------------------------------------------

    bool begin_panel(std::string_view title, bool* open, const PanelConfig& config = {});
    bool begin_panel(std::string_view title, const PanelConfig& config = {}) {
        return begin_panel(title, nullptr, config);
    }
    void end_panel();

    // -------------------------------------------------------------------------
    // Layout
    // -------------------------------------------------------------------------

    // Put the next widget to the right of the previous one
    void same_line(float spacing = -1.0f); // < 0: style.item_spacing.x

    // Empty space below the previous widget
    void spacing(float height = -1.0f);    // < 0: style.item_spacing.y

    // Horizontal line across the panel
    void separator();

    // -------------------------------------------------------------------------
    // Output Widgets
    // -------------------------------------------------------------------------

    void text   (std::string_view text);
    void text   (std::string_view text, const glm::vec4& color);
    void heading(std::string_view text); // style.heading color

    // -------------------------------------------------------------------------
    // Input Widgets
    //
    // Each returns true on the frame the player changed the value (or, for
    // button, clicked it).
    // -------------------------------------------------------------------------

    // width 0: fit the label, < 0: fill the rest of the line
    bool button(std::string_view label, float width = 0.0f);

    // Box with a check mark, label on the right
    bool checkbox(std::string_view label, bool& value);

    // On/off switch, label on the right
    bool toggle(std::string_view label, bool& value);

    // Drag (or click) anywhere on the track; the value is printed on it
    bool slider(std::string_view label, float& value, float min, float max,
                const char* format = "%.2f");
    bool slider(std::string_view label, int& value, int min, int max);

    // Single line. Focus by clicking; Enter, Escape or a click elsewhere
    // ends editing. Returns true on every edit. Printable ASCII only.
    bool text_field(std::string_view label, std::string& value,
                    std::size_t max_length = 256);

    // One choice from `items`; `selected` is an index into it (-1 = none)
    bool dropdown(std::string_view label, int& selected,
                  std::span<const std::string_view> items);

    // -------------------------------------------------------------------------
    // Input Capture (IInputCapture)
    //
    // Live: true while the cursor is over a panel or open dropdown, or a
    // widget is being dragged / a text field has focus. Usable from on_tick,
    // from event handlers, and from on_draw.
    // -------------------------------------------------------------------------

    bool wants_mouse   () const override;
    bool wants_keyboard() const override { return focus_id_ != 0; }

    // -------------------------------------------------------------------------
    // Cursor
    //
    // The cursor the game wants while the UI doesn't claim the mouse (e.g. a
    // crosshair over enemies). Kept until changed. Over a panel, the UI picks
    // its own (hand over buttons, I-beam over text fields, arrows on edges).
    // -------------------------------------------------------------------------

    void        set_cursor(CursorStyle style) { game_cursor_ = style; }
    CursorStyle get_cursor() const            { return game_cursor_;  }

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    bool              is_enabled () const { return font_ != nullptr; }
    Style&            style      ()       { return style_; }
    const Style&      style      () const { return style_; }
    Render::Font*     font       () const { return font_;  }
    glm::vec2         screen_size() const { return screen_size_; } // UI units (points)
    glm::vec2         mouse      () const { return mouse_; }       // UI units, y down

    // Screen pixels per UI unit: the display scale (1.5 at 150%). UI units
    // are points, so mouse() * scale() is the cursor in window pixels, as
    // Camera::screen_to_world takes it.
    float             scale      () const { return scale_; }

    // Panel bounds as last drawn, or an empty rect if it hasn't been
    Rect get_panel_rect(std::string_view title) const;

private:

    // -------------------------------------------------------------------------
    // Types
    // -------------------------------------------------------------------------

    using DrawList = std::vector<Render::UIQuad>;

    struct Panel {
        ID          id         = 0;
        std::string title      = {};   // shown part (before "##")
        glm::vec2   pos        = { 0.0f, 0.0f };
        glm::vec2   size       = { 0.0f, 0.0f };
        glm::vec2   content    = { 0.0f, 0.0f }; // content size measured last frame
        float       scroll     = 0.0f;
        PanelFlags  flags      = PanelFlags::None;
        glm::vec2   min_size   = { 0.0f, 0.0f };
        glm::vec2   anchor     = { 0.0f, 0.0f }; // PanelConfig::position
        glm::vec2   pivot      = { 0.0f, 0.0f };
        bool        place      = false; // apply anchor and pivot in end_panel
        bool        closable   = false;
        bool        close_hot  = false; // close button under the cursor
        std::uint64_t drawn    = 0;    // frame it was last begun

        DrawList    chrome     = {};   // background, title bar (drawn first)
        DrawList    body       = {};   // widgets

        Rect rect() const { return Rect::from_size(pos, size); }
        bool has(PanelFlags f) const { return has_any(flags, f); }
    };

    // Layout state of the panel being built
    struct Layout {
        Panel*    panel      = nullptr;
        Rect      content    = {};   // inside the padding, below the title
        Rect      clip       = {};
        glm::vec2 origin     = {};   // content top-left, scrolled
        glm::vec2 cursor     = {};   // where the next widget goes
        glm::vec2 prev_end   = {};   // top-right of the previous widget
        float     line_h     = 0.0f; // tallest widget on the current line
        float     max_x      = 0.0f; // content extents, for AutoSize and scrolling
        float     max_y      = 0.0f;
        bool      same_line  = false;
        float     same_space = 0.0f;
    };

    // Resize edges, as a bit mask
    enum Edge : std::uint8_t {
        EdgeNone   = 0,
        EdgeLeft   = 1u << 0,
        EdgeRight  = 1u << 1,
        EdgeTop    = 1u << 2,
        EdgeBottom = 1u << 3,
    };

    // -------------------------------------------------------------------------
    // Helpers: ids and panels (Context.cpp)
    // -------------------------------------------------------------------------

    static ID               hash_(std::string_view text, ID seed);
    static std::string_view visible_(std::string_view label); // before "##"

    ID     id_(std::string_view label) const;
    Panel* find_panel_(ID id) const;
    Panel* panel_at_(glm::vec2 p) const;  // topmost panel under p, as last drawn
    Rect   hit_rect_(const Panel& panel) const; // rect, plus the resize margin
    std::uint8_t edges_at_(const Panel& panel, glm::vec2 p) const;
    void   bring_to_front_(Panel& panel);
    void   update_panel_drag_(Panel& panel);
    void   clamp_to_screen_(Panel& panel) const;
    void   draw_chrome_(Panel& panel);
    float  title_height_(const Panel& panel) const;
    Rect   close_rect_(const Panel& panel) const;

    // -------------------------------------------------------------------------
    // Helpers: layout and interaction (Context.cpp)
    // -------------------------------------------------------------------------

    // Space for a widget of `size` at the layout cursor; advances the cursor.
    // A `fill` widget (separator, full-width button) takes the width it is
    // given but doesn't count toward an AutoSize panel's width.
    Rect   place_(glm::vec2 size, bool fill = false);
    float  frame_height_() const;
    float  control_width_() const;
    float  avail_width_() const; // from the cursor to the content's right edge
    float  label_width_(std::string_view visible) const; // 0 for an empty label
    void   draw_label_(const Rect& row, float x, std::string_view visible);

    // Mouse over `r`, in the current panel, nothing else in the way
    bool   hovered_(ID id, const Rect& r) const;

    // Press/release logic shared by clickable widgets: true when clicked
    // (pressed and released on it). `hovered` / `held` are optional outputs.
    bool   behavior_(ID id, const Rect& r, bool* hovered = nullptr, bool* held = nullptr);

    bool   slider_(std::string_view label, float& value, float min, float max,
                   bool integer, const char* format);

    // -------------------------------------------------------------------------
    // Helpers: pixel snapping
    //
    // UI units are points, scale_ pixels each. Positions and sizes snap to
    // whole screen pixels (k / scale_), so edges and text land on pixel
    // boundaries at any display scale, not just at 1x and 2x.
    // -------------------------------------------------------------------------

    float     snap_      (float v)     const { return std::round(v * scale_) / scale_; }
    glm::vec2 snap_      (glm::vec2 v) const { return glm::round(v * scale_) / scale_; }
    float     snap_floor_(float v)     const { return std::floor(v * scale_) / scale_; }
    // A line or border width: whole pixels, at least one (0 stays 0)
    float     thickness_ (float w)     const {
        return w > 0.0f ? std::max(1.0f, std::round(w * scale_)) / scale_ : 0.0f;
    }

    // -------------------------------------------------------------------------
    // Helpers: drawing, into list_ and clipped to clip_ (Context.cpp)
    // -------------------------------------------------------------------------

    void  draw_box_  (const Rect& r, const glm::vec4& color, float radius = 0.0f,
                      const glm::vec4& border = {}, float border_width = 0.0f);
    void  draw_shape_(const Rect& r, Render::UIQuadShape shape, const glm::vec4& color,
                      float stroke = 0.0f);
    // Text whose first line's top is at `pos`; returns its width
    float draw_text_ (glm::vec2 pos, std::string_view text, const glm::vec4& color);
    float text_width_(std::string_view text) const;
    // Text vertically centered in `r`, starting at r.min.x + pad, clipped to r
    void  draw_text_in_(const Rect& r, std::string_view text, const glm::vec4& color,
                        float pad, bool center = false);

    void  submit_(const DrawList& list);

    // -------------------------------------------------------------------------
    // Helpers: events (Context.cpp)
    // -------------------------------------------------------------------------

    void subscribe_();

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    EventBus&         events_;
    Render::Renderer& renderer_;
    Window*           window_ = nullptr;
    UIConfig          config_;
    Style             style_;
    Render::Font*     font_   = nullptr;

    // Font metrics at style_.font_size, refreshed each frame
    float text_scale_  = 0.0f; // font pixels -> UI pixels
    float text_height_ = 0.0f; // ascender to descender

    // Panels, in stacking order (back to front)
    std::vector<std::unique_ptr<Panel>> panels_ = {};
    Layout   layout_  = {};
    DrawList overlay_ = {};    // open dropdown, above every panel

    // Where draw_* helpers write and what they clip to
    DrawList* list_ = nullptr;
    Rect      clip_ = {};

    // Interaction state (retained)
    ID           hot_panel_    = 0; // panel under the cursor this frame
    ID           front_panel_  = 0; // last panel brought to the front
    ID           active_id_    = 0; // widget or panel part being held
    ID           focus_id_     = 0; // text field being edited
    bool         focus_seen_   = false;
    std::size_t  caret_        = 0;
    float        field_scroll_ = 0.0f;

    // Panel move / resize drag
    ID           drag_panel_  = 0;
    std::uint8_t drag_edges_  = EdgeNone; // EdgeNone = moving
    glm::vec2    drag_mouse_  = {};
    glm::vec2    drag_pos_    = {};
    glm::vec2    drag_size_   = {};

    // Open dropdown: its id, where its list was drawn, and the box that
    // opened it (a click there closes it instead of reopening it)
    ID   popup_id_    = 0;
    Rect popup_rect_  = {};
    Rect popup_owner_ = {};
    bool popup_drawn_ = false;
    bool on_popup_    = false; // cursor over the open dropdown this frame

    // Input gathered from events since the last frame
    glm::vec2         mouse_          = { -1.0f, -1.0f }; // UI units
    glm::vec2         mouse_pixels_   = { -1.0f, -1.0f }; // window pixels, from events
    bool              mouse_down_     = false;
    bool              mouse_pressed_  = false;
    bool              mouse_released_ = false;
    float             wheel_          = 0.0f;
    std::string       typed_          = {};
    std::vector<ScanCode> keys_       = {};   // editing keys, with repeats

    CursorStyle ui_cursor_   = CursorStyle::Arrow;
    CursorStyle game_cursor_ = CursorStyle::Arrow;

    glm::vec2     screen_size_ = { 0.0f, 0.0f };
    float         scale_       = 1.0f; // screen pixels per UI unit
    std::uint64_t frame_       = 0;
    bool          in_frame_    = false;

    std::array<EventListenerID, 6> listeners_ = {};
};

} // namespace UI
} // namespace Nezumi
