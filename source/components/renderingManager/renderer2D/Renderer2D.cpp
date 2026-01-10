#include "Renderer2D.h"
#include "../../assets/AssetManager.h"
#include "../../assets/fonts/Font.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include <glm/gtc/matrix_transform.hpp>

namespace vke {
  Renderer2D::Renderer2D(std::shared_ptr<AssetManager> assetManager)
    : m_assetManager(std::move(assetManager))
  {}

  void Renderer2D::render(const RenderInfo* renderInfo,
                          const std::shared_ptr<PipelineManager>& pipelineManager)
  {
    normalizeZValues();

    renderRects(pipelineManager, renderInfo);

    renderTriangles(pipelineManager, renderInfo);

    renderEllipses(pipelineManager, renderInfo);

    renderGlyphs(pipelineManager, renderInfo);

    if (m_shouldDoDots)
    {
      pipelineManager->renderDotsPipeline(renderInfo);
    }
  }

  void Renderer2D::createNewFrame()
  {
    m_transformStack.clear();
    resetMatrix();
    fill(255, 255, 255, 255);

    m_currentZ = 0.0f;

    m_rectsToRender.clear();

    m_trianglesToRender.clear();

    m_ellipsesToRender.clear();

    m_glyphsToRender.clear();
  }

  bool Renderer2D::shouldDoDots() const
  {
    return m_shouldDoDots;
  }

  void Renderer2D::setShouldDoDots(const bool shouldDoDots)
  {
    m_shouldDoDots = shouldDoDots;
  }

  void Renderer2D::fill(const float r,
                        const float g,
                        const float b,
                        const float a)
  {
    m_currentFill = glm::vec4(
      r / 255.0f,
      g / 255.0f,
      b / 255.0f,
      a / 255.0f
    );
  }

  void Renderer2D::rotate(const float angle)
  {
    m_currentTransform *= glm::rotate(glm::mat4(1.0), glm::radians(angle), {0.0f, 0.0f, 1.0f});
  }

  void Renderer2D::translate(const float x,
                             const float y)
  {
    m_currentTransform *= glm::translate(glm::mat4(1.0), {x, y, 0});
  }

  void Renderer2D::scale(const float xy)
  {
    m_currentTransform *= glm::scale(glm::mat4(1.0), glm::vec3(xy));
  }

  void Renderer2D::scale(const float x,
                         const float y)
  {
    m_currentTransform *= glm::scale(glm::mat4(1.0), {x, y, 1});
  }

  void Renderer2D::pushMatrix()
  {
    m_transformStack.push_back(m_currentTransform);
  }

  void Renderer2D::popMatrix()
  {
    if (m_transformStack.empty())
    {
      return;
    }

    m_currentTransform = m_transformStack.back();
    m_transformStack.pop_back();
  }

  void Renderer2D::resetMatrix()
  {
    m_currentTransform = glm::mat4(1.0f);
  }

  void Renderer2D::textFont(const std::string& font)
  {
    m_currentFontName = font;

    updateCurrentFont();
  }

  void Renderer2D::textFont(const std::string& font,
                            const uint32_t size)
  {
    m_currentFontName = font;
    m_currentFontSize = size;

    updateCurrentFont();
  }

  void Renderer2D::textSize(const uint32_t size)
  {
    m_currentFontSize = size;

    updateCurrentFont();
  }

  void Renderer2D::rect(const float x,
                        const float y,
                        const float width,
                        const float height)
  {
    m_rectsToRender.push_back({
      .bounds = glm::vec4(x, y, width, height),
      .color = m_currentFill,
      .transform = m_currentTransform,
      .z = m_currentZ
    });

    increaseCurrentZ();
  }

  void Renderer2D::triangle(const float x1,
                            const float y1,
                            const float x2,
                            const float y2,
                            const float x3,
                            const float y3)
  {
    m_trianglesToRender.push_back({
      .p1 = glm::vec2(x1, y1),
      .p2 = glm::vec2(x2, y2),
      .p3 = glm::vec2(x3, y3),
      .color = m_currentFill,
      .transform = m_currentTransform,
      .z = m_currentZ
    });

    increaseCurrentZ();
  }

  void Renderer2D::ellipse(const float x,
                           const float y,
                           const float width,
                           const float height)
  {
    m_ellipsesToRender.push_back({
      .bounds = glm::vec4(x, y, width, height),
      .color = m_currentFill,
      .transform = m_currentTransform,
      .z = m_currentZ
    });

    increaseCurrentZ();
  }

  void Renderer2D::text(const std::string& text,
                        const float x,
                        const float y)
  {
    const float maxGlyphHeight = m_currentFont->getMaxGlyphHeight();

    float currentX = x;

    for (const auto& character : text)
    {
      if (const auto glyphInfo = m_currentFont->getGlyphInfo(character))
      {
        m_glyphsToRender[m_currentFontName][m_currentFontSize].push_back({
          .bounds = glm::vec4(
            currentX + glyphInfo->bearingX,
            y - glyphInfo->bearingY + maxGlyphHeight,
            glyphInfo->width,
            glyphInfo->height
          ),
          .color = m_currentFill,
          .transform = m_currentTransform,
          .uv = glm::vec4(
            glyphInfo->u0,
            glyphInfo->v0,
            glyphInfo->u1,
            glyphInfo->v1
            ),
          .z = m_currentZ
        });

        currentX += glyphInfo->advance;
      }
    }

    increaseCurrentZ();
  }

