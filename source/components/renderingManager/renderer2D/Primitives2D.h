#ifndef VULKANPROJECT_PRIMITIVES2D_H
#define VULKANPROJECT_PRIMITIVES2D_H

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

struct Rect {
  glm::vec4 bounds;
  glm::vec4 color;
  glm::mat4 transform;
  float z;

  struct PushConstant {
    glm::mat4 transform;
    int screenWidth;
    int screenHeight;
    float z;
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float a;
  };

  [[nodiscard]] PushConstant createPushConstant(const VkExtent2D extent) const
  {
    return {
      .transform = transform,
      .screenWidth = static_cast<int>(extent.width),
      .screenHeight = static_cast<int>(extent.height),
      .z = z,
      .x = bounds.x,
      .y = bounds.y,
      .width = bounds.z,
      .height = bounds.w,
      .r = color.r,
      .g = color.g,
      .b = color.b,
      .a = color.a
    };
  }
};

#endif //VULKANPROJECT_PRIMITIVES2D_H