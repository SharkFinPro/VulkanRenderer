#ifndef VULKANPROJECT_FONT_H
#define VULKANPROJECT_FONT_H

#include <freetype/freetype.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vke {

  class DescriptorSet;
  class LogicalDevice;
  class TextureGlyph;

  struct GlyphInfo {
    float u0, v0;
    float u1, v1;
    float width, height;
    float bearingX, bearingY;
    float advance;
  };

  inline std::vector<uint32_t> decodeUTF8(const std::string& utf8String)
  {
    std::vector<uint32_t> codepoints;
    size_t i = 0;

    while (i < utf8String.length())
    {
      uint32_t codepoint = 0;
      uint8_t byte = utf8String[i];

      if (byte <= 0x7F) // 1-byte (ASCII)
      {
        codepoint = byte;
        i += 1;
      }
      else if ((byte & 0xE0) == 0xC0) // 2-byte
      {
        codepoint = ((byte & 0x1F) << 6) | (utf8String[i + 1] & 0x3F);
        i += 2;
      }
      else if ((byte & 0xF0) == 0xE0) // 3-byte
      {
        codepoint = ((byte & 0x0F) << 12) |
                   ((utf8String[i + 1] & 0x3F) << 6) |
                   (utf8String[i + 2] & 0x3F);
        i += 3;
      }
      else if ((byte & 0xF8) == 0xF0) // 4-byte (emoji range)
      {
        codepoint = ((byte & 0x07) << 18) |
                   ((utf8String[i + 1] & 0x3F) << 12) |
                   ((utf8String[i + 2] & 0x3F) << 6) |
                   (utf8String[i + 3] & 0x3F);
        i += 4;
      }
      else
      {
        i += 1; // Skip invalid byte
        continue;
      }

      codepoints.push_back(codepoint);
    }

    return codepoints;
  }

  class Font {
  public:
    Font(std::shared_ptr<LogicalDevice> logicalDevice,
         const std::string& fileName,
         uint32_t fontSize,
         VkCommandPool commandPool,
         VkDescriptorPool descriptorPool,
         VkDescriptorSetLayout descriptorSetLayout);

    [[nodiscard]] GlyphInfo* getGlyphInfo(uint32_t codepoint);

    [[nodiscard]] float getMaxGlyphHeight() const;

    [[nodiscard]] VkDescriptorSet getDescriptorSet(uint32_t currentFrame) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<TextureGlyph> m_glyphTexture;

    std::unordered_map<uint32_t, GlyphInfo> m_glyphMap;

    float m_maxGlyphHeight = 0.0f;

    std::shared_ptr<DescriptorSet> m_descriptorSet;

    static std::vector<uint8_t> loadFontFromFile(const std::string& fileName);

    void createGlyphAtlas(VkCommandPool commandPool,
                          const std::vector<uint8_t>& fontBuffer,
                          uint32_t fontSize);

    static std::vector<FT_ULong> getCharset(FT_Face face);

    static std::vector<uint8_t> createAtlasBuffer(FT_Face face,
                                                  const std::vector<FT_ULong>& charset,
                                                  uint32_t& maxGlyphWidth,
                                                  uint32_t& maxGlyphHeight,
                                                  uint32_t& glyphsPerRow,
                                                  uint32_t& atlasWidth,
                                                  uint32_t& atlasHeight);

    void populateAtlasBuffer(FT_Face face,
                             const std::vector<FT_ULong>& charset,
                             std::vector<uint8_t>& atlasBuffer,
                             uint32_t maxGlyphWidth,
                             uint32_t maxGlyphHeight,
                             uint32_t glyphsPerRow,
                             uint32_t atlasWidth,
                             uint32_t atlasHeight);

    void createDescriptorSet(VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout descriptorSetLayout);
  };

} // vke

#endif //VULKANPROJECT_FONT_H