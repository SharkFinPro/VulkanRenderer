#ifndef VKE_ENGINECONFIG_H
#define VKE_ENGINECONFIG_H

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
      bool fullscreen = false;
      bool resizable = true;
    } window;

    struct Device {
      // Case-insensitive substring match against VkPhysicalDeviceProperties::deviceName
      // (e.g. "RTX", "Radeon", "Intel"). When it matches a suitable device, that device is
      // used outright; when it matches nothing suitable, a warning is printed and the
      // highest-scoring device is used instead. Empty means automatic selection.
      std::string preferredName;
    } device;

    struct Camera {
      glm::vec3 position = glm::vec3{ 0.0f };
      float speed = 1.0f;
    } camera;

    struct ImGui {
      bool useDockspace = true;
      std::string sceneViewName = "Scene View";
      uint32_t maxTextures = 5;
      std::function<void()> styleSetup;
    } imGui;

    struct Rendering {
      // Initial ray tracing state; only takes effect when the hardware supports ray tracing.
      // Can be changed at runtime via RenderingManager::enableRayTracing()/disableRayTracing().
      bool rayTracingEnabled = true;
    } rendering;
  };

} // namespace vke

#endif //VKE_ENGINECONFIG_H
