# Nezumi Engine

<p align="center">
    <img src="assets/graphics/nezumi-logo.png" width="400" alt="Nezumi Engine Logo">
</p>

## Overview

**Nezumi** is a 2D (3D planned) game engine built using C++20 designed for personal hobby game projects. It currently features:

- **Event System**: Time-based publish-subscribe event bus  
- **Render Hardware Interface (RHI)**: Device abstraction layer (Backends: OpenGL; Vulkan and DX12 planned) 
- **Render**: Layered on the RHI:
  - **Render/Core**: shared by any renderer: frame recipe (passes), views, materials, shaders, instance streaming, fonts and text layout
  - **Render/2D**: `Renderer2D`, a feature renderer that loads atlases, sets up camera views, and automatically batch draws render objects (sprites, shapes, SDF text, etc.). On by default; opt out with `EngineConfig::enable_renderer2d`
  - **Render/3D**: planned
- **Controllers**: Input-driven gameplay helpers, e.g. an RTS-style camera controller (key/edge/drag pan, zoom to cursor)
- **UI**: Immediate-mode panels (movable, resizable, scrolling) and widgets (buttons, checkboxes, toggles, sliders, text fields, dropdowns), drawn in one batch, with cursor styles and input capture
- **Platform**: SDL3 for windowing, input, and application-level events
- **Entity System**: Supports simulation of ~20k active units at 60 FPS
- **World System**: Chunk-based sparse set of "Realms" (similar to Factorio surfaces)

Planned features and improvements include:

- **Audio System**: Sound effects and music
- **Entity Component System (ECS)**: Archetype and chunk based data-oriented entity framework
- **Physics**: AABB collision system
- **Serialization**: Saving and loading game data
- **Networking**: Determinitic lockstep for low-bandwidth multiplayer games
- **GUI Manager**: Menus and other UI elements

## Documentation

Please see [docs/getting-started.md](docs/getting-started.md) for instructions on how to use the Nezumi Engine.

## Examples

- [Scrapfall](https://elchupathingy.itch.io/scrapfall)
- Nightmourn (planned)

## License
 
Nezumi is free and open source under the [MIT License](LICENSE.txt).
