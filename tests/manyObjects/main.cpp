#include <source/components/assets/AssetManager.h>
#include <source/components/assets/objects/Model.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/assets/textures/Texture2D.h>
#include <source/components/imGui/ImGuiInstance.h>
#include <source/components/lighting/LightingManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/components/renderingManager/RenderingManager.h>
#include <source/components/renderingManager/renderer3D/Renderer3D.h>
#include <source/components/window/PresentMode.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <vector>

constexpr float OBJECT_SPACING = 2.5f;
constexpr size_t ROLLING_FRAMES = 120;
constexpr std::array MODEL_PATHS {
  "assets/models/square.glb",
  "assets/models/cube.obj",
  "assets/models/curtain.glb"
};
constexpr std::array<std::array<const char*, 2>, 3> MATERIAL_PATHS {{
  { "assets/textures/white.png", "assets/textures/blank_specular.png" },
  { "assets/textures/container.png", "assets/textures/container_specular.png" },
  { "assets/textures/texture.jpg", "assets/textures/blank_specular.png" }
}};

struct Options {
  uint32_t objects = 10000;
  uint32_t frames = 0;
  uint32_t warmup = 120;
  uint32_t frameRateLimit = 0;
  std::optional<vke::PresentMode> presentMode;
};

struct FrameTimes {
  double submitMs = 0.0;
  double renderMs = 0.0;
  double frameMs = 0.0;
};

struct Samples {
  std::vector<double> submit;
  std::vector<double> render;
  std::vector<double> frame;
};

using Clock = std::chrono::steady_clock;

[[nodiscard]] bool parseValue(const char* text,
                              uint32_t& value);

[[nodiscard]] bool parsePresentMode(const char* text,
                                    vke::PresentMode& mode);

[[nodiscard]] const char* presentModeName(vke::PresentMode mode);

[[nodiscard]] bool parseOptions(int argc,
                                char** argv,
                                Options& options);

[[nodiscard]] vke::EngineConfig createConfig(uint32_t objects);

[[nodiscard]] uint32_t gridSide(uint32_t objects);

[[nodiscard]] std::vector<std::shared_ptr<vke::RenderObject>> createObjects(const vke::VulkanEngine& renderer,
                                                                            uint32_t objects);

[[nodiscard]] std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer,
                                                                    uint32_t objects);

void updateObjects(const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                   float seconds);

void submitScene(vke::VulkanEngine& renderer,
                 const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights);

void dockWindow(const std::shared_ptr<vke::ImGuiInstance>& gui,
                uint64_t frameIndex);

void displayStats(size_t objectCount,
                  const std::array<FrameTimes, ROLLING_FRAMES>& rolling,
                  size_t rollingCount);

[[nodiscard]] double milliseconds(Clock::time_point from,
                                  Clock::time_point to);

[[nodiscard]] double percentile(const std::vector<double>& sorted,
                                double fraction);

void printMetric(const char* name,
                 std::vector<double>& values);

void printSummary(const Options& options,
                  const vke::RenderingManager& rendering,
                  Samples& samples);

struct Scene {
  std::vector<std::shared_ptr<vke::RenderObject>> objects;
  std::vector<std::shared_ptr<vke::Light>> lights;
};

struct RollingTimes {
  std::array<FrameTimes, ROLLING_FRAMES> times {};
  size_t count = 0;
};

[[nodiscard]] FrameTimes runFrame(vke::VulkanEngine& renderer,
                                  const Scene& scene,
                                  const RollingTimes& rolling,
                                  uint64_t frameIndex,
                                  Clock::time_point startTime);

[[nodiscard]] Samples runBenchmark(vke::VulkanEngine& renderer,
                                   const Scene& scene,
                                   const Options& options);

