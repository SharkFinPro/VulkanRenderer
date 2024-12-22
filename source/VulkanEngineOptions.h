#ifndef VULKANPROJECT_VULKANENGINEOPTIONS_H
#define VULKANPROJECT_VULKANENGINEOPTIONS_H

#include <glm/vec3.hpp>

struct VulkanEngineOptions {
  uint32_t WINDOW_WIDTH;
  uint32_t WINDOW_HEIGHT;
  const char* WINDOW_TITLE;

  glm::vec3 CAMERA_POSITION = { 0.0f, 0.0f, 0.0f };
  float CAMERA_SPEED = 1.0f;

  bool FULLSCREEN = false;

  bool DO_DOTS = false;

  bool USE_DOCKSPACE = true;

  const char* SCENE_VIEW_NAME = "SceneView";

  uint32_t MAX_IMGUI_TEXTURES = 5;
};

#endif //VULKANPROJECT_VULKANENGINEOPTIONS_H
