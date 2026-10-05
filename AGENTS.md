# AGENTS.md

> **Living document.** Keep this current. When architecture, conventions, workflows, dependencies, or
> repository structure change, update this file in the same change. Keep it concise (target 2–5 pages) —
> summarize systems and link to code; don't duplicate what file names, CMake, or source already make obvious.
> Read this before making changes.

## Project Overview

- **VulkanRenderer** is a custom real-time 3D (and some 2D) rendering engine written in modern C++ (C++23) on the Vulkan API.
- The deliverable is a reusable library, **`VulkanEngine`** (CMake target/project name). All engine code lives in the `vke` namespace.
- Vulkan is used via **vulkan-hpp RAII** (`vulkan/vulkan_raii.hpp`), with dynamic dispatch loader and `VULKAN_HPP_NO_STRUCT_CONSTRUCTORS`.
- No standalone application ships from the engine. Executables are small **test apps** under `tests/*` that drive the library.
- Scope today: rendering experiments and feature validation (lighting/shadows, ray tracing, mouse picking, cubemaps, particle systems/smoke, procedural plants, clouds, fonts, 2D). It is a personal/learning engine, not yet a product runtime.

## Repository Structure

| Path | Responsibility |
|------|----------------|
| `CMakeLists.txt` (root) | Top-level config, output dirs (`bin/`), RPATH, copies `tests/assets/`. Builds tests automatically when this is the top-level project. |
| `source/` | The `VulkanEngine` library. The only place engine code lives. |
| `source/VulkanEngine.{h,cpp}` | Engine facade: owns all subsystems, exposes accessors, drives the frame. |
| `source/EngineConfig.h` | `EngineConfig` struct — window/camera/ImGui/rendering startup configuration (e.g. initial ray tracing state). |
| `source/components/` | The engine subsystems (see Architecture). Each subsystem is its own subdirectory. |
| `source/components/pipelines/` | Pipeline infrastructure (`Pipeline`, `GraphicsPipeline`, `ComputePipeline`, `RayTracingPipeline`), descriptor sets, uniform buffers, shader modules, `PipelineManager`. |
| `source/components/pipelines/implementations/` | Concrete pipelines + shared `PipelineTypes.h` (the `PipelineType` enum) and vertex input definitions. |
| `source/utilities/` | Low-level, dependency-light helpers: `Buffers`, `Images`, `EventSystem`. |
| `source/shaders/` | GLSL shaders, compiled to SPIR-V at build time (see Build System). |
| `source/assets/` | Engine-bundled assets (noise, cubemap, bendy data). |
| `source/cmake/` | Modular CMake includes (Sources, Dependencies, External, CompileShaders, Assets, Headers). |
| `tests/` | Test/demo executables, one subdirectory per app. `tests/common/` shared GUI helpers, `tests/assets/` runtime assets. |
| `.github/workflows/` | `cmake-multi-platform.yml` CI. |

## Build System

- **CMake ≥ 3.27**, C++23. Built as a **shared library** (`BUILD_SHARED_LIBS ON`); on MSVC, `WINDOWS_EXPORT_ALL_SYMBOLS`.
- Layering: root → `add_subdirectory(source)` then (conditionally) `add_subdirectory(tests)`.
- `VulkanProject_BUILD_TESTS` defaults OFF, but is forced ON when this repo is the top-level project. So building standalone builds the tests; consuming via FetchContent does not.
- Source files are enumerated explicitly in `source/cmake/Sources.cmake` (grouped lists, not globs) with an existence-check validation loop. **Add new engine files there.**
- Dependencies are fetched with `FetchContent` in `source/cmake/External.cmake`: GLFW 3.4, GLM 1.0.1, Assimp 6.0.2, stb (master), ImGui (docking), FreeType 2.14.1. Vulkan is `find_package`d. `imgui` is compiled as a local static target with the GLFW+Vulkan backends.
- Shaders: `CompileShaders.cmake` globs `source/shaders/**` and compiles each to `bin/assets/shaders/...spv` via `glslangValidator --target-env vulkan1.2` (include dir `shaders/include`); the `Shaders` target is a build dependency of `VulkanEngine`. `*.spv` is git-ignored.
- Assets are copied to `bin/assets/` at configure time (`Assets.cmake`, plus root copying `tests/assets/`). Runtime asset paths are relative (e.g. `assets/textures/white.png`), so executables must run from `bin/`.
- `Headers.cmake` copies public headers into `include/VulkanEngine/` for external (FetchContent) consumers.
- **Dependency direction (must hold):** `tests/*` → `VulkanEngine`; within the library, `components/*` may use `utilities/*`; `utilities/*` depends on nothing engine-specific. Tests never depend on each other; engine never depends on tests.

