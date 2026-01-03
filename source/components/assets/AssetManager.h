#ifndef VKE_ASSETMANAGER_H
#define VKE_ASSETMANAGER_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vke {

  class Font;
  class LogicalDevice;
  class Model;
  class RenderObject;
  class SmokeSystem;
  class Texture;
  class Texture2D;

  struct FontKey {
    std::string name;
    uint32_t size;

    bool operator==(const FontKey& other) const
    {
      return name == other.name && size == other.size;
    }
  };

  struct FontKeyHash {
    std::size_t operator()(const FontKey& key) const
    {
      const std::size_t h1 = std::hash<std::string>{}(key.name);
      const std::size_t h2 = std::hash<uint32_t>{}(key.size);

      return h1 ^ (h2 << 1);
    }
  };

  class AssetManager {
  public:
    explicit AssetManager(std::shared_ptr<LogicalDevice> logicalDevice);

    ~AssetManager();

    [[nodiscard]] std::shared_ptr<Texture2D> loadTexture(const char* path,
                                                         bool repeat = true);

    [[nodiscard]] std::shared_ptr<Model> loadModel(const char* path,
                                                   glm::vec3 rotation = { 0, 0, 0 });

    [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                                 const std::shared_ptr<Texture2D>& specularMap,
                                                                 const std::shared_ptr<Model>& model);

    void registerFont(std::string fontName,
                      std::string fontPath);

    [[nodiscard]] std::shared_ptr<Font> getFont(const std::string& fontName,
                                                uint32_t fontSize);

    [[nodiscard]] std::shared_ptr<SmokeSystem> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                                 uint32_t numParticles = 5'000'000);

    [[nodiscard]] VkDescriptorSetLayout getObjectDescriptorSetLayout() const;

    [[nodiscard]] VkDescriptorSetLayout getFontDescriptorSetLayout() const;

    [[nodiscard]] VkDescriptorSetLayout getSmokeSystemDescriptorSetLayout() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_fontDescriptorSetLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_smokeSystemDescriptorSetLayout = VK_NULL_HANDLE;

    std::vector<std::shared_ptr<Texture>> m_textures;
    std::vector<std::shared_ptr<Model>> m_models;
    std::vector<std::shared_ptr<RenderObject>> m_renderObjects;

    std::unordered_map<std::string, std::string> m_fontNames;
    std::unordered_map<FontKey, std::shared_ptr<Font>, FontKeyHash> m_fonts;

    void createDescriptorSetLayouts();

    void createObjectDescriptorSetLayout();

    void createFontDescriptorSetLayout();

    void createSmokeSystemDescriptorSetLayout();

    void loadFont(const std::string& fontName,
                  uint32_t fontSize);

    void createCommandPool();

    void createDescriptorPool();
  };

} // namespace vke

#endif //VKE_ASSETMANAGER_H
