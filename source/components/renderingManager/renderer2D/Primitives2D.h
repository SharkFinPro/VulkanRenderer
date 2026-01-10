#ifndef VULKANPROJECT_PRIMITIVES2D_H
#define VULKANPROJECT_PRIMITIVES2D_H

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

namespace vke {

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

  struct Triangle {
    glm::vec2 p1;
    glm::vec2 p2;
    glm::vec2 p3;
    glm::vec4 color;
    glm::mat4 transform;
    float z;

    struct PushConstant {
      float r;
      float g;
      float b;
      float a;
      glm::mat4 transform;
      int screenWidth;
      int screenHeight;
      float z;
      float x1;
      float y1;
      float x2;
      float y2;
      float x3;
      float y3;
    };

    [[nodiscard]] PushConstant createPushConstant(const VkExtent2D extent) const
    {
      return {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
        .transform = transform,
        .screenWidth = static_cast<int>(extent.width),
        .screenHeight = static_cast<int>(extent.height),
        .z = z,
        .x1 = p1.x,
        .y1 = p1.y,
        .x2 = p2.x,
        .y2 = p2.y,
        .x3 = p3.x,
        .y3 = p3.y
      };
    }
  };

  struct Ellipse {
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

  struct Glyph {
    glm::vec4 bounds;
    glm::vec4 color;
    glm::mat4 transform;
    glm::vec4 uv;
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
      float u0, v0;
      float u1, v1;
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
        .u0 = uv.x,
        .v0 = uv.y,
        .u1 = uv.z,
        .v1 = uv.w,
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a
      };
    }
  };

} // vke

#endif //VULKANPROJECT_PRIMITIVES2D_H