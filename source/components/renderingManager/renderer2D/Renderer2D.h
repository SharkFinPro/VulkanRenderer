#ifndef VULKANPROJECT_RENDERER2D_H
#define VULKANPROJECT_RENDERER2D_H

#include "Primitives2D.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vke {

  class AssetManager;
  class Font;
  class PipelineManager;
  struct RenderInfo;

  class Renderer2D {
  public:
    explicit Renderer2D(std::shared_ptr<AssetManager> assetManager);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager);

    void createNewFrame();

    [[nodiscard]] bool shouldDoDots() const;

    void setShouldDoDots(bool shouldDoDots);

    void fill(float r,
              float g,
              float b,
              float a = 255.0f);

    void rotate(float angle);

    void translate(float x,
                   float y);

    void scale(float xy);

    void scale(float x,
               float y);

    void pushMatrix();

    void popMatrix();

    void resetMatrix();

    void textFont(const std::string& font);

    void textFont(const std::string& font,
                  uint32_t size);

    void textSize(uint32_t size);

    void rect(float x,
              float y,
              float width,
              float height);

    void triangle(float x1,
                  float y1,
                  float x2,
                  float y2,
                  float x3,
                  float y3);

    void ellipse(float x,
                 float y,
                 float width,
                 float height);

    void text(const std::string& text,
              float x,
              float y);

  private:
    std::shared_ptr<AssetManager> m_assetManager;

    glm::vec4 m_currentFill = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    glm::mat4 m_currentTransform = glm::mat4(1.0f);

    std::vector<glm::mat4> m_transformStack;

    std::vector<Rect> m_rectsToRender;

    std::vector<Triangle> m_trianglesToRender;

    std::vector<Ellipse> m_ellipsesToRender;

    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<Glyph>>> m_glyphsToRender;

    std::shared_ptr<Font> m_currentFont;
    std::string m_currentFontName = "";
    uint32_t m_currentFontSize = 12;

    float m_currentZ = 0.0f;

    bool m_shouldDoDots = false;

    void updateCurrentFont();

    void increaseCurrentZ();

    void normalizeZValues();

    void renderRects(const std::shared_ptr<PipelineManager>& pipelineManager,
                    const RenderInfo* renderInfo) const;

    static void renderRect(const std::shared_ptr<PipelineManager>& pipelineManager,
                           const RenderInfo* renderInfo,
                           const Rect& rect);

    void renderTriangles(const std::shared_ptr<PipelineManager>& pipelineManager,
                         const RenderInfo* renderInfo) const;

    static void renderTriangle(const std::shared_ptr<PipelineManager>& pipelineManager,
                               const RenderInfo* renderInfo,
                               const Triangle& triangle);

    void renderEllipses(const std::shared_ptr<PipelineManager>& pipelineManager,
                        const RenderInfo* renderInfo) const;

    static void renderEllipse(const std::shared_ptr<PipelineManager>& pipelineManager,
                              const RenderInfo* renderInfo,
                              const Ellipse& ellipse);

    void renderGlyphs(const std::shared_ptr<PipelineManager>& pipelineManager,
                      const RenderInfo* renderInfo) const;

    static void renderGlyph(const std::shared_ptr<PipelineManager>& pipelineManager,
                            const RenderInfo* renderInfo,
                            const Glyph& glyph);
  };
} // vke

#endif //VULKANPROJECT_RENDERER2D_H