#ifndef VKE_ASSETMANAGER_H
#define VKE_ASSETMANAGER_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>

namespace vke {

class Font;
class LogicalDevice;
class Model;
class RenderObject;
class Texture;
class Texture2D;

class AssetManager {
public:
  AssetManager(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool);

  ~AssetManager();

  [[nodiscard]] std::shared_ptr<Texture2D> loadTexture(const char* path, bool repeat = true);

  [[nodiscard]] std::shared_ptr<Model> loadModel(const char* path, glm::vec3 rotation = { 0, 0, 0 });

  [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                               const std::shared_ptr<Texture2D>& specularMap,
                                                               const std::shared_ptr<Model>& model);

  [[nodiscard]] std::shared_ptr<Font> loadFont(std::string path, uint32_t fontSize);

  [[nodiscard]] VkDescriptorSetLayout getObjectDescriptorSetLayout() const;

  [[nodiscard]] VkDescriptorSetLayout getFontDescriptorSetLayout() const;

private:
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorSetLayout m_fontDescriptorSetLayout = VK_NULL_HANDLE;

  std::vector<std::shared_ptr<Texture>> m_textures;
  std::vector<std::shared_ptr<Model>> m_models;
  std::vector<std::shared_ptr<RenderObject>> m_renderObjects;

  void createDescriptorSetLayouts();

  void createObjectDescriptorSetLayout();

  void createFontDescriptorSetLayout();
};

} // namespace vke

#endif //VKE_ASSETMANAGER_H
