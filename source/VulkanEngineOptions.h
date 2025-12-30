#ifndef VKE_VULKANENGINEOPTIONS_H
#define VKE_VULKANENGINEOPTIONS_H

#include <glm/vec3.hpp>

namespace vke {

  struct VulkanEngineOptions {
    uint32_t WINDOW_WIDTH;
    uint32_t WINDOW_HEIGHT;
    const char* WINDOW_TITLE;

    glm::vec3 CAMERA_POSITION = { 0.0f, 0.0f, 0.0f };
    float CAMERA_SPEED = 1.0f;

    bool FULLSCREEN = false;

    bool USE_DOCKSPACE = true;

    const char* SCENE_VIEW_NAME = "SceneView";

    uint32_t MAX_IMGUI_TEXTURES = 5;
  };

} // namespace vke

#endif //VKE_VULKANENGINEOPTIONS_H
