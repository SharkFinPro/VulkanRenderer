#ifndef VULKANPROJECT_RENDERER2D_H
#define VULKANPROJECT_RENDERER2D_H

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

  struct Glyph {
    glm::vec4 bounds;
    glm::vec4 color;
    glm::mat4 transform;
    glm::vec4 uv;
  };

  struct Rect {
    glm::vec4 bounds;
    glm::vec4 color;
    glm::mat4 transform;
  };

  struct Triangle {
    glm::vec2 p1;
    glm::vec2 p2;
    glm::vec2 p3;
    glm::vec4 color;
    glm::mat4 transform;
  };

  class Renderer2D {
  public:
    explicit Renderer2D(std::shared_ptr<AssetManager> assetManager);

    void render(const RenderInfo* renderInfo,
                const std::shared_ptr<PipelineManager>& pipelineManager) const;

    void createNewFrame();

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

    void textFont(const std::string& font);

    void textFont(const std::string& font,
                  uint32_t size);

    void textSize(uint32_t size);

    void rect(float x,
              float y,
              float w,
              float h);

    void text(const std::string& text,
              float x,
              float y);

  private:
    std::shared_ptr<AssetManager> m_assetManager;

    glm::vec4 m_currentFill = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    glm::mat4 m_currentTransform = glm::mat4(1.0f);

    std::vector<glm::mat4> m_transformStack;

    std::vector<Rect> m_rectsToRender;

    std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<Glyph>>> m_glyphsToRender;

    std::shared_ptr<Font> m_currentFont;
    std::string m_currentFontName = "";
    uint32_t m_currentFontSize = 12;

    void updateCurrentFont();
  };
} // vke

#endif //VULKANPROJECT_RENDERER2D_H