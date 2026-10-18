// =============================================================================
// IApplication.hpp
//
// Application interface for use in Engine.
// =============================================================================

#pragma once
class Engine;

namespace Nezumi {

class IApplication {
public:
    virtual ~IApplication() = default;
    virtual void on_init(Engine&) = 0;
    virtual void on_tick(Engine&) = 0;
    virtual void on_draw(Engine&) = 0;
    virtual void on_exit(Engine&) = 0;
};

} // namespace Nezumi
