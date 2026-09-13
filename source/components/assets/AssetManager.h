#ifndef VKE_ASSETMANAGER_H
#define VKE_ASSETMANAGER_H

#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vke {

  class Cloud;
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

    [[nodiscard]] std::shared_ptr<Texture2D> loadTexture(const char* path,
                                                         bool repeat = true);

    [[nodiscard]] std::shared_ptr<Model> loadModel(const char* path,
                                                   glm::vec3 rotation = { 0, 0, 0 });

    [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                                 const std::shared_ptr<Texture2D>& specularMap,
                                                                 const std::shared_ptr<Model>& model);

    // Hands over the caller's reference. Nothing is destroyed here, so this is safe at any point in the frame: the
    // resource is destroyed at the end of a frame, after that frame's draws were submitted and the device was waited
    // on, once nothing else holds it (a render object keeps its texture and model alive). Handles taken from it, such
    // as an ImTextureID from getImGuiTexture(), must not be used after the frame it is released in.
    void release(std::shared_ptr<RenderObject> renderObject);

    void release(std::shared_ptr<Model> model);

    void release(std::shared_ptr<Texture> texture);

    void release(std::shared_ptr<Texture2D> texture);

    // Called by the engine at the end of each frame and before teardown.
    void destroyReleasedResources();

    void registerFont(std::string fontName,
                      std::string fontPath);

    [[nodiscard]] std::shared_ptr<Font> getFont(const std::string& fontName,
                                                uint32_t fontSize);

    [[nodiscard]] std::shared_ptr<SmokeSystem> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                                 uint32_t numParticles = 5'000'000);

    [[nodiscard]] vk::DescriptorSetLayout getObjectDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getFontDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getSmokeSystemDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getRayTracingDescriptorSetLayout() const;

    [[nodiscard]] std::shared_ptr<Cloud> createCloud();

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::raii::CommandPool m_commandPool { nullptr };

    std::vector<vk::raii::DescriptorPool> m_descriptorPools;
    uint32_t m_descriptorPoolSize = 500;
    uint32_t m_currentDescriptorPoolSize = 0;

    vk::raii::DescriptorSetLayout m_objectDescriptorSetLayout = nullptr;

    vk::raii::DescriptorSetLayout m_fontDescriptorSetLayout = nullptr;

    vk::raii::DescriptorSetLayout m_smokeSystemDescriptorSetLayout = nullptr;

    vk::raii::DescriptorSetLayout m_rayTracingDescriptorSetLayout = nullptr;

    std::unordered_map<std::string, std::string> m_fontNames;
    std::unordered_map<FontKey, std::shared_ptr<Font>, FontKeyHash> m_fonts;

    std::vector<std::shared_ptr<void>> m_releasedResources;

    void createDescriptorSetLayouts();

    void createObjectDescriptorSetLayout();

    void createFontDescriptorSetLayout();

    void createSmokeSystemDescriptorSetLayout();

    void createRayTracingDescriptorSetLayout();

    void loadFont(const std::string& fontName,
                  uint32_t fontSize);

    void createCommandPool();

    void createDescriptorPool();

    [[nodiscard]] vk::DescriptorPool getDescriptorPool();

    void queueRelease(std::shared_ptr<void> resource);
  };

} // namespace vke

#endif //VKE_ASSETMANAGER_H
