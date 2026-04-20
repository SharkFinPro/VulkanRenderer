#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include "RayTracer.h"
#include "Renderer3DPushConstants.h"
#include "../../pipelines/implementations/common/PipelineTypes.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vke {
  class Cloud;

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
  class SmokeVolume;
  class Texture3D;
  class TextureCubemap;
  class Window;

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
    vk::ShaderStageFlags stages;
  };

  class Renderer3D {
  public:
    Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
               std::shared_ptr<AssetManager> assetManager,
               std::shared_ptr<Window> window);

    void updateLightingManager(const std::shared_ptr<LightingManager>& lightingManager,
                               uint32_t currentFrame) const;

    void renderShadowMaps(const std::shared_ptr<LightingManager>& lightingManager,
                          const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          uint32_t currentFrame) const;

    void renderMousePicking(const RenderInfo* renderInfo,
                            const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void handleRenderedMousePickingImage(vk::Image image) const;

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager,
                const std::shared_ptr<LightingManager>& lightingManager);

    void doRayTracing(const RenderInfo* renderInfo,
                      const std::shared_ptr<PipelineManager>& pipelineManager,
                      const std::shared_ptr<LightingManager>& lightingManager,
                      const ImageResource& imageResource) const;

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

    [[nodiscard]] const std::vector<std::shared_ptr<SmokeSystem>>& getSmokeSystems() const;

    [[nodiscard]] vk::DescriptorSetLayout getNoiseDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getCubeMapDescriptorSetLayout() const;

    void setCloudToRender(std::shared_ptr<Cloud> cloud);

    void renderSmokeVolume(std::shared_ptr<SmokeVolume> smokeVolume);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<AssetManager> m_assetManager;

    vk::raii::CommandPool m_commandPool = nullptr;

    vk::raii::DescriptorPool m_descriptorPool = nullptr;

    std::shared_ptr<MousePicker> m_mousePicker;

    bool m_shouldRenderGrid = true;

    glm::vec3 m_viewPosition{};
    glm::mat4 m_viewMatrix{};

    std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;
    std::vector<std::shared_ptr<RenderObject>> m_renderObjectsToRenderFlattened;

    std::vector<LineVertex> m_lineVerticesToRender;

    std::vector<BendyPlant> m_bendyPlantsToRender;

    std::vector<std::shared_ptr<SmokeSystem>> m_smokeSystemsToRender;
    std::vector<std::shared_ptr<SmokeVolume>> m_smokeVolumesToRender;

    std::shared_ptr<Texture3D> m_noiseTexture;

    std::shared_ptr<TextureCubemap> m_cubeMapTexture;

    std::shared_ptr<DescriptorSet> m_noiseDescriptorSet;

    std::shared_ptr<DescriptorSet> m_cubeMapDescriptorSet;

    std::unordered_map<PipelineType, PushConstantEntry> m_pushConstants = {
      { PipelineType::magnifyWhirlMosaic,  { MagnifyWhirlMosaicPushConstant{},  vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::ellipticalDots,      { EllipticalDotsPushConstant{},      vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::crosses,             { CrossesPushConstant{},             vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::curtain,             { CurtainPushConstant{},             vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::bumpyCurtain,        { BumpyCurtainPushConstant{},        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::snake,               { SnakePushConstant{},               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::noisyEllipticalDots, { NoisyEllipticalDotsPushConstant{}, vk::ShaderStageFlagBits::eFragment } },
      { PipelineType::cubeMap,             { CubeMapPushConstant{},             vk::ShaderStageFlagBits::eFragment } },
    };

    std::unique_ptr<RayTracer> m_rayTracer;

    std::shared_ptr<Cloud> m_cloudToRender;

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
  };
} // vke

#endif //VULKANPROJECT_RENDERER3D_H