// Streams small procedural meshes while an ordinary scene renders: each frame creates a few meshes and releases the ones
// that reached their lifetime. Compares AssetManager::createMesh (queued, never waits on the GPU) with --blocking, which
// uses the synchronous createModel path.
#include <source/components/assets/AssetManager.h>
#include <source/components/assets/objects/Mesh.h>
#include <source/components/assets/objects/Model.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/assets/textures/Texture2D.h>
#include <source/components/imGui/ImGuiInstance.h>
#include <source/components/lighting/LightingManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/components/pipelines/implementations/vertexInputs/Vertex.h>
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
#include <deque>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

constexpr size_t ROLLING_FRAMES = 120;
constexpr uint32_t DRAIN_FRAME_CAP = 600;

struct Options {
  uint32_t perFrame = 8;
  uint32_t lifetime = 60;
  uint32_t frames = 0;
  uint32_t warmup = 60;
  uint32_t frameRateLimit = 0;
  bool blocking = false;
  std::optional<vke::PresentMode> presentMode;
};

struct Tracked {
  std::shared_ptr<vke::Mesh> mesh;
  std::shared_ptr<vke::Model> model;
  std::weak_ptr<void> weak;
  uint64_t createdFrame = 0;
  bool ready = false;
};

struct Stats {
  uint64_t created = 0;
  uint64_t released = 0;
  uint64_t becameReady = 0;
  uint64_t neverReady = 0;
  uint32_t minFramesToReady = UINT32_MAX;
  uint32_t maxFramesToReady = 0;
  uint32_t lastFramesToReady = 0;
  // Time inside createMesh / createModel only, not the test's own geometry generation or releases
  double createCallMs = 0.0;
};

struct FrameTimes {
  double createMs = 0.0;
  double frameMs = 0.0;
};

struct Samples {
  std::vector<double> create;
  std::vector<double> frame;
};

struct Geometry {
  std::vector<vke::Vertex> vertices;
  std::vector<uint32_t> indices;
};

struct Scene {
  std::vector<std::shared_ptr<vke::RenderObject>> objects;
  std::shared_ptr<vke::Light> light;
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

[[nodiscard]] vke::EngineConfig createConfig();

[[nodiscard]] Scene createScene(const vke::VulkanEngine& renderer);

void submitScene(vke::VulkanEngine& renderer,
                 const Scene& scene,
                 float seconds);

[[nodiscard]] Geometry createSphere(uint32_t index);

void createStreamedMesh(vke::VulkanEngine& renderer,
                        const Options& options,
                        std::deque<Tracked>& live,
                        Stats& stats,
                        uint64_t frameIndex);

void releaseDue(vke::VulkanEngine& renderer,
                const Options& options,
                std::deque<Tracked>& live,
                std::vector<Tracked>& released,
                Stats& stats,
                uint64_t frameIndex);

void observeReady(std::deque<Tracked>& live,
                  Stats& stats,
                  uint64_t frameIndex);

void dockWindow(const std::shared_ptr<vke::ImGuiInstance>& gui,
                uint64_t frameIndex);

void displayStats(const Options& options,
                  const std::array<FrameTimes, ROLLING_FRAMES>& rolling,
                  size_t rollingCount,
                  size_t liveCount,
                  const Stats& stats);

[[nodiscard]] double milliseconds(Clock::time_point from,
                                  Clock::time_point to);

[[nodiscard]] double percentile(const std::vector<double>& sorted,
                                double fraction);

void printMetric(const char* name,
                 std::vector<double>& values);

[[nodiscard]] int finish(vke::VulkanEngine& renderer,
                         const Options& options,
                         std::deque<Tracked>& live,
                         std::vector<Tracked>& released,
                         Stats& stats,
                         uint64_t frameIndex);

int main(int argc, char** argv)
{
#ifdef _MSC_VER
  // A failed assert or abort must not leave a modal dialog behind in a scripted run.
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
  for (const int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT })
  {
    _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
  }
#endif