int main(int argc, char** argv)
{
  try
  {
    Options options;
    if (!parseOptions(argc, argv, options))
    {
      std::cerr << "Usage: manyObjects [--objects N (>= 1)] [--frames N] [--warmup N] [--frame-rate-limit N (0 = unlimited)]"
                << " [--present-mode fifo|fifoRelaxed|mailbox|immediate]" << std::endl;
      return EXIT_FAILURE;
    }

    vke::VulkanEngine renderer(createConfig(options.objects));

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto rendering = renderer.getRenderingManager();
    rendering->setFrameRateLimit(options.frameRateLimit);
    if (options.presentMode)
    {
      rendering->setPresentMode(*options.presentMode);
    }

    const Scene scene {
      .objects = createObjects(renderer, options.objects),
      .lights = createLights(renderer, options.objects)
    };

    Samples samples = runBenchmark(renderer, scene, options);

    if (options.frames > 0)
    {
      printSummary(options, *rendering, samples);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

FrameTimes runFrame(vke::VulkanEngine& renderer,
                    const Scene& scene,
                    const RollingTimes& rolling,
                    const uint64_t frameIndex,
                    const Clock::time_point startTime)
{
  const auto frameStart = Clock::now();

  dockWindow(renderer.getImGuiInstance(), frameIndex);
  displayStats(scene.objects.size(), rolling.times, rolling.count);
  updateObjects(scene.objects, std::chrono::duration<float>(frameStart - startTime).count());
  submitScene(renderer, scene.objects, scene.lights);

  const auto submitEnd = Clock::now();

  // Includes the frame rate limit wait and any wait on the GPU and on presentation
  renderer.render();

  const auto frameEnd = Clock::now();

  return {
    .submitMs = milliseconds(frameStart, submitEnd),
    .renderMs = milliseconds(submitEnd, frameEnd),
    .frameMs = milliseconds(frameStart, frameEnd)
  };
}

Samples runBenchmark(vke::VulkanEngine& renderer,
                     const Scene& scene,
                     const Options& options)
{
  Samples samples;
  samples.submit.reserve(options.frames);
  samples.render.reserve(options.frames);
  samples.frame.reserve(options.frames);

  RollingTimes rolling;
  uint64_t frameIndex = 0;
  const auto startTime = Clock::now();

  while (renderer.isActive() && (options.frames == 0 || samples.frame.size() < options.frames))
  {
    const FrameTimes times = runFrame(renderer, scene, rolling, frameIndex, startTime);

    rolling.times[frameIndex % ROLLING_FRAMES] = times;
    rolling.count = std::min(rolling.count + 1, ROLLING_FRAMES);

    if (frameIndex >= options.warmup && options.frames > 0)
    {
      samples.submit.push_back(times.submitMs);
      samples.render.push_back(times.renderMs);
      samples.frame.push_back(times.frameMs);
    }

    ++frameIndex;
  }

  return samples;
}

bool parseValue(const char* text,
                uint32_t& value)
{
  const std::string_view view(text);
  const auto result = std::from_chars(view.data(), view.data() + view.size(), value);
  return result.ec == std::errc() && result.ptr == view.data() + view.size();
}

bool parsePresentMode(const char* text,
                      vke::PresentMode& mode)
{
  for (const vke::PresentMode candidate : { vke::PresentMode::fifo, vke::PresentMode::fifoRelaxed, vke::PresentMode::mailbox,
                                            vke::PresentMode::immediate })
  {
    if (std::strcmp(text, presentModeName(candidate)) == 0)
    {
      mode = candidate;
      return true;
    }
  }

  return false;
}

const char* presentModeName(const vke::PresentMode mode)
{
  switch (mode)
  {
    case vke::PresentMode::fifo:
      return "fifo";
    case vke::PresentMode::fifoRelaxed:
      return "fifoRelaxed";
    case vke::PresentMode::mailbox:
      return "mailbox";
    case vke::PresentMode::immediate:
      return "immediate";
  }

  return "unknown";
}

bool parseOptions(const int argc,
                  char** argv,
                  Options& options)
{
  for (int i = 1; i < argc; ++i)
  {
    uint32_t* target = nullptr;
    if (std::strcmp(argv[i], "--objects") == 0)
    {
      target = &options.objects;
    }
    else if (std::strcmp(argv[i], "--frames") == 0)
    {
      target = &options.frames;
    }
    else if (std::strcmp(argv[i], "--warmup") == 0)
    {
      target = &options.warmup;
    }
    else if (std::strcmp(argv[i], "--frame-rate-limit") == 0)
    {
      target = &options.frameRateLimit;
    }
    else if (std::strcmp(argv[i], "--present-mode") == 0)
    {
      vke::PresentMode mode{};
      if (i + 1 >= argc || !parsePresentMode(argv[++i], mode))
      {
        return false;
      }

      options.presentMode = mode;
      continue;
    }

    if (target == nullptr || i + 1 >= argc || !parseValue(argv[++i], *target))
    {
      return false;
    }
  }

  return options.objects >= 1;
}

uint32_t gridSide(const uint32_t objects)
{
  return static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<double>(objects))));
}

