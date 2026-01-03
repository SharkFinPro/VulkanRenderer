#ifndef VKE_VULKANENGINEOPTIONS_H
#define VKE_VULKANENGINEOPTIONS_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <string>

namespace vke {

  struct VulkanEngineOptions {
    struct window {
      uint32_t width = 1280;
      uint32_t height = 720;
      std::string title = "Vulkan Engine";
      bool fullscreen = false;
      bool resizable = true;
    };

    struct camera {
      glm::vec3 position = glm::vec3{ 0.0f };
      float speed = 1.0f;
    };

    struct ImGui {
      bool useDockspace = true;
      std::string sceneViewName = "Scene View";
      uint32_t maxTextures = 5;
    };
  };

} // namespace vke

#endif //VKE_VULKANENGINEOPTIONS_H
