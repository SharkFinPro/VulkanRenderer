#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/assets/objects/SmokeVolume.h>
#include <source/components/assets/particleSystems/SmokeSystem.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void createLights(const vke::VulkanEngine& renderer,
                  std::vector<std::shared_ptr<vke::Light>>& lights);

std::vector<std::shared_ptr<vke::SmokeSystem>> createSmokeSystems(const vke::VulkanEngine& renderer);

std::vector<std::shared_ptr<vke::SmokeVolume>> createSmokeVolumes(const vke::VulkanEngine& renderer);

void displaySmokeSystemGui(const std::shared_ptr<vke::SmokeSystem>& smokeSystem,
                           uint32_t id);

void displaySmokeVolumeGUI(const std::shared_ptr<vke::SmokeVolume>& smokeVolume);

int main()
{
  try
  {
    const vke::EngineConfig engineConfig {
      .window {
        .width = 800,
        .height = 600,
        .title = "Smoke"
      },
      .camera {
        .position = { 0.0f, 5.0f, -15.0f }
      }
    };

    vke::VulkanEngine renderer(engineConfig);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.getAssetManager()->loadModel("assets/models/square.glb");

    const auto object = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, 0, 0 });

    std::vector<std::shared_ptr<vke::Light>> lights;
    createLights(renderer, lights);

    const auto smokeSystems = createSmokeSystems(renderer);

    const auto smokeVolumes = createSmokeVolumes(renderer);

    const auto r3d = renderer.getRenderingManager()->getRenderer3D();

    while (renderer.isActive())
    {
      displayGui(gui, lights, { object }, renderer.getRenderingManager());

      ImGui::Begin("Smoke Systems");
      for (uint32_t i = 0; i < smokeSystems.size(); ++i)
      {
        displaySmokeSystemGui(smokeSystems[i], i + 1);
      }
      ImGui::End();

      r3d->renderObject(object, vke::PipelineType::object);

      for (const auto& system : smokeSystems)
      {
        r3d->renderSmokeSystem(system);
      }

      for (auto volume : smokeVolumes)
      {
        displaySmokeVolumeGUI(volume);

        r3d->renderSmokeVolume(volume);
      }

      for (const auto& light : lights)
      {
        renderer.getLightingManager()->renderLight(light);
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

void createLights(const vke::VulkanEngine& renderer,
                  std::vector<std::shared_ptr<vke::Light>>& lights)
{
  lights.push_back(renderer.getLightingManager()->createPointLight({0, 1.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, 1.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, 1.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, 1.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, 1.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

std::vector<std::shared_ptr<vke::SmokeSystem>> createSmokeSystems(const vke::VulkanEngine& renderer)
{
  std::vector<std::shared_ptr<vke::SmokeSystem>> systems;

  constexpr uint32_t numParticles = 2'500'000;

  systems.push_back(renderer.getAssetManager()->createSmokeSystem({0, 0.95f, 0}, numParticles));
  systems.push_back(renderer.getAssetManager()->createSmokeSystem({-5, 0.95f, -5}, numParticles * 2));
  systems.push_back(renderer.getAssetManager()->createSmokeSystem({-5, 0.95f, 5}, numParticles / 2));
  systems.push_back(renderer.getAssetManager()->createSmokeSystem({5, .95f, 5}, numParticles * 2));
  systems.push_back(renderer.getAssetManager()->createSmokeSystem({5, 0.95f, -5}, numParticles / 2));

  return systems;
}

std::vector<std::shared_ptr<vke::SmokeVolume>> createSmokeVolumes(const vke::VulkanEngine& renderer)
{
  std::vector<std::shared_ptr<vke::SmokeVolume>> volumes;

  volumes.push_back(renderer.getAssetManager()->createSmokeVolume({0, 1.25f, 0}));

  return volumes;
}

void displaySmokeSystemGui(const std::shared_ptr<vke::SmokeSystem>& smokeSystem,
                           const uint32_t id)
{
  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Smoke System " + std::to_string(id)).c_str()))
  {
    auto position = smokeSystem->getPosition();
    auto speed = smokeSystem->getSpeed();
    auto spreadFactor = smokeSystem->getSpreadFactor();
    auto maxSpreadDistance = smokeSystem->getMaxSpreadDistance();
    auto windStrength = smokeSystem->getWindStrength();

    ImGui::SliderFloat3("Position", &position[0], -20, 20);

    ImGui::SliderFloat("Speed", &speed, 0.001f, 10.0f);

    ImGui::SliderFloat("Spread Factor", &spreadFactor, 0.0f, 3.0f);

    ImGui::SliderFloat("Max Spread Distance", &maxSpreadDistance, 0.0f, 20.0f);

    ImGui::SliderFloat("Wind Strength", &windStrength, 0.0f, 3.0f);

    smokeSystem->setPosition(position);
    smokeSystem->setSpeed(speed);
    smokeSystem->setSpreadFactor(spreadFactor);
    smokeSystem->setMaxSpreadDistance(maxSpreadDistance);
    smokeSystem->setWindStrength(windStrength);
  }

  ImGui::PopID();
}

void displaySmokeVolumeGUI(const std::shared_ptr<vke::SmokeVolume>& smokeVolume)
{
  ImGui::Begin("Smoke Volumes");

  auto frequency = smokeVolume->getFrequency();
  if (ImGui::DragFloat("Frequency", &frequency, 0.01f, 0.0f))
  {
    smokeVolume->setFrequency(frequency);
  }

  auto amplitude = smokeVolume->getAmplitude();
  if (ImGui::DragFloat("Amplitude", &amplitude, 0.01f, 0.0f))
  {
    smokeVolume->setAmplitude(amplitude);
  }

  auto density = smokeVolume->getDensity();
  if (ImGui::DragFloat("Density", &density, 0.001f, 0.0f))
  {
    smokeVolume->setDensity(density);
  }

  auto yScale = smokeVolume->getYScale();
  if (ImGui::DragFloat("Y Scale", &yScale, 0.001f, 0.0f))
  {
    smokeVolume->setYScale(yScale);
  }

  static float speed = 25.0f;
  ImGui::DragFloat("Speed", &speed, 0.01f);

  ImGui::End();

  static std::chrono::time_point<std::chrono::steady_clock> previousTime = std::chrono::steady_clock::now();

  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - previousTime).count() / 250.0f;
  previousTime = currentTime;

  smokeVolume->setTime(smokeVolume->getTime() + dt * speed);
}