## Architecture Overview

**Engine facade** — `VulkanEngine` owns and wires the subsystems (all `shared_ptr`), exposes a subset via getters (`getAssetManager`, `getCamera`, `getImGuiInstance`, `getLightingManager`, `getRenderingManager`, `getWindow`), and runs the loop via `isActive()` / `render()`.

**Core / device layer** — `Instance` (+ `DebugMessenger`), `Surface`, `Window` (GLFW), `PhysicalDevice`, `LogicalDevice`. These form the Vulkan context the rest of the engine builds on. Closing is a request, not a command: the window system's close button (and Escape, only when `EngineConfig::Window::closeOnEscape` is set) emits `CloseRequestEvent`, and a listener can veto with `Window::cancelClose()`.

**Rendering** — `RenderingManager` orchestrates per-frame work: an offscreen pass rendered into a `RenderTarget`/`ImageResource`, presented into an ImGui "Scene View" dock, plus the swapchain pass. `isSceneFocused()` gates camera movement, while rotation follows a right-drag that started over the scene view (`isSceneRightDragged()`; a right press there also focuses the scene, since ImGui focuses on left presses only); `isSceneHovered()` gates the mouse wheel and mouse picking, so scrolling another panel doesn't zoom the camera and a window covering the scene doesn't pick through it. An invisible button over the scene image takes left presses, so dragging over a detached scene view reaches the scene instead of moving the window; items an application submits from the overlay may overlap it. `setSceneOverlay` lets an application draw its own manipulators over the scene each frame, right after the scene image is drawn (the image is recorded before the application's own GUI, so nothing else could otherwise land on top of it); `getSceneViewRect` exposes where that image sits in ImGui screen coordinates for mapping the cursor onto it. With the dockspace the offscreen images are sized in framebuffer pixels (the content region times the scene window's viewport `FramebufferScale`), while the scene rect, the overlay and the cursor stay in ImGui units. Without the dockspace the scene fills the window and is blitted straight to the swapchain, so the rect there is the main viewport's position and size (not the swapchain extent, which is framebuffer pixels rather than the screen coordinates the rect and the cursor share), and the overlay is drawn on the background draw list instead of a window's. Mouse picking (`MousePicker`) draws each pickable object's 1-based id, `gl_PrimitiveID` and depth into an `R32G32B32A32Uint` identifier buffer and reads back the one pixel under the cursor, rather than casting a ray query: it works with ray tracing off or unsupported and matches exactly what was rasterized (a ray query could be added later as an alternative backend). `Renderer3D::getPickResult()` returns a `PickResult` (object, triangle index, world position unprojected from the depth with the picking pass's own matrices, depth), or `nullopt` over empty space, off or under a covered scene, or once the object is destroyed. `renderObject(..., &flag)` still sets a bool; `renderPickableObject` makes an object pickable without one. It delegates to `Renderer3D` (render objects grouped by `PipelineType`, shadow maps, mouse picking, grid, lines, smoke, plants, clouds, optional ray tracing) and `Renderer2D`. The camera's field of view and clip planes are set with `Renderer3D::setProjectionParameters` and travel in `RenderInfo`, whose `getProjectionMatrix()` is the camera projection for raster and mouse picking; ray tracing takes its field of view and clip planes, cutting camera rays at the planes' view-space depth (lights have their own projections). `SwapChain` and `CommandBuffer`/`SingleUseCommandBuffer` support it. `FrameScheduler` (owned by `RenderingManager`, also used by `ComputingManager` and the engine loop) owns all frame synchronization: one timeline semaphore paces the CPU and orders the per-frame compute → offscreen → swapchain submissions; binary semaphores are used only for WSI acquire/present. While the main window is minimized, frames skip the swapchain pass and end with `submitFrameEndWithoutSwapchain`: detached windows (possibly the scene view) keep rendering, and with none on screen the loop waits for window events instead. Barriers use Synchronization2 (`vk::ImageMemoryBarrier2` via `CommandBuffer::pipelineBarrier`).

