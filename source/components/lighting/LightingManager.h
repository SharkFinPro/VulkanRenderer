#ifndef LIGHTINGMANAGER_H
#define LIGHTINGMANAGER_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class DescriptorSet;
class Light;
class LogicalDevice;
class UniformBuffer;

class LightingManager {
public:
  LightingManager(const std::shared_ptr<LogicalDevice>& logicalDevice, VkDescriptorPool descriptorPool);

  [[nodiscard]] std::shared_ptr<Light> createLight(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular);

  void renderLight(const std::shared_ptr<Light>& light);

  [[nodiscard]] std::shared_ptr<DescriptorSet> getLightingDescriptorSet() const;

  void clearLightsToRender();

  void update(uint32_t currentFrame, glm::vec3 viewPosition);

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;

  std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
  std::shared_ptr<UniformBuffer> m_spotLightsUniform;
  std::shared_ptr<UniformBuffer> m_pointLightsUniform;
  std::shared_ptr<UniformBuffer> m_cameraUniform;

  int m_prevNumPointLights = 0;
  int m_prevNumSpotLights = 0;

  std::vector<std::shared_ptr<Light>> lights;

  std::vector<std::shared_ptr<Light>> pointLightsToRender;
  std::vector<std::shared_ptr<Light>> spotLightsToRender;

  void createUniforms();

  void createDescriptorSet(VkDescriptorPool descriptorPool);

  void updateUniforms(uint32_t currentFrame, glm::vec3 viewPosition);

  void updatePointLightUniforms(uint32_t currentFrame);

  void updateSpotLightUniforms(uint32_t currentFrame);
};



#endif //LIGHTINGMANAGER_H
