#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include "Renderer3DPushConstants.h"
#include "../../pipelines/implementations/common/PipelineTypes.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vke {

  class AssetManager;
  class CommandBuffer;
  class DescriptorSet;
  class ImageResource;
  class LightingManager;
  struct LineVertex;
  class LogicalDevice;
  class MousePicker;
  class PipelineManager;
  struct RenderInfo;
  class RenderObject;
  class SmokeSystem;
  class Texture3D;
  class TextureCubemap;
  class UniformBuffer;
  struct Vertex;
  class Window;

  struct MeshInfo {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t textureIndex;
    uint32_t specularIndex;
  };

  struct BendyPlant {
    glm::vec3 position = glm::vec3(0.0f);
    int numFins = 21;
    int leafLength = 3;
    float pitch = 77.5;
    float bendStrength = -0.07;
  };

  using PushConstantVariant = std::variant<
    MagnifyWhirlMosaicPushConstant,
    EllipticalDotsPushConstant,
    CrossesPushConstant,
    CurtainPushConstant,
    BumpyCurtainPushConstant,
    SnakePushConstant,
    NoisyEllipticalDotsPushConstant,
    CubeMapPushConstant
  >;

  struct PushConstantEntry {
    PushConstantVariant data;
    VkShaderStageFlags stages;
  };

  class Renderer3D {
  public:
    Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
               std::shared_ptr<AssetManager> assetManager,
               std::shared_ptr<Window> window);

    ~Renderer3D();

    void renderShadowMaps(const std::shared_ptr<LightingManager>& lightingManager,
                          const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          uint32_t currentFrame) const;

    void renderMousePicking(const RenderInfo* renderInfo,
                            const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void handleRenderedMousePickingImage(VkImage image) const;

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager,
                const std::shared_ptr<LightingManager>& lightingManager);

    void doRayTracing(const RenderInfo* renderInfo,
                      const std::shared_ptr<PipelineManager>& pipelineManager,
                      const std::shared_ptr<LightingManager>& lightingManager,
                      const std::shared_ptr<ImageResource>& imageResource);

    void createNewFrame();

    void enableGrid();

    void disableGrid();

    [[nodiscard]] bool isGridEnabled() const;

    void setCameraParameters(glm::vec3 position,
                             const glm::mat4& viewMatrix);

    [[nodiscard]] std::shared_ptr<MousePicker> getMousePicker() const;

    [[nodiscard]] std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& getRenderObjectsToRender();

    void renderObject(const std::shared_ptr<RenderObject>& renderObject,
                      PipelineType pipelineType,
                      bool* mousePicked = nullptr);

    void renderLine(glm::vec3 start, glm::vec3 end);

    void renderBendyPlant(const BendyPlant& bendyPlant);

    void renderSmokeSystem(const std::shared_ptr<SmokeSystem>& smokeSystem);

    [[nodiscard]] const std::vector<std::shared_ptr<SmokeSystem>>& getSmokeSystems() const;;

    [[nodiscard]] VkDescriptorSetLayout getNoiseDescriptorSetLayout() const;

    [[nodiscard]] VkDescriptorSetLayout getCubeMapDescriptorSetLayout() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<AssetManager> m_assetManager;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    std::shared_ptr<MousePicker> m_mousePicker;

    bool m_shouldRenderGrid = true;

    glm::vec3 m_viewPosition{};
    glm::mat4 m_viewMatrix{};

    std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;
    std::vector<std::shared_ptr<RenderObject>> m_renderObjectsToRenderFlattened;

    std::vector<LineVertex> m_lineVerticesToRender;

    std::vector<BendyPlant> m_bendyPlantsToRender;

    std::vector<std::shared_ptr<SmokeSystem>> m_smokeSystemsToRender;

    std::shared_ptr<Texture3D> m_noiseTexture;

    std::shared_ptr<TextureCubemap> m_cubeMapTexture;

    std::shared_ptr<DescriptorSet> m_noiseDescriptorSet;

    std::shared_ptr<DescriptorSet> m_cubeMapDescriptorSet;

    std::unordered_map<PipelineType, PushConstantEntry> m_pushConstants = {
      { PipelineType::magnifyWhirlMosaic,  { MagnifyWhirlMosaicPushConstant{},  VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::ellipticalDots,      { EllipticalDotsPushConstant{},      VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::crosses,             { CrossesPushConstant{},             VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::curtain,             { CurtainPushConstant{},             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::bumpyCurtain,        { BumpyCurtainPushConstant{},        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::snake,               { SnakePushConstant{},               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::noisyEllipticalDots, { NoisyEllipticalDotsPushConstant{}, VK_SHADER_STAGE_FRAGMENT_BIT } },
      { PipelineType::cubeMap,             { CubeMapPushConstant{},             VK_SHADER_STAGE_FRAGMENT_BIT } },
    };

    VkBuffer m_tlasInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_tlasInstanceBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_tlasBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_tlasBufferMemory = VK_NULL_HANDLE;

    VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;
    VkWriteDescriptorSetAccelerationStructureKHR m_tlasInfo{};

    std::shared_ptr<UniformBuffer> m_cameraUniformRT;

    std::shared_ptr<DescriptorSet> m_rayTracingDescriptorSet;

    VkBuffer m_mergedVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_mergedVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_mergedIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_mergedIndexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_meshInfoBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_meshInfoBufferMemory = VK_NULL_HANDLE;

    VkDescriptorBufferInfo m_vertexBufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo m_indexBufferInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };
    VkDescriptorBufferInfo m_meshInfoInfo = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };

    std::vector<VkDescriptorImageInfo> m_textureImageInfos;

    void createCommandPool();

    void createDescriptorPool();

    void renderRenderObjectsByPipeline(const RenderInfo* renderInfo,
                                       const std::shared_ptr<PipelineManager>& pipelineManager,
                                       const std::shared_ptr<LightingManager>& lightingManager) const;

    void renderSmokeSystems(const RenderInfo* renderInfo,
                            const std::shared_ptr<PipelineManager>& pipelineManager) const;

    static void renderGrid(const std::shared_ptr<PipelineManager>& pipelineManager,
                           const RenderInfo* renderInfo);

    void renderRenderObjects(const std::shared_ptr<PipelineManager>& pipelineManager,
                             const std::shared_ptr<LightingManager>& lightingManager,
                             const RenderInfo* renderInfo,
                             PipelineType pipelineType,
                             const std::vector<std::shared_ptr<RenderObject>>* objects) const;

    void bindPushConstant(const std::shared_ptr<PipelineManager>& pipelineManager,
                          const std::shared_ptr<CommandBuffer>& commandBuffer,
                          PipelineType pipelineType) const;

    void bindDescriptorSets(const std::shared_ptr<PipelineManager>& pipelineManager,
                            const std::shared_ptr<LightingManager>& lightingManager,
                            const std::shared_ptr<CommandBuffer>& commandBuffer,
                            PipelineType pipelineType,
                            uint32_t currentFrame) const;

    void createDescriptorSets();

    [[nodiscard]] bool pipelineIsActive(PipelineType pipelineType) const;

    void displayGui();

    void displayCrossesGui();

    void displayCurtainGui();

    void displayEllipticalDotsGui();

    void displayMiscGui();

    void createTLAS();

    void destroyTLAS();

    void updateRTSceneInfo();

    void uploadRTSceneInfoBuffers(const std::vector<Vertex>& mergedVertices,
                                  const std::vector<uint32_t>& mergedIndices,
                                  const std::vector<MeshInfo>& meshInfos);

    void updateRTDescriptorSets(const std::shared_ptr<ImageResource>& imageResource,
                                VkExtent2D extent,
                                uint32_t currentFrame);

    void updateRTDescriptorSetData(VkExtent2D extent,
                                   uint32_t currentFrame);
  };
} // vke

#endif //VULKANPROJECT_RENDERER3D_H