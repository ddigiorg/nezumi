// =============================================================================
// SpriteSheet.cpp
// =============================================================================

#include "Nezumi/Render/2D/Sprite/SpriteSheet.hpp"

#include "Nezumi/Core/BasePath.hpp"
#include "Nezumi/Render/Core/Material/MaterialTable.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <algorithm>  // max, min
#include <cmath>      // floor
#include <filesystem> // path
#include <fstream>    // ifstream
#include <nlohmann/json.hpp>
#include <stdexcept>  // runtime_error
#include <utility>    // move, swap

namespace Nezumi::Render {

namespace {

// -----------------------------------------------------------------------------
// Manifest helpers
// -----------------------------------------------------------------------------

using Json = nlohmann::json;

[[noreturn]] void fail(const std::string& sheet, const std::string& what) {
    throw std::runtime_error("SpriteSheet '" + sheet + "': " + what);
}

glm::vec2 read_vec2(const Json& j, const std::string& sheet, const char* key) {
    if (!j.is_array() || j.size() != 2) fail(sheet, std::string("'") + key + "' must be [x, y]");
    return { j[0].get<float>(), j[1].get<float>() };
}

glm::vec4 read_vec4(const Json& j, const std::string& sheet, const char* key) {
    if (!j.is_array() || j.size() != 4) fail(sheet, std::string("'") + key + "' must be [x0, y0, x1, y1]");
    return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>() };
}

} // namespace

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

SpriteSheet::SpriteSheet(RenderContext& context, const SpriteSheetConfig& config)
    : context_(&context)
    , name_(config.name)
{
    std::vector<Image>       images;
    std::vector<Ktx2Texture> textures;
    load_manifest_(config, images, textures);
    if (!textures.empty()) create_(config, std::move(textures));
    else                   create_(config, images);
}

SpriteSheet::SpriteSheet(
    RenderContext&           context,
    const SpriteSheetConfig& config,
    const SpritePacker&      packer)
        : context_(&context)
        , name_(config.name)
{
    if (config.mip_levels == 0 || config.mip_levels > 16) fail(name_, "a packed sheet needs mip_levels in [1, 16]");
    SpritePacker::Packed packed = packer.pack(1u << (config.mip_levels - 1));

    regions_    = std::move(packed.regions);
    animations_ = std::move(packed.animations);
    for (std::size_t i = 0; i < packed.sprite_names.size(); ++i) {
        sprite_names_.emplace(std::move(packed.sprite_names[i]), static_cast<SpriteID>(i));
    }
    for (std::size_t i = 0; i < packed.animation_names.size(); ++i) {
        animation_names_.emplace(std::move(packed.animation_names[i]), static_cast<AnimationID>(i));
    }
    create_(config, packed.pages);
}

SpriteSheet::SpriteSheet(
    RenderContext&           context,
    const SpriteSheetConfig& config,
    std::span<const Image>   pages)
        : context_(&context)
        , name_(config.name)
{
    create_(config, pages);
}

SpriteSheet::SpriteSheet(
    RenderContext&           context,
    const SpriteSheetConfig& config,
    std::vector<Ktx2Texture> pages)
        : context_(&context)
        , name_(config.name)
{
    create_(config, std::move(pages));
}

SpriteSheet::~SpriteSheet() {
    remove_();
}

// -----------------------------------------------------------------------------
// Lookup
// -----------------------------------------------------------------------------

const SpriteID* SpriteSheet::find_sprite(std::string_view name) const {
    const auto it = sprite_names_.find(name);
    return it == sprite_names_.end() ? nullptr : &it->second;
}

const AnimationID* SpriteSheet::find_animation(std::string_view name) const {
    const auto it = animation_names_.find(name);
    return it == animation_names_.end() ? nullptr : &it->second;
}

SpriteID SpriteSheet::sprite(std::string_view name) const {
    if (const SpriteID* id = find_sprite(name)) return *id;
    fail(name_, "no sprite named '" + std::string(name) + "'");
}

AnimationID SpriteSheet::animation(std::string_view name) const {
    if (const AnimationID* id = find_animation(name)) return *id;
    fail(name_, "no animation named '" + std::string(name) + "'");
}

