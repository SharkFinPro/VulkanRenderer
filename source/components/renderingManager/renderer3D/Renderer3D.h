#ifndef VULKANPROJECT_RENDERER3D_H
#define VULKANPROJECT_RENDERER3D_H

#include "RayTracer.h"
#include "Renderer3DPushConstants.h"
#include "../../pipelines/implementations/common/PipelineTypes.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
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
  class Texture3D;
  class TextureCubemap;
  class UniformBuffer;

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
    // The mask holds a color's index plus one in 8 bits
    static constexpr uint32_t s_maxOutlineColors = 255;

    Renderer3D(std::shared_ptr<LogicalDevice> logicalDevice,
               std::shared_ptr<AssetManager> assetManager);

    void updateLightingManager(const std::shared_ptr<LightingManager>& lightingManager,
                               uint32_t currentFrame) const;

    void renderShadowMaps(const std::shared_ptr<LightingManager>& lightingManager,
                          const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const std::shared_ptr<PipelineManager>& pipelineManager,
                          uint32_t currentFrame) const;

    void renderMousePicking(const RenderInfo* renderInfo,
                            const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void handleRenderedMousePickingImage(vk::Image image) const;

    [[nodiscard]] bool hasOutlines() const;

    // Draws the objects submitted with renderOutline into the mask the outline composite reads, one value per color.
    void renderOutlineMask(const RenderInfo* renderInfo,
                           const std::shared_ptr<PipelineManager>& pipelineManager) const;

    // Draws the outlines around the masked objects. multisampled selects the pipeline for the scene pass (true) or
    // for a single-sample image without a depth attachment.
    void renderOutlines(const RenderInfo* renderInfo,
                        const std::shared_ptr<PipelineManager>& pipelineManager,
                        vk::DescriptorSet maskDescriptorSet,
                        bool multisampled) const;

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

    // Vertical field of view in degrees. Throws std::invalid_argument unless all values are finite, 0 < fov < 180 and
    // 0 < near < far. Raster geometry and ray-traced camera rays are both cut at nearPlane and farPlane.
    void setProjectionParameters(float fieldOfViewDegrees,
                                 float nearPlane,
                                 float farPlane);

    [[nodiscard]] std::shared_ptr<MousePicker> getMousePicker() const;

    [[nodiscard]] std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& getRenderObjectsToRender();

    void renderObject(const std::shared_ptr<RenderObject>& renderObject,
                      PipelineType pipelineType,
                      bool* mousePicked = nullptr);

    // Outlines the object for this frame in the given color (alpha included), on top of however the application draws
    // it. The outline also shows where other geometry hides the object, as the outline of a selection usually does.
    // It is drawn from the object's own draw call, so it follows whatever geometry the object draws. Throws
    // std::length_error beyond 255 different colors in a frame.
    void renderOutline(const std::shared_ptr<RenderObject>& renderObject, glm::vec4 color);

    // Outline width in pixels of the scene image, clamped to 1..8.
    void setOutlineWidth(float pixels);

    void renderLine(glm::vec3 start, glm::vec3 end);

    void renderBendyPlant(const BendyPlant& bendyPlant);

    void renderSmokeSystem(const std::shared_ptr<SmokeSystem>& smokeSystem);

    [[nodiscard]] const std::vector<std::shared_ptr<SmokeSystem>>& getSmokeSystems() const;

    [[nodiscard]] vk::DescriptorSetLayout getNoiseDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getCubeMapDescriptorSetLayout() const;

    [[nodiscard]] vk::DescriptorSetLayout getOutlineColorsDescriptorSetLayout() const;

    void setCloudToRender(std::shared_ptr<Cloud> cloud);

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<AssetManager> m_assetManager;

    vk::raii::CommandPool m_commandPool = nullptr;

    vk::raii::DescriptorPool m_descriptorPool = nullptr;

    std::shared_ptr<MousePicker> m_mousePicker;

    bool m_shouldRenderGrid = true;

    glm::vec3 m_viewPosition{};
    glm::mat4 m_viewMatrix{};

    float m_fieldOfView = 45.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1000.0f;

    std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>> m_renderObjectsToRender;
    std::vector<std::shared_ptr<RenderObject>> m_renderObjectsToRenderFlattened;

    // Each outlined object with its value in the mask, which is its color's index plus one (zero is no outline)
    std::vector<std::pair<std::shared_ptr<RenderObject>, uint32_t>> m_outlineObjects;
    std::vector<glm::vec4> m_outlineColors;
    float m_outlineWidth = 3.0f;

    std::vector<LineVertex> m_lineVerticesToRender;

    std::vector<BendyPlant> m_bendyPlantsToRender;

    std::vector<std::shared_ptr<SmokeSystem>> m_smokeSystemsToRender;

    std::shared_ptr<Texture3D> m_noiseTexture;

    std::shared_ptr<TextureCubemap> m_cubeMapTexture;

    std::shared_ptr<DescriptorSet> m_noiseDescriptorSet;

    std::shared_ptr<DescriptorSet> m_cubeMapDescriptorSet;

    std::shared_ptr<UniformBuffer> m_outlineColorsUniform;

    std::shared_ptr<DescriptorSet> m_outlineColorsDescriptorSet;

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