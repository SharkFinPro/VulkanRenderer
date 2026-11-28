// Headless checks of the volume palette rules and triplanar math (VolumeMaterial.h). No engine, no GPU: ctest runs this.
#include "source/components/assets/volume/VolumeMaterial.h"
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  int g_checks = 0;
  int g_failures = 0;

  void check(const bool condition,
             const char* name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  std::string readFile(const char* path)
  {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
  }

  bool throwsInvalidArgument(const std::vector<vke::VolumeMaterial>& palette,
                             std::string* message = nullptr)
  {
    try
    {
      vke::validateVolumePalette(palette);
    }
    catch (const std::invalid_argument& e)
    {
      if (message)
      {
        *message = e.what();
      }

      return true;
    }

    return false;
  }

  void testValidation()
  {
    check(!throwsInvalidArgument({}), "an empty palette is valid");
    check(!throwsInvalidArgument(std::vector<vke::VolumeMaterial>(16)), "16 materials are valid");

    std::string message;
    check(throwsInvalidArgument(std::vector<vke::VolumeMaterial>(17), &message), "17 materials throw");
    check(message.find("16") != std::string::npos, "the message names the limit");

    for (const float scale : { 0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::infinity() })
    {
      std::vector<vke::VolumeMaterial> palette(3);
      palette[1].textureScale = scale;

      check(throwsInvalidArgument(palette), "a texture scale that is not finite and positive throws");
    }

    std::vector<vke::VolumeMaterial> scaled(2);
    scaled[0].textureScale = 0.25f;
    scaled[1].textureScale = 8.0f;
    check(!throwsInvalidArgument(scaled), "positive finite scales are valid");
  }

  void testColor()
  {
    check(vke::VolumeMaterial {}.color == glm::vec3(1.0f), "the default color is white");
    check(vke::albedoFill(vke::VolumeMaterial {}) == glm::vec4(1.0f), "an unset color fills white");

    vke::VolumeMaterial red;
    red.color = { 1.0f, 0.0f, 0.25f };
    check(vke::albedoFill(red) == glm::vec4(1.0f, 0.0f, 0.25f, 1.0f), "a texture-less material fills its color");
    check(!throwsInvalidArgument({ red }), "a color in range is valid");

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();

    for (const float bad : { nan, inf, -0.01f, 1.01f })
    {
      for (int c = 0; c < 3; c++)
      {
        std::vector<vke::VolumeMaterial> palette(3);
        palette[2].color[c] = bad;

        std::string message;
        check(throwsInvalidArgument(palette, &message), "a color component that is bad throws");
        check(message.find("material 2") != std::string::npos, "the message names the material");
      }
    }
  }

  void testTriplanar()
  {
    std::mt19937 random(7);
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);

    bool sumsToOne = true;
    bool aboveMinimum = true;

    for (int i = 0; i < 2000; i++)
    {
      glm::vec3 n(unit(random), unit(random), unit(random));

      if (glm::dot(n, n) < 1e-4f)
      {
        continue;
      }

      const glm::vec3 w = vke::triplanarWeights(glm::normalize(n));

      sumsToOne = sumsToOne && std::abs(w.x + w.y + w.z - 1.0f) < 1e-5f;

      for (int c = 0; c < 3; c++)
      {
        aboveMinimum = aboveMinimum && (w[c] == 0.0f || w[c] >= vke::s_triplanarMinWeight);
      }
    }

    check(sumsToOne, "weights sum to 1");
    check(aboveMinimum, "a nonzero weight is at least the minimum");

    check(vke::triplanarWeights({ 1.0f, 0.0f, 0.0f }) == glm::vec3(1.0f, 0.0f, 0.0f), "x axis");
    check(vke::triplanarWeights({ 0.0f, -1.0f, 0.0f }) == glm::vec3(0.0f, 1.0f, 0.0f), "y axis");
    check(vke::triplanarWeights({ 0.0f, 0.0f, 1.0f }) == glm::vec3(0.0f, 0.0f, 1.0f), "z axis");

    const glm::vec3 diagonal = vke::triplanarWeights(glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
    check(std::abs(diagonal.x - 0.5f) < 1e-5f && std::abs(diagonal.y - 0.5f) < 1e-5f && diagonal.z == 0.0f,
          "a 45 degree normal in xy splits evenly and drops z");

    // Just below and just above the threshold: n = (1, e, 0) normalized, raw weight of y is e^4 / (1 + e^4)
    const auto yWeightRaw = [](const float e) { return std::pow(e, 4.0f) / (1.0f + std::pow(e, 4.0f)); };
    const float belowE = std::pow(vke::s_triplanarMinWeight * 0.9f, 0.25f);
    const float aboveE = std::pow(vke::s_triplanarMinWeight * 1.2f, 0.25f);

    check(yWeightRaw(belowE) < vke::s_triplanarMinWeight &&
          vke::triplanarWeights(glm::normalize(glm::vec3(1.0f, belowE, 0.0f))).y == 0.0f,
          "a weight below the minimum drops out");
    check(yWeightRaw(aboveE) >= vke::s_triplanarMinWeight &&
          vke::triplanarWeights(glm::normalize(glm::vec3(1.0f, aboveE, 0.0f))).y > 0.0f,
          "a weight above the minimum stays");
  }

  // The value assigned to a GLSL constant, or NaN when the declaration is missing
  float shaderConstant(const std::string& source,
                       const char* name)
  {
    const std::regex pattern(std::string("const\\s+float\\s+") + name + "\\s*=\\s*([0-9.eE+-]+)\\s*;");
    std::smatch match;

    if (!std::regex_search(source, match, pattern))
    {
      return std::numeric_limits<float>::quiet_NaN();
    }

    return std::stof(match[1].str());
  }

  void testShaderMirrors()
  {
    const std::string glsl = readFile(VKE_VOLUME_MATERIAL_GLSL);
    check(!glsl.empty(), "VolumeMaterial.glsl is readable");

    check(shaderConstant(glsl, "triplanarSharpness") == vke::s_triplanarSharpness, "the shader's sharpness matches");
    check(shaderConstant(glsl, "triplanarMinWeight") == vke::s_triplanarMinWeight, "the shader's minimum weight matches");
    std::smatch match;
    check(std::regex_search(glsl, match, std::regex(R"(const\s+int\s+maxVolumeMaterials\s*=\s*(\d+)\s*;)")) &&
          std::stoul(match[1].str()) == vke::s_maxVolumeMaterials, "the shader's palette size matches");
  }

} // namespace

int main()
{
  testValidation();
  testColor();
  testTriplanar();
  testShaderMirrors();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
