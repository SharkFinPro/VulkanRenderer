#include "Font.h"
#include "../textures/TextureGlyph.h"
#include <fstream>
#include <stdexcept>
#include <utility>

constexpr uint32_t MAX_ASCII_CODE = 255;

namespace vke {
  Font::Font(std::shared_ptr<LogicalDevice> logicalDevice,
             const std::string& fileName,
             const uint32_t fontSize,
             VkCommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice))
  {
    std::unique_ptr<uint8_t[]> fontBuffer;
    size_t fontBufferSize = 0;

    loadFontFromFile(fileName, fontBuffer, fontBufferSize);

    createGlyphAtlas(commandPool, fontBuffer, fontBufferSize, fontSize);
  }

  void Font::loadFontFromFile(const std::string& fileName,
                              std::unique_ptr<uint8_t[]>& fontBuffer,
                              size_t& fontBufferSize)
  {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (!file)
    {
      throw std::runtime_error(std::string("Failed to open file: ") + fileName);
    }

    fontBufferSize = file.tellg();
    file.seekg(0, std::ios::beg);

    fontBuffer = std::make_unique<uint8_t[]>(fontBufferSize);
    file.read(reinterpret_cast<char*>(fontBuffer.get()), static_cast<std::streamsize>(fontBufferSize));

    if (!file)
    {
      throw std::runtime_error(std::string("Failed to read file: ") + fileName);
    }

    file.close();
  }

  void Font::createGlyphAtlas(VkCommandPool commandPool,
                              const std::unique_ptr<uint8_t[]>& fontBuffer,
                              const size_t& fontBufferSize,
                              const uint32_t fontSize)
  {
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
      throw std::runtime_error("Failed to initialize FreeType");
    }

    FT_Face face;
    if (FT_New_Memory_Face(ft, fontBuffer.get(), static_cast<FT_Long>(fontBufferSize), 0, &face))
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
        for (uint32_t col = 0; col < bitmap.width; ++col)
        {
          const uint32_t atlasX = x + col;
          const uint32_t atlasY = y + row;
          const uint32_t atlasIndex = atlasY * atlasWidth + atlasX;
          const uint32_t bitmapIndex = row * bitmap.width + col;

          if (atlasIndex < atlasBuffer.size())
          {
            atlasBuffer[atlasIndex] = bitmap.buffer[bitmapIndex];
          }
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
} // vke