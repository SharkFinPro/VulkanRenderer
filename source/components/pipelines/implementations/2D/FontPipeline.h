#ifndef VULKANPROJECT_FONTPIPELINE_H
#define VULKANPROJECT_FONTPIPELINE_H

#include "../../GraphicsPipeline.h"
#include <glm/mat4x4.hpp>

namespace vke {

  struct Glyph;

  struct GlyphPushConstant {
    glm::mat4 transform;
    int screenWidth;
    int screenHeight;
    float x;
    float y;
    float width;
    float height;
    float u0, v0;
    float u1, v1;
    float r;
    float g;
    float b;
    float a;
  };

  class FontPipeline final : public GraphicsPipeline {
  public:
    FontPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 std::shared_ptr<RenderPass> renderPass,
                 VkDescriptorSetLayout fontDescriptorSetLayout);

    void render(const RenderInfo* renderInfo,
                const std::vector<Glyph>* glyphs,
                VkDescriptorSet descriptorSet);

  private:
    void renderGlyph(const RenderInfo* renderInfo, const Glyph& glyph) const;
  };
} // vke

#endif //VULKANPROJECT_FONTPIPELINE_H