  try
  {
    Options options;
    if (!parseOptions(argc, argv, options))
    {
      std::cerr << "Usage: meshStream [--per-frame N] [--lifetime N (frames, >= 1)] [--frames N] [--warmup N]"
                << " [--frame-rate-limit N (0 = unlimited)] [--present-mode fifo|fifoRelaxed|mailbox|immediate]"
                << " [--blocking]" << std::endl;
      return EXIT_FAILURE;
    }

    vke::VulkanEngine renderer(createConfig());

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto rendering = renderer.getRenderingManager();
    rendering->setFrameRateLimit(options.frameRateLimit);
    if (options.presentMode)
    {
      rendering->setPresentMode(*options.presentMode);
    }

    const Scene scene = createScene(renderer);

    std::deque<Tracked> live;
    std::vector<Tracked> released;
    Stats stats;
    Samples samples;
    std::array<FrameTimes, ROLLING_FRAMES> rolling {};
    size_t rollingCount = 0;
    uint64_t frameIndex = 0;
    const auto startTime = Clock::now();

    while (renderer.isActive() && (options.frames == 0 || samples.frame.size() < options.frames))
    {
      const auto frameStart = Clock::now();

      dockWindow(renderer.getImGuiInstance(), frameIndex);
      displayStats(options, rolling, rollingCount, live.size(), stats);
      submitScene(renderer, scene, std::chrono::duration<float>(frameStart - startTime).count());

      const double createCallsBefore = stats.createCallMs;

      releaseDue(renderer, options, live, released, stats, frameIndex);
      for (uint32_t i = 0; i < options.perFrame; ++i)
      {
        createStreamedMesh(renderer, options, live, stats, frameIndex);
      }

      renderer.render();

      observeReady(live, stats, frameIndex);

      const FrameTimes times {
        .createMs = stats.createCallMs - createCallsBefore,
        .frameMs = milliseconds(frameStart, Clock::now())
      };

      rolling[frameIndex % ROLLING_FRAMES] = times;
      rollingCount = std::min(rollingCount + 1, ROLLING_FRAMES);

      if (frameIndex >= options.warmup && options.frames > 0)
      {
        samples.create.push_back(times.createMs);
        samples.frame.push_back(times.frameMs);
      }

      ++frameIndex;
    }

    int exitCode = EXIT_SUCCESS;

    if (options.frames > 0)
    {
      std::cout << std::fixed << std::setprecision(3);
      std::cout << "mode " << (options.blocking ? "blocking (createModel)" : "queued (createMesh)") << " per-frame "
                << options.perFrame << " lifetime " << options.lifetime << " frames " << samples.frame.size()
                << " warmup " << options.warmup << " (times in ms)\n";
      std::cout << "frame rate limit ";
      if (options.frameRateLimit == 0)
      {
        std::cout << "none";
      }
      else
      {
        std::cout << options.frameRateLimit << " fps";
      }
      std::cout << " present mode " << presentModeName(rendering->getActivePresentMode()) << "\n";
      printMetric("create", samples.create);
      printMetric("frame", samples.frame);

      exitCode = finish(renderer, options, live, released, stats, frameIndex);
    }

    return exitCode;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}

int finish(vke::VulkanEngine& renderer,
           const Options& options,
           std::deque<Tracked>& live,
           std::vector<Tracked>& released,
           Stats& stats,
           uint64_t frameIndex)
{
  // The newest meshes are recorded in the next frame and ready in the one after; let them get there before releasing.
  for (int i = 0; i < 2 && renderer.isActive(); ++i)
  {
    renderer.render();
    observeReady(live, stats, frameIndex);
    ++frameIndex;
  }

  // Release everything still live, then run frames until the deferred destruction has let every mesh go.
  Options drain = options;
  drain.lifetime = 0;
  drain.perFrame = 0;

  uint32_t drainFrames = 0;
  while (renderer.isActive())
  {
    releaseDue(renderer, drain, live, released, stats, frameIndex);

    size_t alive = 0;
    for (const Tracked& tracked : released)
    {
      if (!tracked.weak.expired())
      {
        ++alive;
      }
    }

    if (alive == 0 || drainFrames >= DRAIN_FRAME_CAP)
    {
      break;
    }

    renderer.render();
    observeReady(live, stats, frameIndex);
    ++frameIndex;
    ++drainFrames;
  }

  size_t alive = 0;
  for (const Tracked& tracked : released)
  {
    if (!tracked.weak.expired())
    {
      ++alive;
    }
  }

  std::cout << "created " << stats.created << " released " << stats.released << " drain frames " << drainFrames << "\n";

  if (!options.blocking)
  {
    std::cout << "frames to ready min " << (stats.becameReady > 0 ? stats.minFramesToReady : 0) << " max "
              << stats.maxFramesToReady << " (ready " << stats.becameReady << ", never ready " << stats.neverReady << ")\n";
  }

  std::cout << "meshes still alive " << alive << std::endl;

  return alive == 0 && stats.neverReady == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

Geometry createSphere(const uint32_t index)
{
  // A few hundred to about two thousand vertices.
  const uint32_t segments = 8 + (index * 7) % 34;
  const uint32_t rings = 6 + (index * 5) % 28;
  constexpr float PI = std::numbers::pi_v<float>;

  Geometry geometry;
  geometry.vertices.reserve(static_cast<size_t>(rings + 1) * (segments + 1));
  geometry.indices.reserve(static_cast<size_t>(rings) * segments * 6);

  for (uint32_t ring = 0; ring <= rings; ++ring)
  {
    const float v = static_cast<float>(ring) / static_cast<float>(rings);
    const float polar = v * PI;

    for (uint32_t segment = 0; segment <= segments; ++segment)
    {
      const float u = static_cast<float>(segment) / static_cast<float>(segments);
      const float azimuth = u * 2.0f * PI;
      const glm::vec3 normal { std::sin(polar) * std::cos(azimuth), std::cos(polar), std::sin(polar) * std::sin(azimuth) };

      vke::Vertex vertex {};
      vertex.pos = normal * 0.3f;
      vertex.normal = normal;
      vertex.texCoord = { u, v };
      geometry.vertices.push_back(vertex);
    }
  }

  const uint32_t stride = segments + 1;
  for (uint32_t ring = 0; ring < rings; ++ring)
  {
    for (uint32_t segment = 0; segment < segments; ++segment)
    {
      const uint32_t a = ring * stride + segment;
      const uint32_t b = a + stride;

      geometry.indices.insert(geometry.indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
    }
  }

  return geometry;
}

void createStreamedMesh(vke::VulkanEngine& renderer,
                        const Options& options,
                        std::deque<Tracked>& live,
                        Stats& stats,
                        const uint64_t frameIndex)
{
  const auto assets = renderer.getAssetManager();
  const Geometry geometry = createSphere(static_cast<uint32_t>(stats.created));

  Tracked tracked;
  tracked.createdFrame = frameIndex;

  if (options.blocking)
  {
    const auto start = Clock::now();
    tracked.model = assets->createModel(geometry.vertices, geometry.indices);
    stats.createCallMs += milliseconds(start, Clock::now());
    tracked.weak = tracked.model;
    tracked.ready = true;
  }
  else
  {
    const vke::MeshCreateInfo createInfo {
      .vertexData = std::as_bytes(std::span<const vke::Vertex>(geometry.vertices)),
      .vertexStride = sizeof(vke::Vertex),
      .positionOffset = offsetof(vke::Vertex, pos),
      .indices = geometry.indices,
      .buildBLAS = stats.created % 2 == 0
    };

    const auto start = Clock::now();
    tracked.mesh = assets->createMesh(createInfo);
    stats.createCallMs += milliseconds(start, Clock::now());
    tracked.weak = tracked.mesh;
  }

  live.push_back(std::move(tracked));
  ++stats.created;
}

void releaseDue(vke::VulkanEngine& renderer,
                const Options& options,
                std::deque<Tracked>& live,
                std::vector<Tracked>& released,
                Stats& stats,
                const uint64_t frameIndex)
{
  const auto assets = renderer.getAssetManager();

  while (!live.empty() && frameIndex >= live.front().createdFrame + options.lifetime)
  {
    Tracked tracked = std::move(live.front());
    live.pop_front();

    if (!tracked.ready)
    {
      ++stats.neverReady;
    }

    if (tracked.mesh)
    {
      assets->release(std::move(tracked.mesh));
    }

    if (tracked.model)
    {
      assets->release(std::move(tracked.model));
    }

    released.push_back(std::move(tracked));
    ++stats.released;
  }

  // Only the weak pointers of long-expired meshes are dropped, keeping the list short in a long run.
  if (released.size() > 4096)
  {
    std::erase_if(released, [](const Tracked& tracked) {
      return tracked.weak.expired();
    });
  }
}

void observeReady(std::deque<Tracked>& live,
                  Stats& stats,
                  const uint64_t frameIndex)
{
  for (Tracked& tracked : live)
  {
    if (tracked.ready || !tracked.mesh->isReady())
    {
      continue;
    }

    tracked.ready = true;

    const auto frames = static_cast<uint32_t>(frameIndex - tracked.createdFrame);
    stats.minFramesToReady = std::min(stats.minFramesToReady, frames);
    stats.maxFramesToReady = std::max(stats.maxFramesToReady, frames);
    stats.lastFramesToReady = frames;
    ++stats.becameReady;
  }
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
    if (std::strcmp(argv[i], "--per-frame") == 0)
    {
      target = &options.perFrame;
    }
    else if (std::strcmp(argv[i], "--lifetime") == 0)
    {
      target = &options.lifetime;
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
    else if (std::strcmp(argv[i], "--blocking") == 0)
    {
      options.blocking = true;
      continue;
    }
    else if (std::strcmp(argv[i], "--present-mode") == 0)
    {
      vke::PresentMode mode {};
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

  return options.lifetime >= 1;
}

vke::EngineConfig createConfig()
{
  return {
    .window {
      .width = 1280,
      .height = 720,
      .title = "Mesh Stream",
      .closeOnEscape = true
    },
    .camera {
      .position = { 0.0f, 3.0f, -9.0f }
    },
    // A detached window would change what is rendered and presented between runs
    .imGui {
      .detachableWindows = false
    }
  };
}

Scene createScene(const vke::VulkanEngine& renderer)
{
  const auto assets = renderer.getAssetManager();

  const auto model = assets->loadModel("assets/models/cube.obj");
  const auto texture = assets->loadTexture("assets/textures/container.png");
  const auto specular = assets->loadTexture("assets/textures/container_specular.png");

  Scene scene;

  for (int i = 0; i < 5; ++i)
  {
    const auto object = assets->loadRenderObject(texture, specular, model);
    object->setPosition({ static_cast<float>(i - 2) * 2.5f, 0.0f, 0.0f });
    scene.objects.push_back(object);
  }

  scene.light = renderer.getLightingManager()->createPointLight({ 0.0f, 5.0f, -3.0f }, { 1.0f, 1.0f, 1.0f }, 0.1f, 0.6f, 1.0f);

  return scene;
}

void submitScene(vke::VulkanEngine& renderer,
                 const Scene& scene,
                 const float seconds)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();

  for (size_t i = 0; i < scene.objects.size(); ++i)
  {
    scene.objects[i]->setOrientationEuler({ 0.0f, seconds * 20.0f + static_cast<float>(i) * 30.0f, 0.0f });
    r3d->renderPickableObject(scene.objects[i], vke::PipelineType::object);
  }

  renderer.getLightingManager()->renderLight(scene.light);
}

// Placed once the first frame has built the dock tree; the default dock percents are kept
void dockWindow(const std::shared_ptr<vke::ImGuiInstance>& gui,
                const uint64_t frameIndex)
{
  if (frameIndex == 1)
  {
    gui->dockCenter("Scene View");
    gui->dockBottom("Mesh Stream");
  }
}

void displayStats(const Options& options,
                  const std::array<FrameTimes, ROLLING_FRAMES>& rolling,
                  const size_t rollingCount,
                  const size_t liveCount,
                  const Stats& stats)
{
  FrameTimes sum;
  for (size_t i = 0; i < rollingCount; ++i)
  {
    sum.createMs += rolling[i].createMs;
    sum.frameMs += rolling[i].frameMs;
  }

  const double count = static_cast<double>(std::max<size_t>(rollingCount, 1));
  const double frameMs = sum.frameMs / count;

  ImGui::Begin("Mesh Stream");
  ImGui::Text("Mode: %s", options.blocking ? "blocking (createModel)" : "queued (createMesh)");
  ImGui::Text("Live: %zu  created: %llu", liveCount, static_cast<unsigned long long>(stats.created));
  ImGui::Text("Create: %.3f ms per frame", sum.createMs / count);
  ImGui::Text("Frame: %.3f ms", frameMs);
  ImGui::Text("FPS: %.1f", frameMs > 0.0 ? 1000.0 / frameMs : 0.0);
  if (!options.blocking)
  {
    ImGui::Text("Frames to ready: last %u, max %u", stats.lastFramesToReady, stats.maxFramesToReady);
  }
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

  std::cout << std::left << std::setw(7) << name << std::right
            << " p50 " << percentile(values, 0.5)
            << " p99 " << percentile(values, 0.99)
            << " max " << values.back() << std::endl;
}
