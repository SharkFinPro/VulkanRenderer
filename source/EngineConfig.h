#ifndef VKE_ENGINECONFIG_H
#define VKE_ENGINECONFIG_H

#include "components/window/PresentMode.h"
#include <glm/vec3.hpp>
#include <cstdint>
#include <functional>
#include <string>

namespace vke {

  struct EngineConfig {
    struct Window {
      uint32_t width = 1280;
      uint32_t height = 720;
      std::string title = "Vulkan Engine";
      // Initial mode only; Window::setFullscreen() switches at runtime.
      bool fullscreen = false;
      bool resizable = true;
      // Escape raises a close request like the window system's close button, which listeners can veto. ImGui also
      // uses Escape to cancel text edits and popups, so applications with text input usually leave this off.
      bool closeOnEscape = false;
    } window;

    struct Camera {
      glm::vec3 position = glm::vec3{ 0.0f };
      float speed = 1.0f;
    } camera;

    struct ImGui {
      bool useDockspace = true;
      // With the dockspace, a window dragged outside the main window becomes its own OS window. Has no effect on Wayland,
      // where windows can't be positioned, or when the engine presents from a queue family other than graphics.
      bool detachableWindows = true;
      std::string sceneViewName = "Scene View";
      uint32_t maxTextures = 5;
      std::function<void()> styleSetup;
    } imGui;

    // Present mode and frame rate cap semantics: docs/FramePacing.md.
    struct Rendering {
      // Initial ray tracing state; only takes effect when the hardware supports ray tracing.
      // Can be changed at runtime via RenderingManager::enableRayTracing()/disableRayTracing().
      bool rayTracingEnabled = true;
      // Initial present mode preference; falls back per choosePresentMode when the surface lacks it.
      // RenderingManager::setPresentMode changes it at runtime.
      PresentMode presentMode = PresentMode::mailbox;
      // Frames per second; 0 = unlimited. RenderingManager::setFrameRateLimit changes it at runtime.
      double frameRateLimit = 0.0;
    } rendering;
  };

} // namespace vke

#endif //VKE_ENGINECONFIG_H
