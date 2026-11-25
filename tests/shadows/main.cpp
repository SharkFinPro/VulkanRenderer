#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/lighting/lights/DirectionalLight.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/window/Window.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights,
                float lightRange,
                bool sun,
                int cascades,
                float shadowDistance,
                const vke::ShadowBias& shadowBias,
                const glm::vec3& sunDirection);

void renderScene(vke::VulkanEngine& renderer,
                 const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights);

namespace {

  // Three comma separated finite numbers that are not negative
  vke::ShadowBias parseShadowBias(const std::string& text)
  {
    const auto malformed = [] {
      return std::invalid_argument("--shadow-bias needs three comma separated numbers, not negative: C,S,N");
    };

    float values[3];
    size_t start = 0;

    for (int term = 0; term < 3; term++)
    {
      const size_t end = term < 2 ? text.find(',', start) : text.size();

      if (end == std::string::npos || end == start)
      {
        throw malformed();
      }

      size_t used = 0;

      try
      {
        values[term] = std::stof(text.substr(start, end - start), &used);
      }
      catch (const std::logic_error&)
      {
        throw malformed();
      }

      if (used != end - start)
      {
        throw malformed();
      }

      start = end + 1;
    }

    const vke::ShadowBias bias { values[0], values[1], values[2] };

    if (!vke::isValidShadowBias(bias))
    {
      throw malformed();
    }

    return bias;
  }

} // namespace

int main(const int argc,
         char** argv)
{
  try
  {
    // --light-range R gives every light that range (0 = none); --sun adds a directional light, whose cascade count and
    // shadow distance --cascades N (1 to 4) and --shadow-distance D set (0 keeps the light's default) and whose angle
    // above the horizon --sun-elevation DEG (up to 90) sets; --shadow-bias C,S,N sets every light's shadow bias
    float lightRange = 0.0f;
    bool sun = false;
    int cascades = 0;
    float shadowDistance = 0.0f;
    vke::ShadowBias shadowBias;
    glm::vec3 sunDirection(0.4f, -1.0f, 0.3f);
    for (int i = 1; i < argc; i++)
    {
      if (std::string(argv[i]) == "--sun")
      {
        sun = true;
      }
      else if (std::string(argv[i]) == "--light-range")
      {
        if (i + 1 >= argc)
        {
          throw std::invalid_argument("--light-range needs a value");
        }

        lightRange = std::stof(argv[++i]);
      }
      else if (std::string(argv[i]) == "--cascades")
      {
        if (i + 1 >= argc)
        {
          throw std::invalid_argument("--cascades needs a value");
        }

        cascades = std::stoi(argv[++i]);

        if (cascades < 1 || cascades > 4)
        {
          throw std::invalid_argument("--cascades must be from 1 to 4");
        }
      }
      else if (std::string(argv[i]) == "--shadow-distance")
      {
        if (i + 1 >= argc)
        {
          throw std::invalid_argument("--shadow-distance needs a value");
        }

        shadowDistance = std::stof(argv[++i]);

        if (!(shadowDistance > 0.0f))
        {
          throw std::invalid_argument("--shadow-distance must be greater than 0");
        }
      }
      else if (std::string(argv[i]) == "--shadow-bias")
      {
        if (i + 1 >= argc)
        {
          throw std::invalid_argument("--shadow-bias needs a value");
        }

        shadowBias = parseShadowBias(argv[++i]);
      }
      else if (std::string(argv[i]) == "--sun-elevation")
      {
        if (i + 1 >= argc)
        {
          throw std::invalid_argument("--sun-elevation needs a value");
        }

        const std::string value = argv[++i];
        size_t used = 0;
        const float elevation = std::stof(value, &used);

        if (used != value.size())
        {
          throw std::invalid_argument("--sun-elevation must be a number of degrees");
        }

        if (!(elevation > 0.0f && elevation <= 90.0f))
        {
          throw std::invalid_argument("--sun-elevation must be greater than 0 and at most 90");
        }

        const float radians = glm::radians(elevation);
        sunDirection = glm::normalize(glm::vec3(std::cos(radians) * 0.8f, -std::sin(radians), std::cos(radians) * 0.6f));
      }
    }

    const vke::EngineConfig engineConfig {
      .window {
        .width = 800,
        .height = 600,
        .title = "Shadows",
        .closeOnEscape = true
      },
      .camera {
        .position = { 0.0f, 0.0f, -5.0f }
      }
    };

    vke::VulkanEngine renderer(engineConfig);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    std::vector<std::shared_ptr<vke::RenderObject>> objects;
    std::vector<std::shared_ptr<vke::Light>> lights;
    setupScene(renderer, objects, lights, lightRange, sun, cascades, shadowDistance, shadowBias, sunDirection);

    while (renderer.isActive())
    {
      renderScene(renderer, gui, objects, lights);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights,
                const float lightRange,
                const bool sun,
                const int cascades,
                const float shadowDistance,
                const vke::ShadowBias& shadowBias,
                const glm::vec3& sunDirection)
{
  const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
  const auto model = renderer.getAssetManager()->loadModel("assets/models/square.glb");

  const auto object1 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object1->setPosition({ 0, -5, 0 });
  objects.push_back(object1);

  const auto object2 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object2->setPosition({ -5, -10, 0 });
  objects.push_back(object2);

  const auto object3 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object3->setPosition({ 10, 0, 15 });
  objects.push_back(object3);

  // Reaches well past 100 units from the lights, where rasterized shadows used to stop
  const auto ground = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  ground->setPosition({ 0, -15, 0 });
  ground->setScale({ 20.0f, 1.0f, 20.0f }); // 400 x 2 x 400
  objects.push_back(ground);

  lights.push_back(renderer.getLightingManager()->createSpotLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createSpotLight({5.0f, 3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  if (lightRange != 0.0f)
  {
    for (const auto& light : lights)
    {
      light->setRange(lightRange);
    }
  }

  if (sun)
  {
    // Travels down (-y is toward the floor here) and slants toward +x and +z. It has no range, so it is added after the
    // range above. Its cascades follow the camera out to the shadow distance.
    const auto sunLight = renderer.getLightingManager()->createDirectionalLight(
      sunDirection, { 1.0f, 1.0f, 0.95f }, 0.0f, 0.6f, 0.5f);
    const auto directionalLight = std::dynamic_pointer_cast<vke::DirectionalLight>(sunLight);

    if (cascades != 0)
    {
      directionalLight->setCascadeCount(static_cast<uint32_t>(cascades));
    }

    if (shadowDistance != 0.0f)
    {
      directionalLight->setShadowDistance(shadowDistance);
    }

    lights.push_back(sunLight);
  }

  for (const auto& light : lights)
  {
    light->setShadowBias(shadowBias);
  }
}

void renderScene(vke::VulkanEngine& renderer,
                 const std::shared_ptr<vke::ImGuiInstance>& gui,
                 const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();

  displayGui(gui, lights, objects, renderer.getRenderingManager());

  // Render Objects
  for (auto& object : objects)
  {
    r3d->renderObject(object, vke::PipelineType::object);
  }

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}