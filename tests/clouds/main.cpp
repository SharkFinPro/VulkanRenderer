#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/assets/objects/Cloud.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

const vke::EngineConfig ENGINE_CONFIG {
  .window {
    .width = 800,
    .height = 600,
    .title = "Clouds"
  },
  .camera {
    .position = { 0.0f, 5.0f, -15.0f }
  }
};

std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer);

std::shared_ptr<vke::RenderObject> createCubeObject(const vke::VulkanEngine& renderer);

void displayCloudGUI(const vke::VulkanEngine& renderer,
                     const std::shared_ptr<vke::Cloud>& cloud);

int main()
{
  try
  {
    vke::VulkanEngine renderer(ENGINE_CONFIG);

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto cubeObject = createCubeObject(renderer);

    const auto lights = createLights(renderer);

    auto currentPipeline = vke::PipelineType::object;

    const auto r3d = renderer.getRenderingManager()->getRenderer3D();

    const std::shared_ptr<vke::Cloud> cloud = renderer.getAssetManager()->createCloud();

    while (renderer.isActive())
    {
      displayGui(renderer.getImGuiInstance(), lights, { cubeObject }, renderer.getRenderingManager());

      displayCloudGUI(renderer, cloud);

      r3d->renderObject(cubeObject, currentPipeline);

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

std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer)
{
  return {
    renderer.getLightingManager()->createPointLight({0, 1.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.4f, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({5.0f, 1.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({-5.0f, 1.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({5.0f, 1.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f),
    renderer.getLightingManager()->createPointLight({-5.0f, 1.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f)
  };
}

std::shared_ptr<vke::RenderObject> createCubeObject(const vke::VulkanEngine& renderer)
{
  const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");

  const auto cubeModel = renderer.getAssetManager()->loadModel("assets/models/square.glb");

  const auto cubeObject = renderer.getAssetManager()->loadRenderObject(texture, specularMap, cubeModel);
  cubeObject->setPosition({ 0, 0, 0 });
  cubeObject->setScale({ 1000.0f, 1.0f, 1000.0f });

  return cubeObject;
}

void displayCloudGUI(const vke::VulkanEngine& renderer,
                     const std::shared_ptr<vke::Cloud>& cloud)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();
  r3d->setCloudToRender(cloud);

  ImGui::Begin("Clouds");

  auto frequency = cloud->getFrequency();
  if (ImGui::DragFloat("Frequency", &frequency, 0.01f, 0.0f))
  {
    cloud->setFrequency(frequency);
  }

  auto amplitude = cloud->getAmplitude();
  if (ImGui::DragFloat("Amplitude", &amplitude, 0.01f, 0.0f))
  {
    cloud->setAmplitude(amplitude);
  }

  auto density = cloud->getDensity();
  if (ImGui::DragFloat("Density", &density, 0.001f, 0.0f, 1.0f))
  {
    cloud->setDensity(density);
  }

  auto yScale = cloud->getYScale();
  if (ImGui::DragFloat("Y Scale", &yScale, 0.001f, 0.0f, 2.0f))
  {
    cloud->setYScale(yScale);
  }

  static float speed = 1.0f;
  ImGui::DragFloat("Speed", &speed, 0.01f);

  ImGui::End();

  static std::chrono::time_point<std::chrono::steady_clock> previousTime = std::chrono::steady_clock::now();

  const auto currentTime = std::chrono::steady_clock::now();
  const float dt = std::chrono::duration<float>(currentTime - previousTime).count() / 250.0f;
  previousTime = currentTime;

  cloud->setTime(cloud->getTime() + dt * speed);
}