  void Renderer2D::updateCurrentFont()
  {
    m_currentFont = m_assetManager->getFont(m_currentFontName, m_currentFontSize);
  }

  void Renderer2D::increaseCurrentZ()
  {
    m_currentZ++;
  }

  void Renderer2D::normalizeZValues()
  {
    for (auto& rect : m_rectsToRender)
    {
      rect.z /= m_currentZ;
      rect.z = 1.0f - rect.z;
    }

    for (auto& triangle : m_trianglesToRender)
    {
      triangle.z /= m_currentZ;
      triangle.z = 1.0f - triangle.z;
    }

    for (auto& ellipse : m_ellipsesToRender)
    {
      ellipse.z /= m_currentZ;
      ellipse.z = 1.0f - ellipse.z;
    }

    for (auto& font : m_glyphsToRender)
    {
      for (auto& fontSize : font.second)
      {
        for (auto& glyph : fontSize.second)
        {
          glyph.z /= m_currentZ;
          glyph.z = 1.0f - glyph.z;
        }
      }
    }
  }

  void Renderer2D::renderRects(const std::shared_ptr<PipelineManager>& pipelineManager,
                               const RenderInfo* renderInfo) const
  {
    pipelineManager->bindRectPipeline(renderInfo->commandBuffer);

    for (const auto& rect : m_rectsToRender)
    {
      renderRect(pipelineManager, renderInfo, rect);
    }
  }

  void Renderer2D::renderRect(const std::shared_ptr<PipelineManager>& pipelineManager,
                              const RenderInfo* renderInfo,
                              const Rect& rect)
  {
    const auto rectPC = rect.createPushConstant(renderInfo->extent);

    pipelineManager->pushRectPipelineConstants(
      renderInfo->commandBuffer,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(rectPC),
      &rectPC
    );

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }

  void Renderer2D::renderTriangles(const std::shared_ptr<PipelineManager>& pipelineManager,
                                   const RenderInfo* renderInfo) const
  {
    pipelineManager->bindTrianglePipeline(renderInfo->commandBuffer);

    for (const auto& triangle : m_trianglesToRender)
    {
      renderTriangle(pipelineManager, renderInfo, triangle);
    }
  }

  void Renderer2D::renderTriangle(const std::shared_ptr<PipelineManager>& pipelineManager,
                                  const RenderInfo* renderInfo,
                                  const Triangle& triangle)
  {
    const auto trianglePC = triangle.createPushConstant(renderInfo->extent);

    pipelineManager->pushTrianglePipelineConstants(
      renderInfo->commandBuffer,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(trianglePC),
      &trianglePC
    );

    renderInfo->commandBuffer->draw(3, 1, 0, 0);
  }

  void Renderer2D::renderEllipses(const std::shared_ptr<PipelineManager>& pipelineManager,
                                  const RenderInfo* renderInfo) const
  {
    pipelineManager->bindEllipsePipeline(renderInfo->commandBuffer);

    for (const auto& ellipse : m_ellipsesToRender)
    {
      renderEllipse(pipelineManager, renderInfo, ellipse);
    }
  }

  void Renderer2D::renderEllipse(const std::shared_ptr<PipelineManager>& pipelineManager,
                                 const RenderInfo* renderInfo,
                                 const Ellipse& ellipse)
  {
    const auto ellipsePC = ellipse.createPushConstant(renderInfo->extent);

    pipelineManager->pushEllipsePipelineConstants(
      renderInfo->commandBuffer,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(ellipsePC),
      &ellipsePC
    );

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }

  void Renderer2D::renderGlyphs(const std::shared_ptr<PipelineManager>& pipelineManager,
                                const RenderInfo* renderInfo) const
  {
    pipelineManager->bindFontPipeline(renderInfo->commandBuffer);

    for (const auto& [fontName, fontSizes] : m_glyphsToRender)
    {
      for (const auto& [fontSize, text] : fontSizes)
      {
        const auto descriptorSet = m_assetManager->getFont(fontName, fontSize)->getDescriptorSet(renderInfo->currentFrame);

        pipelineManager->bindFontPipelineDescriptorSet(
          renderInfo->commandBuffer,
          descriptorSet,
          0
        );

        for (const auto& glyph : text)
        {
          renderGlyph(pipelineManager, renderInfo, glyph);
        }
      }
    }
  }

  void Renderer2D::renderGlyph(const std::shared_ptr<PipelineManager>& pipelineManager,
                               const RenderInfo* renderInfo,
                               const Glyph& glyph)
  {
    const auto glyphPC = glyph.createPushConstant(renderInfo->extent);

    pipelineManager->pushFontPipelineConstants(
      renderInfo->commandBuffer,
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      sizeof(glyphPC),
      &glyphPC
    );

    renderInfo->commandBuffer->draw(4, 1, 0, 0);
  }
} // vke