**Pipelines** — `PipelineManager` builds and owns pipelines keyed by the `PipelineType` enum (`PipelineTypes.h`), and exposes bind/push-constant/descriptor helpers. Concrete pipelines live in `pipelines/implementations/`. Push-constant payloads are modeled as a `std::variant` per pipeline type in `Renderer3D`.

**Resources / assets** — `AssetManager` loads and owns textures (`Texture2D/3D/Cubemap/Glyph`), models (`Model`, via Assimp), `RenderObject`s, fonts (`Font`), smoke systems, and clouds; it manages descriptor set layouts and a pooled descriptor allocator. `AssetManager::release()` takes a render object, model or texture and destroys it at the end of a frame, after that frame's draws are submitted and the device is waited on, once nothing else holds it; descriptor sets are not yet returned to their pools. `utilities/Buffers` and `utilities/Images` are the low-level VkBuffer/VkImage helpers.

**Lighting** — `LightingManager` owns `PointLight`/`SpotLight` (`Light` base) and drives shadow-map data.

**Compute** — `ComputingManager` handles compute-pipeline dispatch (e.g. particle/smoke systems).

**UI** — `ImGuiInstance` integrates Dear ImGui (docking) with a configurable dockspace and scene-view window. Docking ignores ImGui size constraints, so `setDockedWindowMinimumSize` floors are enforced by clamping dock node sizes right after each frame's dock layout. With the dockspace and `EngineConfig::ImGui::detachableWindows` (on by default), windows dragged out of the main window become OS windows through ImGui's multi-viewport support: the ImGui GLFW and Vulkan backends own those windows and their swapchains, and they are submitted and presented in `ImGuiInstance::render`, before the swapchain pass is submitted. ImGui positions are then screen positions, not main-window positions. It stays off on Wayland and when presenting uses a queue family other than graphics. With it on, text follows the DPI of each window's monitor (`ConfigDpiScaleFonts`/`ConfigDpiScaleViewports`), while style sizes such as padding follow the main window's content scale, which ImGui can't yet apply per monitor. `ImGuiInstance::getViewportDpiScale` chooses each window's DPI with hysteresis, because rescaling a window straddling two monitors would otherwise flip its DPI every frame, and ImGui's windows are created without GLFW's scale-to-monitor hint so only ImGui resizes them. ImGui's own GLFW backend installs only its own key/scroll callbacks on a detached viewport's window (no chaining, unlike the main window's), so `ImGuiInstance` wraps `Platform_CreateWindow`/`Platform_DestroyWindow` to replace them with ones that call ImGui's handler and then `Window::handleSecondaryWindowKey`/`handleScroll`, and registers the window with `Window::addInputWindow` so `Window::buttonIsPressed` also polls it (mouse buttons need no callback: GLFW tracks per-window press state independent of one). GLFW never delivers a release for a window it is about to destroy, so `Window` records which input window last pressed a key and `removeInputWindow` synthesizes that key's release before the window goes away, the same fix ImGui's own backend applies to its internal state for the same reason. `MousePicker` reads the cursor from `ImGui::GetIO().MousePos`, an absolute screen coordinate once viewports are enabled, rather than the main window's own cursor position, and compares it against an equally absolute viewport position `RenderingManager` sets, then scales the offset by the picking image's extent over the size the image is displayed at, since window coordinates aren't pixels on high-density displays.

**Utilities** — header-only templated `EventSystem` (typed listener tuples; e.g. `FramebufferResizeEvent`; listeners may add or remove listeners during emission: removals take effect immediately, additions from the next emission); `Buffers`, `Images`.

## Development Principles

- **Namespace:** everything engine-side is in `vke`. Keep it that way.
- **Headers:** include guards are `#ifndef VKE_<NAME>_H` (newer files) — some older files still use `VULKANPROJECT_*`; prefer `VKE_*` for new files. Forward-declare engine types in headers and include in `.cpp` to keep build coupling low (widely used here).
- **Naming:** `PascalCase` types, `camelCase` methods/locals, `m_` member prefix, `PascalCase.cpp/.h` files matching the class. Enum values are `camelCase`.
- **Ownership:** subsystems are shared via `std::shared_ptr`; non-owning collaborators are passed by `const std::shared_ptr<T>&`. RAII (vulkan-hpp `vk::raii::*`) owns all Vulkan handles — avoid manual destroy.
- **`[[nodiscard]]`** on getters/queries is standard. Prefer `const` accessors.
- **Separation:** one class per subsystem directory; managers coordinate, components do the work. The `PipelineType` enum is the central key tying render requests to pipelines.
- **Source list discipline:** every new engine source/header must be registered in `Sources.cmake`.

## Test Applications

- Each subdirectory under `tests/` is one executable: a `CMakeLists.txt` (`project(name)` + `add_executable` linking `VulkanEngine`) and a `main.cpp`. Register new apps in `tests/CMakeLists.txt` (both `add_subdirectory` and the `build_all` target).
- **Focused feature tests** validate one system (`mousePicking`, `cubeMap`, `shadows`, `2D`, `crosses`, `magicLens`, `objectLoading`, `clouds`, `smoke`, `plants`, `renderObject`).
- **Integration tests** combine systems into a small interactive app (e.g. `snake`).
- Pattern (`main.cpp`): construct `vke::VulkanEngine` from an `EngineConfig`, set the ImGui context, load assets via `AssetManager`, then loop `while (renderer.isActive()) { ...build GUI, submit render requests..., renderer.render(); }`, all inside a try/catch on `std::exception`.
- Shared GUI helpers live in `tests/common/`. Tests include engine headers via repo-relative paths (`<source/...>`), enabled by the root `include_directories(<repo root>)`.
- Tests consume the public engine API only; they must not reach into engine internals or modify engine behavior to work.

## AI Agent Guidelines

- Read this file before changing anything.
- Understand existing patterns (the facade, manager/component split, `PipelineType` keying, RAII ownership, forward-declaration style) before introducing new ones.
- Preserve architectural consistency; prefer **extending existing systems over creating parallel ones**.
- Do not introduce new frameworks, dependencies, or large architectural rewrites without approval. Add dependencies only via `External.cmake`.
- Avoid speculative refactors. Keep changes scoped and incremental.
- When adding engine files, update `Sources.cmake`; when adding a test, update `tests/CMakeLists.txt`.
- Ask questions rather than assuming intent — especially on lifetime/ownership, threading, and long-term direction (see Open Questions, which the maintainer should resolve over time).
- Update this document when your understanding of the project meaningfully improves.

## Living Document Notice

- AGENTS.md is a **living document** and the primary onboarding reference for AI agents.
- Reflect significant architectural, structural, workflow, or convention changes here as they happen.
- Future contributors and agents should improve it as understanding grows — and keep it **concise and relevant** rather than letting it grow into a full architecture manual. For deep topics, add a dedicated doc and link it here instead of expanding this file.