// -----------------------------------------------------------------------------
// Animation
// -----------------------------------------------------------------------------

SpriteSheet::Frame SpriteSheet::frame(AnimationID id, std::uint32_t heading, float seconds) const {
    const Animation& a = animations_[id];

    // Heading -> stored direction, mirrored past the halfway heading
    heading %= a.headings;
    Frame f;
    std::uint32_t direction = heading;
    if (a.mirrored() && heading > a.headings / 2) {
        direction  = a.headings - heading;
        f.mirrored = true;
    }

    // Time -> frame
    std::uint32_t index = 0;
    if (a.frames > 1 && a.fps > 0.0f && seconds > 0.0f) {
        const auto n = static_cast<std::uint64_t>(std::floor(seconds * a.fps));
        index = a.loop ? static_cast<std::uint32_t>(n % a.frames)
                       : static_cast<std::uint32_t>(std::min<std::uint64_t>(n, a.frames - 1));
    }

    f.sprite = a.first + direction * a.frames + index;
    return f;
}

// -----------------------------------------------------------------------------
// Drawing
// -----------------------------------------------------------------------------

Sprite SpriteSheet::sprite_at(SpriteID id, glm::vec2 anchor, float scale, bool mirrored) const {
    const SpriteRegion& r = regions_[id];

    // Stored box relative to the pivot, in pixels: x right, y down
    float left = r.offset.x - r.pivot.x;
    const float top = r.offset.y - r.pivot.y;
    if (mirrored) left = -(left + r.size.x);

    Sprite s;
    s.pos_box = {
        anchor.x + left * scale,
        anchor.y - (top + r.size.y) * scale,
        anchor.x + (left + r.size.x) * scale,
        anchor.y - top * scale,
    };
    s.tex_box = r.tex_box;
    if (mirrored) std::swap(s.tex_box.x, s.tex_box.z);
    s.layer = r.layer;
    return s;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void SpriteSheet::load_manifest_(
    const SpriteSheetConfig&  config,
    std::vector<Image>&       images,
    std::vector<Ktx2Texture>& textures)
{
    const std::filesystem::path path = BasePath::get(config.manifest);

    std::ifstream file(path);
    if (!file) fail(name_, "can't open manifest '" + path.string() + "'");

    Json j;
    try {
        j = Json::parse(file);
    } catch (const Json::exception& e) {
        fail(name_, "manifest '" + path.string() + "': " + e.what());
    }

    try {
        if (j.value("version", 0) != 1) fail(name_, "manifest version must be 1");

        // Pages
        const glm::vec2 page_size = read_vec2(j.at("page_size"), name_, "page_size");
        const auto& page_files = j.at("pages");
        if (!page_files.is_array() || page_files.empty()) fail(name_, "'pages' must list at least one image");

        // .ktx2 pages are used as stored (format, mips); anything else is an
        // image that gets a premultiplied mip chain on load. One kind per sheet.
        for (const Json& p : page_files) {
            const std::filesystem::path page_path = path.parent_path() / p.get<std::string>();
            std::uint32_t w = 0, h = 0;
            if (page_path.extension() == ".ktx2") {
                textures.push_back(load_ktx2(page_path.string()));
                if (textures.back().layers != 1) fail(name_, "page '" + page_path.string() + "' has more than one layer");
                w = textures.back().width;
                h = textures.back().height;
            } else {
                images.push_back(load_image(page_path.string()));
                w = images.back().width;
                h = images.back().height;
            }
            if (!images.empty() && !textures.empty()) fail(name_, "pages mix .ktx2 and image files");
            if (float(w) != page_size.x || float(h) != page_size.y) {
                fail(name_, "page '" + page_path.string() + "' is " + std::to_string(w) + "x" +
                            std::to_string(h) + ", not page_size");
            }
        }
        const auto page_count = static_cast<std::uint32_t>(images.size() + textures.size());

        // Sprites
        if (j.contains("sprites")) {
            for (const Json& s : j.at("sprites")) {
                const std::string name = s.at("name").get<std::string>();
                const auto page = s.at("page").get<std::uint32_t>();
                if (page >= page_count) fail(name_, "sprite '" + name + "' is on page " + std::to_string(page) + ", out of range");

                const glm::vec4 box  = read_vec4(s.at("box"), name_, "box");
                const glm::vec2 size = { box.z - box.x, box.w - box.y };
                if (size.x <= 0.0f || size.y <= 0.0f) fail(name_, "sprite '" + name + "' has an empty box");

                SpriteRegion r;
                r.tex_box = box / glm::vec4(page_size, page_size);
                r.size    = size;
                r.offset  = s.contains("offset") ? read_vec2(s["offset"], name_, "offset") : glm::vec2(0.0f);
                r.pivot   = s.contains("pivot")  ? read_vec2(s["pivot"],  name_, "pivot")
                                                 : r.offset + glm::vec2(size.x * 0.5f, size.y);
                r.layer   = page;

                const auto id = static_cast<SpriteID>(regions_.size());
                if (!sprite_names_.emplace(name, id).second) fail(name_, "duplicate sprite '" + name + "'");
                regions_.push_back(r);
            }
        }

        // Animations
        if (j.contains("animations")) {
            for (const Json& a : j.at("animations")) {
                const std::string name = a.at("name").get<std::string>();

                Animation anim;
                anim.first      = a.at("first").get<SpriteID>();
                anim.frames     = a.value("frames", 1u);
                anim.directions = a.value("directions", 1u);
                anim.headings   = a.value("headings", anim.directions);
                anim.fps        = a.value("fps", 0.0f);
                anim.loop       = a.value("loop", true);

                if (anim.frames == 0 || anim.directions == 0) fail(name_, "animation '" + name + "' has no frames");
                if (anim.mirrored() && anim.headings != 2 * (anim.directions - 1)) {
                    fail(name_, "animation '" + name + "': mirrored headings must be 2 * (directions - 1)");
                }
                const std::uint64_t last = std::uint64_t(anim.first) + std::uint64_t(anim.frames) * anim.directions;
                if (last > regions_.size()) fail(name_, "animation '" + name + "' runs past the last sprite");

                const auto id = static_cast<AnimationID>(animations_.size());
                if (!animation_names_.emplace(name, id).second) fail(name_, "duplicate animation '" + name + "'");
                animations_.push_back(anim);
            }
        }
    } catch (const Json::exception& e) {
        fail(name_, "manifest '" + path.string() + "': " + e.what());
    }
}

// RGBA8 page -> premultiplied mip chain of `levels` levels (see
// SpriteSheetConfig.hpp)
static Ktx2Texture chain_from_image(const Image& page, std::uint32_t levels) {
    Ktx2Texture t;
    t.format        = RHI::Format::RGBA8Unorm;
    t.width         = page.width;
    t.height        = page.height;
    t.premultiplied = true;
    t.levels.resize(levels);

    Image level = page;
    premultiply_alpha(level);
    for (std::uint32_t m = 0; m < levels; ++m) {
        if (m > 0) level = half_size(level);
        const auto bytes = std::as_bytes(std::span(level.pixels));
        t.levels[m].assign(bytes.begin(), bytes.end());
    }
    return t;
}

void SpriteSheet::create_(const SpriteSheetConfig& config, std::span<const Image> pages) {
    if (pages.empty()) fail(name_, "no pages");
    for (const Image& p : pages) {
        if (p.empty()) fail(name_, "empty page");
        if (p.width != pages[0].width || p.height != pages[0].height) fail(name_, "pages differ in size");
    }

    // Number of mip levels: 0 = the full chain, otherwise capped at it
    std::uint32_t full_chain = 1;
    for (std::uint32_t s = std::max(pages[0].width, pages[0].height); s > 1; s /= 2) ++full_chain;
    const std::uint32_t levels = (config.mip_levels == 0) ? full_chain : std::min(config.mip_levels, full_chain);

    std::vector<Ktx2Texture> chains;
    chains.reserve(pages.size());
    for (const Image& p : pages) chains.push_back(chain_from_image(p, levels));
    create_(config, std::move(chains));
}

void SpriteSheet::create_(const SpriteSheetConfig& config, std::vector<Ktx2Texture> pages) {
    RHI::Device& device = *context_->device;

    if (pages.empty()) fail(name_, "no pages");
    const Ktx2Texture& first = pages[0];
    for (const Ktx2Texture& p : pages) {
        if (p.width != first.width || p.height != first.height) fail(name_, "pages differ in size");
        if (p.format != first.format)                           fail(name_, "pages differ in format");
        if (p.levels.size() != first.levels.size())             fail(name_, "pages differ in mip levels");
        if (p.levels.empty())                                   fail(name_, "a page has no data");
        const bool has_alpha = RHI::get_format_info(p.format).channel_count == 4;
        if (has_alpha && !p.premultiplied) fail(name_, "RGBA pages must be premultiplied (see SpriteSheetConfig.hpp)");
    }

    // Logical size: what manifest pixel boxes and tex_box() refer to, even
    // when the largest levels are skipped
    page_width_  = first.width;
    page_height_ = first.height;

    // One layer per page. Skipping levels drops the largest ones (the low
    // res setting); UVs don't change.
    const auto     total = static_cast<std::uint32_t>(first.levels.size());
    const auto     skip  = std::min(config.skip_levels, total - 1);
    std::uint32_t  layers = 0;
    for (const Ktx2Texture& p : pages) layers += p.layers;
    pages_      = layers;
    mip_levels_ = total - skip;

    // A GPU that can't sample the format gets it decoded here instead
    const RHI::DeviceInfo& info = device.info();
    const bool bc7 = first.format == RHI::Format::BC7RGBAUnorm || first.format == RHI::Format::BC7RGBAUnormSrgb;
    if ((bc7 && !info.supports_bc7) || (first.format == RHI::Format::BC4RUnorm && !info.supports_bc4)) {
        for (Ktx2Texture& p : pages) p = decompress(std::move(p));
        decoded_on_cpu_ = true;
    }
    const RHI::Format format = pages[0].format;

    // Each level holds every layer back to back, as texture uploads expect
    std::vector<std::vector<std::byte>> levels(mip_levels_);
    for (std::uint32_t m = 0; m < mip_levels_; ++m) {
        for (const Ktx2Texture& p : pages) {
            levels[m].insert(levels[m].end(), p.levels[skip + m].begin(), p.levels[skip + m].end());
        }
    }

    texture_ = device.create(
        RHI::TextureConfig{
            .name            = name_,
            .type            = RHI::TextureType::Texture2DArray,
            .format          = format,
            .width           = first.level_width(skip),
            .height          = first.level_height(skip),
            .depth_or_layers = pages_,
            .mip_levels      = mip_levels_,
            .usage           = (mip_levels_ > 1)
                ? RHI::TextureUsageFlags::Sampled | RHI::TextureUsageFlags::CopyDst
                : RHI::TextureUsageFlags::Sampled,
            .initial_data    = std::span<const std::byte>(levels[0]),
        }
    );
    if (!texture_) {
        remove_();
        fail(name_, "texture creation failed (see log)");
    }

    for (std::uint32_t m = 1; m < mip_levels_; ++m) {
        device.write_texture(
            texture_,
            RHI::TextureRegion{ .mip_level = m, .depth_or_layers = pages_ }, // every layer of level m
            std::span<const std::byte>(levels[m])
        );
    }

    // Sampler
    sampler_ = device.create(
        RHI::SamplerConfig{
            .name           = name_,
            .min_filter     = config.min_filter,
            .mag_filter     = config.mag_filter,
            .mip_filter     = config.mip_filter,
            .address_u      = config.address_mode,
            .address_v      = config.address_mode,
            .address_w      = config.address_mode,
            .max_anisotropy = config.max_anisotropy,
            .lod_bias       = config.lod_bias,
        }
    );
    if (!sampler_) {
        remove_();
        fail(name_, "sampler creation failed (see log)");
    }

    // Material
    try {
        material_ = context_->materials->create(name_, texture_, sampler_);
    } catch (...) {
        remove_();
        throw;
    }
}

void SpriteSheet::remove_() {
    if (context_) {
        RHI::Device& device = *context_->device;
        context_->materials->remove(material_);
        device.remove(sampler_);
        device.remove(texture_);
    }
    context_  = nullptr;
    material_ = Material::None;
    texture_  = {};
    sampler_  = {};
}

} // namespace Nezumi::Render
