#ifndef VKE_ENGINECONFIG_H
#define VKE_ENGINECONFIG_H

#include <glm/vec3.hpp>
#include <cstdint>
#include <string>

namespace vke {

  struct EngineConfig {
    struct {
      uint32_t width = 1280;
      uint32_t height = 720;
      std::string title = "Vulkan Engine";
      bool fullscreen = false;
      bool resizable = true;
    } window;

    struct {
      glm::vec3 position = glm::vec3{ 0.0f };
      float speed = 1.0f;
    } camera;

    struct {
      bool useDockspace = true;
      std::string sceneViewName = "Scene View";
      uint32_t maxTextures = 5;
    } imGui;
  };

} // namespace vke

#endif //VKE_ENGINECONFIG_H
