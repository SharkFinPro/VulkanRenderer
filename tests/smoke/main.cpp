#include "../common/gui.h"
#include <source/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void createLights(const VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights);
void createSmokeSystems(VulkanEngine& renderer);
void setDockOptions(const std::shared_ptr<ImGuiInstance>& gui);
void displayGui(const std::shared_ptr<ImGuiInstance>& gui, const std::vector<std::shared_ptr<Light>>& lights,
                const std::shared_ptr<RenderObject>& object);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Smoke",
      .CAMERA_POSITION = { 0.0f, 5.0f, -15.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/square.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, 0, 0 });

    std::vector<std::shared_ptr<Light>> lights;
    createLights(renderer, lights);

    createSmokeSystems(renderer);

    while (renderer.isActive())
    {
      displayGui(gui, lights, { object });

      renderer.renderObject(object, PipelineType::object);

      for (const auto& light : lights)
      {
        renderer.renderLight(light);
      }

      renderer.render();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void createLights(const VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights)
{
  lights.push_back(renderer.createLight({0, 1.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, 1.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, 1.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, 1.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, 1.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void createSmokeSystems(VulkanEngine& renderer)
{
  constexpr uint32_t numParticles = 2'500'000;

  renderer.createSmokeSystem({0, 0.95f, 0}, numParticles);
  renderer.createSmokeSystem({-5, 0.95f, -5}, numParticles * 2);
  renderer.createSmokeSystem({-5, 0.95f, 5}, numParticles / 2);
  renderer.createSmokeSystem({5, .95f, 5}, numParticles * 2);
  renderer.createSmokeSystem({5, 0.95f, -5}, numParticles / 2);
}
