#include "Font.h"
#include "../textures/TextureGlyph.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <utility>

constexpr uint32_t MAX_ASCII_CODE = 255;

namespace vke {
  Font::Font(std::shared_ptr<LogicalDevice> logicalDevice,
             const std::string& fileName,
             const uint32_t fontSize,
             VkCommandPool commandPool,
             VkDescriptorPool descriptorPool,
             VkDescriptorSetLayout descriptorSetLayout)
    : m_logicalDevice(std::move(logicalDevice))
  {
    const auto fontBuffer = loadFontFromFile(fileName);

    createGlyphAtlas(commandPool, fontBuffer, fontSize);

    createDescriptorSet(descriptorPool, descriptorSetLayout);
  }

  GlyphInfo* Font::getGlyphInfo(const char character)
  {
    const auto it = m_glyphMap.find(character);

    return it != m_glyphMap.end() ? &it->second : nullptr;
  }

  float Font::getMaxGlyphHeight() const
  {
    return m_maxGlyphHeight;
  }

  VkDescriptorSet Font::getDescriptorSet(const uint32_t currentFrame) const
  {
    return m_descriptorSet->getDescriptorSet(currentFrame);
  }

  std::vector<uint8_t> Font::loadFontFromFile(const std::string& fileName)
  {
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
      throw std::runtime_error("Failed to open file: " + fileName);
    }

    const size_t fileSize = file.tellg();
    std::vector<uint8_t> buffer(fileSize);

    file.seekg(0);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!file)
    {
      throw std::runtime_error("Failed to read file: " + fileName);
    }

    file.close();

    return buffer;
  }

  void Font::createGlyphAtlas(VkCommandPool commandPool,
                              const std::vector<uint8_t>& fontBuffer,
                              const uint32_t fontSize)
  {
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
      throw std::runtime_error("Failed to initialize FreeType");
    }

    FT_Face face;
    if (FT_New_Memory_Face(ft, fontBuffer.data(), fontBuffer.size(), 0, &face))
    {
      FT_Done_FreeType(ft);
      throw std::runtime_error("Failed to load font from memory");
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    const auto charset = getCharset(face);

    uint32_t maxGlyphWidth, maxGlyphHeight, glyphsPerRow, atlasWidth, atlasHeight;
    auto atlasBuffer = createAtlasBuffer(face, charset, maxGlyphWidth, maxGlyphHeight,
                                         glyphsPerRow, atlasWidth, atlasHeight);


    populateAtlasBuffer(face, charset, atlasBuffer, maxGlyphWidth, maxGlyphHeight,
                        glyphsPerRow, atlasWidth, atlasHeight);

    m_glyphTexture = std::make_shared<TextureGlyph>(
      m_logicalDevice,
      commandPool,
      atlasBuffer.data(),
      atlasWidth,
      atlasHeight
    );

    m_maxGlyphHeight = static_cast<float>(maxGlyphHeight);
  }

  std::vector<FT_ULong> Font::getCharset(const FT_Face face)
  {
    std::vector<FT_ULong> charset;

    FT_UInt gindex;
    FT_ULong charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0)
    {
      if (charcode <= MAX_ASCII_CODE)
      {
        charset.push_back(charcode);
      }
      charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    return charset;
  }

  std::vector<uint8_t> Font::createAtlasBuffer(FT_Face face,
                                               const std::vector<FT_ULong>& charset,
                                               uint32_t& maxGlyphWidth,
                                               uint32_t& maxGlyphHeight,
                                               uint32_t& glyphsPerRow,
                                               uint32_t& atlasWidth,
                                               uint32_t& atlasHeight)
  {
    maxGlyphWidth = 0;
    maxGlyphHeight = 0;

    for (FT_ULong charcode : charset)
    {
      if (FT_Load_Char(face, charcode, FT_LOAD_RENDER))
      {
        continue;
      }
      maxGlyphWidth = std::max(maxGlyphWidth, face->glyph->bitmap.width);
      maxGlyphHeight = std::max(maxGlyphHeight, face->glyph->bitmap.rows);
    }

    glyphsPerRow = static_cast<uint32_t>(std::ceil(std::sqrt(charset.size())));
    atlasWidth = glyphsPerRow * maxGlyphWidth;
    atlasHeight = glyphsPerRow * maxGlyphHeight;

    std::vector<uint8_t> atlasBuffer(atlasWidth * atlasHeight, 0);

    return atlasBuffer;
  }

  void Font::populateAtlasBuffer(FT_Face face,
                                 const std::vector<FT_ULong>& charset,
                                 std::vector<uint8_t>& atlasBuffer,
                                 const uint32_t maxGlyphWidth,
                                 const uint32_t maxGlyphHeight,
                                 const uint32_t glyphsPerRow,
                                 const uint32_t atlasWidth,
                                 const uint32_t atlasHeight)
  {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t currentGlyph = 0;

    for (FT_ULong charcode : charset)
    {
      if (FT_Load_Char(face, charcode, FT_LOAD_RENDER))
      {
        continue;
      }

      const FT_Bitmap& bitmap = face->glyph->bitmap;

      for (uint32_t row = 0; row < bitmap.rows; ++row)
      {
        const uint32_t atlasOffset = (y + row) * atlasWidth + x;
        const uint32_t bitmapOffset = row * bitmap.width;

        if (atlasOffset + bitmap.width <= atlasBuffer.size())
        {
          std::memcpy(&atlasBuffer[atlasOffset], &bitmap.buffer[bitmapOffset], bitmap.width);
        }
      }

      m_glyphMap[static_cast<char>(charcode)] = {
        .u0 = static_cast<float>(x) / static_cast<float>(atlasWidth),
        .v0 = static_cast<float>(y) / static_cast<float>(atlasHeight),
        .u1 = static_cast<float>(x + bitmap.width) / static_cast<float>(atlasWidth),
        .v1 = static_cast<float>(y + bitmap.rows) / static_cast<float>(atlasHeight),
        .width = static_cast<float>(bitmap.width),
        .height = static_cast<float>(bitmap.rows),
        .bearingX = static_cast<float>(face->glyph->bitmap_left),
        .bearingY = static_cast<float>(face->glyph->bitmap_top),
        .advance = static_cast<float>(face->glyph->advance.x >> 6)
      };

      x += maxGlyphWidth;
      currentGlyph++;

      if (currentGlyph % glyphsPerRow == 0)
      {
        x = 0;
        y += maxGlyphHeight;
      }
    }
  }

  void Font::createDescriptorSet(VkDescriptorPool descriptorPool,
                                 VkDescriptorSetLayout descriptorSetLayout)
  {
    m_descriptorSet = std::make_shared<DescriptorSet>(m_logicalDevice, descriptorPool, descriptorSetLayout);
    m_descriptorSet->updateDescriptorSets([this](const VkDescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_glyphTexture->getDescriptorSet(0, descriptorSet)
      }};

      return descriptorWrites;
    });
  }
} // vke