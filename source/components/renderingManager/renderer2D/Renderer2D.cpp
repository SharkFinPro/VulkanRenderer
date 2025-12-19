#include "Renderer2D.h"
#include "../../assets/fonts/Font.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include <glm/gtc/matrix_transform.hpp>

namespace vke {
  void Renderer2D::render(const RenderInfo* renderInfo,
                          const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    pipelineManager->renderRectPipeline(renderInfo, &m_rectsToRender);
  }

  void Renderer2D::createNewFrame()
  {
    m_rectsToRender.clear();

    m_glyphsToRender.clear();
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

  void Renderer2D::font(std::shared_ptr<Font> font)
  {
    m_currentFont = std::move(font);
  }

  void Renderer2D::rect(const float x,
                        const float y,
                        const float w,
                        const float h)
  {
    m_rectsToRender.push_back({
      .bounds = glm::vec4(x, y, w, h),
      .color = m_currentFill,
      .transform = m_currentTransform
    });
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
        m_glyphsToRender.push_back({
          .bounds = glm::vec4(
            x + glyphInfo->bearingX,
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
          )
        });

        currentX += glyphInfo->advance;
      }
    }
  }
} // vke