vke::EngineConfig createConfig(const uint32_t objects)
{
  const float halfExtent = static_cast<float>(gridSide(objects)) * OBJECT_SPACING * 0.5f;

  return {
    .window {
      .width = 1280,
      .height = 720,
      .title = "Many Objects",
      .closeOnEscape = true
    },
    .camera {
      .position = { 0.0f, 30.0f, -(halfExtent + 20.0f) }
    },
    // A detached window would change what is rendered and presented between runs
    .imGui {
      .detachableWindows = false
    },
    .rendering {
      .rayTracingEnabled = false
    }
  };
}

std::vector<std::shared_ptr<vke::RenderObject>> createObjects(const vke::VulkanEngine& renderer,
                                                              const uint32_t objects)
{
  const auto assets = renderer.getAssetManager();

  std::array<std::shared_ptr<vke::Model>, MODEL_PATHS.size()> models;
  for (size_t i = 0; i < models.size(); ++i)
  {
    models[i] = assets->loadModel(MODEL_PATHS[i]);
  }

  std::array<std::array<std::shared_ptr<vke::Texture2D>, 2>, MATERIAL_PATHS.size()> materials;
  for (size_t i = 0; i < materials.size(); ++i)
  {
    materials[i][0] = assets->loadTexture(MATERIAL_PATHS[i][0]);
    materials[i][1] = assets->loadTexture(MATERIAL_PATHS[i][1]);
  }

  const uint32_t side = gridSide(objects);
  const float origin = static_cast<float>(side - 1) * OBJECT_SPACING * 0.5f;

  std::vector<std::shared_ptr<vke::RenderObject>> result;
  result.reserve(objects);

  for (uint32_t i = 0; i < objects; ++i)
  {
    const auto& material = materials[(i / 3) % materials.size()];
    const auto object = assets->loadRenderObject(material[0], material[1], models[i % models.size()]);
    object->setPosition({ static_cast<float>(i % side) * OBJECT_SPACING - origin, 0.0f, static_cast<float>(i / side) * OBJECT_SPACING - origin });
    result.push_back(object);
  }

  return result;
}

std::vector<std::shared_ptr<vke::Light>> createLights(const vke::VulkanEngine& renderer,
                                                      const uint32_t objects)
{
  const float spread = static_cast<float>(gridSide(objects)) * OBJECT_SPACING * 0.25f;
  const auto lighting = renderer.getLightingManager();

  // Point lights cast shadows by default
  return {
    lighting->createPointLight({ 0.0f, 8.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 0.1f, 0.5f, 1.0f),
    lighting->createPointLight({ spread, 8.0f, spread }, { 1.0f, 0.4f, 0.2f }, 0.0f, 0.5f, 1.0f),
    lighting->createPointLight({ -spread, 8.0f, -spread }, { 0.3f, 0.5f, 1.0f }, 0.0f, 0.5f, 1.0f)
  };
}

void updateObjects(const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                   const float seconds)
{
  for (size_t i = 0; i < objects.size(); ++i)
  {
    objects[i]->setOrientationEuler({ 0.0f, seconds * 20.0f + static_cast<float>(i), 0.0f });
  }
}

void submitScene(vke::VulkanEngine& renderer,
                 const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();

  for (const auto& object : objects)
  {
    r3d->renderPickableObject(object, vke::PipelineType::object);
  }

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }
}

