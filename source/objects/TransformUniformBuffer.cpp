#include "TransformUniformBuffer.h"
#include <cstring>

TransformUniformBuffer::TransformUniformBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice,
                                               uint32_t MAX_FRAMES_IN_FLIGHT)
  : UniformBuffer(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(TransformUniform))
{}

void TransformUniformBuffer::update(uint32_t frame, TransformUniform& transformUniform)
{
  memcpy(uniformBuffersMapped[frame], &transformUniform, sizeof(transformUniform));
}
