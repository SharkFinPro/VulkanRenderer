#ifndef VKE_ASSETMANAGER_H
#define VKE_ASSETMANAGER_H

#include "../pipelines/implementations/vertexInputs/Vertex.h"
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vke {

  class Cloud;
  class DescriptorAllocator;
  class Font;
  class LogicalDevice;
  class Material;
  struct MeshCreateInfo;
  class Mesh;
  class Model;
  class RenderObject;
  class SmokeSystem;
  class Texture;
  class Texture2D;
  class UploadQueue;

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
    AssetManager(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<UploadQueue> uploadQueue);

    [[nodiscard]] std::shared_ptr<Texture2D> loadTexture(const char* path,
                                                         bool repeat = true);

    [[nodiscard]] std::shared_ptr<Model> loadModel(const char* path,
                                                   glm::vec3 rotation = { 0, 0, 0 });

    // Builds the same buffers and BLAS as loadModel from triangle data in memory. The data is copied, so the spans
    // need not outlive the call. Throws std::invalid_argument on empty input, an index count that is not a multiple
    // of three or an out-of-range index. Release it with release(std::shared_ptr<Model>) like any model.
    [[nodiscard]] std::shared_ptr<Model> createModel(std::span<const Vertex> vertices,
                                                     std::span<const uint32_t> indices);

    // A lightweight mesh uploaded through the upload queue (see UploadQueue). Never waits on the GPU or builds on the
    // frame's queue, so it may be called at any point in the frame. The copies are recorded at the start of the next
    // offscreen pass, within the per-frame upload budget, and the mesh is ready (Mesh::isReady) from the frame after;
    // drawing it before that is skipped. The data is copied. Throws std::invalid_argument on invalid input (see Mesh).
    // Release it with release(std::shared_ptr<Mesh>).
    [[nodiscard]] std::shared_ptr<Mesh> createMesh(const MeshCreateInfo& createInfo);

    [[nodiscard]] std::shared_ptr<RenderObject> loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                                 const std::shared_ptr<Texture2D>& specularMap,
                                                                 const std::shared_ptr<Model>& model);

    // Hands over the caller's reference. Nothing is destroyed here, so this is safe at any point in the frame: the
    // resource is destroyed after the last frame that could use it has finished on the GPU, once nothing else holds
    // it (a render object keeps its texture and model alive). Handles taken from it, such as an ImTextureID from
    // getImGuiTexture(), must not be used after the frame it is released in.
    void release(std::shared_ptr<RenderObject> renderObject);

    void release(std::shared_ptr<Model> model);

    void release(std::shared_ptr<Mesh> mesh);

    void release(std::shared_ptr<Texture> texture);

    void release(std::shared_ptr<Texture2D> texture);

    // Called by the engine at the end of each frame and before teardown. Hands every released resource nothing
    // else holds to the logical device's deferred destruction; returns whether it handed over any.
    bool destroyReleasedResources();

    void registerFont(std::string fontName,
                      std::string fontPath);

    [[nodiscard]] std::shared_ptr<Font> getFont(const std::string& fontName,
                                                uint32_t fontSize);

    [[nodiscard]] std::shared_ptr<SmokeSystem> createSmokeSystem(glm::vec3 position = glm::vec3(0.0f),
                                                                 uint32_t numParticles = 5'000'000);

    // The material set (set 1 of the object pipelines): a texture at binding 0 and a specular map at binding 1.
    [[nodiscard]] vk::DescriptorSetLayout getMaterialDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getFontDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getSmokeSystemDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getRayTracingDescriptorSetLayout() const;

    [[nodiscard]] std::shared_ptr<Cloud> createCloud();

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<UploadQueue> m_uploadQueue;

    vk::raii::CommandPool m_commandPool { nullptr };

    std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;

    vk::raii::DescriptorSetLayout m_materialDescriptorSetLayout = nullptr;

    vk::raii::DescriptorSetLayout m_fontDescriptorSetLayout = nullptr;

    vk::raii::DescriptorSetLayout m_smokeSystemDescriptorSetLayout = nullptr;

    vk::raii::DescriptorSetLayout m_rayTracingDescriptorSetLayout = nullptr;

    std::unordered_map<std::string, std::string> m_fontNames;
    std::unordered_map<FontKey, std::shared_ptr<Font>, FontKeyHash> m_fonts;

    std::vector<std::shared_ptr<void>> m_releasedResources;

    // Materials by texture and specular map, so render objects with the same textures share one. Weak, so a material
    // lives as long as a render object uses it; expired entries are pruned as the map grows.
    std::map<std::pair<const Texture*, const Texture*>, std::weak_ptr<Material>> m_materials;
    size_t m_materialPruneThreshold = 64;

    void createDescriptorSetLayouts();

    void createMaterialDescriptorSetLayout();

    void createFontDescriptorSetLayout();

    void createSmokeSystemDescriptorSetLayout();

    void createRayTracingDescriptorSetLayout();

    void loadFont(const std::string& fontName,
                  uint32_t fontSize);

    void createCommandPool();

    void queueRelease(std::shared_ptr<void> resource);

    [[nodiscard]] std::shared_ptr<Material> getMaterial(const std::shared_ptr<Texture2D>& texture,
                                                        const std::shared_ptr<Texture2D>& specularMap);
  };

} // namespace vke

#endif //VKE_ASSETMANAGER_H