// Placed once the first frame has built the dock tree; the default dock percents are kept
void dockWindow(const std::shared_ptr<vke::ImGuiInstance>& gui,
                const uint64_t frameIndex)
{
  if (frameIndex == 1)
  {
    gui->dockCenter("Scene View");
    gui->dockBottom("Benchmark");
  }
}

void displayStats(const size_t objectCount,
                  const std::array<FrameTimes, ROLLING_FRAMES>& rolling,
                  const size_t rollingCount)
{
  FrameTimes sum;
  for (size_t i = 0; i < rollingCount; ++i)
  {
    sum.submitMs += rolling[i].submitMs;
    sum.renderMs += rolling[i].renderMs;
    sum.frameMs += rolling[i].frameMs;
  }

  const double count = static_cast<double>(std::max<size_t>(rollingCount, 1));
  const double frameMs = sum.frameMs / count;

  ImGui::Begin("Benchmark");
  ImGui::Text("Objects: %zu", objectCount);
  ImGui::Text("Submit: %.3f ms", sum.submitMs / count);
  ImGui::Text("Render: %.3f ms", sum.renderMs / count);
  ImGui::Text("Frame: %.3f ms", frameMs);
  ImGui::Text("FPS: %.1f", frameMs > 0.0 ? 1000.0 / frameMs : 0.0);
  ImGui::End();
}

double milliseconds(const Clock::time_point from,
                    const Clock::time_point to)
{
  return std::chrono::duration<double, std::milli>(to - from).count();
}

double percentile(const std::vector<double>& sorted,
                  const double fraction)
{
  const auto index = static_cast<size_t>(std::ceil(fraction * static_cast<double>(sorted.size()))) - 1;
  return sorted[std::min(index, sorted.size() - 1)];
}

void printMetric(const char* name,
                 std::vector<double>& values)
{
  if (values.empty())
  {
    return;
  }

  std::ranges::sort(values);

  double sum = 0.0;
  for (const double value : values)
  {
    sum += value;
  }

  std::cout << std::left << std::setw(7) << name << std::right
            << " mean " << sum / static_cast<double>(values.size())
            << " min " << values.front()
            << " median " << percentile(values, 0.5)
            << " p95 " << percentile(values, 0.95)
            << " max " << values.back() << std::endl;
}

void printSummary(const Options& options,
                  const vke::RenderingManager& rendering,
                  Samples& samples)
{
  const vke::SceneViewRect sceneView = rendering.getSceneViewRect();

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "objects " << options.objects << " frames " << samples.frame.size() << " warmup " << options.warmup
            << " scene " << static_cast<int>(sceneView.width) << "x" << static_cast<int>(sceneView.height)
            << " (times in ms)\n";
  std::cout << "frame rate limit ";
  if (options.frameRateLimit == 0)
  {
    std::cout << "none";
  }
  else
  {
    std::cout << options.frameRateLimit << " fps";
  }
  std::cout << " present mode " << presentModeName(rendering.getActivePresentMode()) << " (requested "
            << presentModeName(rendering.getPresentMode()) << ")\n";
  // render() also waits on the GPU and on presentation, so a GPU-bound or vsync-limited run hides CPU changes.
  std::cout << "render includes GPU and present waits; keep the window visible and untouched while measuring\n";
  printMetric("submit", samples.submit);
  printMetric("render", samples.render);
  printMetric("frame", samples.frame);
}
