#ifndef VULKANPROJECT_VULKANENGINEOPTIONS_H
#define VULKANPROJECT_VULKANENGINEOPTIONS_H

#include <glm/vec3.hpp>

struct VulkanEngineOptions {
  uint32_t WINDOW_WIDTH;
  uint32_t WINDOW_HEIGHT;
  const char* WINDOW_TITLE;

  const char* VERTEX_SHADER_FILE;
  const char* FRAGMENT_SHADER_FILE;

  glm::vec3 cameraPosition = { 0.0f, 0.0f, 0.0f };
  float cameraSpeed = 1.0f;
};

#endif //VULKANPROJECT_VULKANENGINEOPTIONS_H
