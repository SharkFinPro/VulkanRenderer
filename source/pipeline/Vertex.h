#ifndef VULKANPROJECT_VERTEX_H
#define VULKANPROJECT_VERTEX_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

// TODO: Make class variables private

class Vertex {
public:
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription();

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};


#endif //VULKANPROJECT_VERTEX_H
