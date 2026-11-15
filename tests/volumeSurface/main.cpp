// A volume surface over an in-memory distance field the app edits: hold the left mouse button over the scene to add
// material with a sphere brush (Shift, or the Remove box, carves instead). With --frames N it runs scripted brush edits
// in bursts instead, prints frame time percentiles and how long edits took to show, and exits non-zero if the surface
// never settled. The field's lock makes each edit and its invalidate call atomic to the engine's worker threads (see
// vke::VolumeSource).
#include <source/components/assets/AssetManager.h>
#include <source/components/assets/volume/VolumeSource.h>
#include <source/components/assets/volume/VolumeSurface.h>
#include <source/components/camera/Camera.h>
#include <source/components/imGui/ImGuiInstance.h>
#include <source/components/renderingManager/RenderingManager.h>
#include <source/components/renderingManager/renderer3D/Renderer3D.h>
#include <source/components/window/PresentMode.h>
#include <source/VulkanEngine.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vector_relational.hpp>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace {

  constexpr float CELL_SIZE = 0.25f;
  constexpr float FIELD_OF_VIEW = 60.0f;
  constexpr uint32_t SETTLE_FRAME_CAP = 600;
  constexpr uint8_t BRUSH_MATERIAL = 3;

  const vke::VolumeRegion BOUNDS { { 0, 0, 0 }, { 129, 65, 129 } };

  using Clock = std::chrono::steady_clock;

  struct Options {
    uint32_t frames = 0;
    uint32_t warmup = 60;
    uint32_t frameRateLimit = 0;
    uint32_t burst = 30;
    uint32_t pause = 90;
    std::optional<vke::PresentMode> presentMode;
  };

  struct Brush {
    // In cells
    glm::vec3 center { 0.0f };
    float radius = 6.0f;
    bool add = true;
  };

  struct Ray {
    glm::vec3 origin { 0.0f };
    glm::vec3 direction { 0.0f, 0.0f, 1.0f };
  };

  [[nodiscard]] float groundHeight(const float x,
                                   const float z)
  {
    const float hill = 7.0f * std::exp(-((x - 30.0f) * (x - 30.0f) + (z - 90.0f) * (z - 90.0f)) / 450.0f);

    return 24.0f + 4.0f * std::sin(x * 0.08f) * std::cos(z * 0.07f) + hill;
  }

  // The ground with a cave under the first hill, in cells and negative inside
  [[nodiscard]] float initialDistance(const glm::vec3 p)
  {
    const float ground = p.y - groundHeight(p.x, p.z);
    const float cave = glm::length((p - glm::vec3(44.0f, 22.0f, 64.0f)) * glm::vec3(1.0f, 1.3f, 1.0f)) - 12.0f;

    return std::max(ground, -cave);
  }

  [[nodiscard]] uint8_t initialMaterial(const glm::vec3 p)
  {
    if (p.y < 16.0f)
    {
      return 0;
    }

    return p.y < 27.0f ? 1 : 2;
  }

  // The samples the surface pulls from, behind a lock that makes an edit and its invalidate call one step for fill()
  class GridSource final : public vke::VolumeSource {
  public:
    GridSource()
      : m_size(vke::extent(BOUNDS)),
        m_distance(vke::sampleCount(BOUNDS)),
        m_material(vke::sampleCount(BOUNDS))
    {
      for (int z = 0; z < m_size.z; ++z)
      {
        for (int y = 0; y < m_size.y; ++y)
        {
          for (int x = 0; x < m_size.x; ++x)
          {
            const glm::vec3 p(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

            m_distance[indexOf({ x, y, z })] = initialDistance(p);
            m_material[indexOf({ x, y, z })] = initialMaterial(p);
          }
        }
      }
    }

    void fill(const vke::VolumeRegion& region,
              uint32_t,
              std::span<float> distance,
              std::span<uint8_t> material) const override
    {
      const std::shared_lock lock(m_mutex);

      const glm::ivec3 size = vke::extent(region);
      size_t out = 0;

      for (int z = 0; z < size.z; ++z)
      {
        for (int y = 0; y < size.y; ++y)
        {
          const size_t row = indexOf(region.min + glm::ivec3(0, y, z));

          std::copy_n(m_distance.begin() + static_cast<std::ptrdiff_t>(row), size.x, distance.begin() + static_cast<std::ptrdiff_t>(out));
          std::copy_n(m_material.begin() + static_cast<std::ptrdiff_t>(row), size.x, material.begin() + static_cast<std::ptrdiff_t>(out));

          out += static_cast<size_t>(size.x);
        }
      }
    }

    [[nodiscard]] vke::VolumeRegion bounds() const override
    {
      return BOUNDS;
    }

    // Changes the field and reports it while holding the lock exclusively
    void edit(const Brush& brush,
              vke::VolumeSurface& surface)
    {
      const std::unique_lock lock(m_mutex);

      // The surface needs the samples around a change as well, which the engine adds; this box is the changed samples
      const glm::ivec3 low = glm::ivec3(glm::floor(brush.center - brush.radius)) - 1;
      const glm::ivec3 high = glm::ivec3(glm::ceil(brush.center + brush.radius)) + 2;
      const vke::VolumeRegion changed = vke::intersect({ low, high }, BOUNDS);

      for (int z = changed.min.z; z < changed.max.z; ++z)
      {
        for (int y = changed.min.y; y < changed.max.y; ++y)
        {
          for (int x = changed.min.x; x < changed.max.x; ++x)
          {
            applyBrush(brush, { x, y, z });
          }
        }
      }

      surface.invalidate(changed);
    }

    // Sphere traces the field; the hit in cells. Safe from the frame thread while no edit is running.
    [[nodiscard]] std::optional<glm::vec3> raycast(const Ray& ray) const
    {
      const std::shared_lock lock(m_mutex);

      float travelled = 0.0f;

      for (int step = 0; step < 600 && travelled < 400.0f; ++step)
      {
        const glm::vec3 p = ray.origin + ray.direction * travelled;

        if (glm::any(glm::lessThan(p, glm::vec3(0.0f))) || glm::any(glm::greaterThan(p, glm::vec3(BOUNDS.max - 1))))
        {
          // Outside the volume: march to its box rather than stop
          travelled += 0.5f;
          continue;
        }

        const float distance = trilinear(p);

        if (distance < 0.02f)
        {
          return p;
        }

        travelled += std::max(distance * 0.9f, 0.02f);
      }

      return std::nullopt;
    }

  private:
    mutable std::shared_mutex m_mutex;
    glm::ivec3 m_size;
    std::vector<float> m_distance;
    std::vector<uint8_t> m_material;

    [[nodiscard]] size_t indexOf(const glm::ivec3 position) const
    {
      return static_cast<size_t>(position.x) +
             static_cast<size_t>(m_size.x) * (static_cast<size_t>(position.y) + static_cast<size_t>(m_size.y) * static_cast<size_t>(position.z));
    }

    void applyBrush(const Brush& brush,
                    const glm::ivec3 position)
    {
      const size_t index = indexOf(position);
      const float brushDistance = glm::length(glm::vec3(position) - brush.center) - brush.radius;

      if (brush.add)
      {
        if (brushDistance < m_distance[index])
        {
          m_distance[index] = brushDistance;
          m_material[index] = BRUSH_MATERIAL;
        }
      }
      else
      {
        m_distance[index] = std::max(m_distance[index], -brushDistance);
      }
    }

    [[nodiscard]] float trilinear(const glm::vec3 p) const
    {
      const glm::ivec3 base = glm::min(glm::ivec3(glm::floor(p)), BOUNDS.max - 2);
      const glm::vec3 f = p - glm::vec3(base);
      float result = 0.0f;

      for (int corner = 0; corner < 8; ++corner)
      {
        const glm::ivec3 offset(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
        const float weight = (offset.x != 0 ? f.x : 1.0f - f.x) * (offset.y != 0 ? f.y : 1.0f - f.y) * (offset.z != 0 ? f.z : 1.0f - f.z);

        result += weight * m_distance[indexOf(base + offset)];
      }

      return result;
    }
  };

  struct Stats {
    uint64_t edits = 0;
    uint32_t maxSettleFrames = 0;
    uint32_t firstSettledFrame = 0;
    bool seenSettled = false;
    // Frames of edits not yet followed by a settled surface
    std::vector<uint64_t> outstanding;
  };

  struct Session {
    std::shared_ptr<GridSource> source;
    std::shared_ptr<vke::VolumeSurface> surface;
    Brush brush;
    bool remove = false;
    float frameMs = 0.0f;
    Stats stats;
  };

  [[nodiscard]] bool parseValue(const char* text,
                                uint32_t& value)
  {
    const std::string_view view(text);
    const auto result = std::from_chars(view.data(), view.data() + view.size(), value);

    return result.ec == std::errc() && result.ptr == view.data() + view.size();
  }

  [[nodiscard]] const char* presentModeName(const vke::PresentMode mode)
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

  [[nodiscard]] bool parsePresentMode(const char* text,
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

  [[nodiscard]] uint32_t* optionTarget(Options& options,
                                       const char* name)
  {
    if (std::strcmp(name, "--frames") == 0)
    {
      return &options.frames;
    }

    if (std::strcmp(name, "--warmup") == 0)
    {
      return &options.warmup;
    }

    if (std::strcmp(name, "--frame-rate-limit") == 0)
    {
      return &options.frameRateLimit;
    }

    if (std::strcmp(name, "--burst") == 0)
    {
      return &options.burst;
    }

    return std::strcmp(name, "--pause") == 0 ? &options.pause : nullptr;
  }

  [[nodiscard]] bool parseOptions(const int argc,
                                  char** argv,
                                  Options& options)
  {
    for (int i = 1; i < argc; ++i)
    {
      if (std::strcmp(argv[i], "--present-mode") == 0)
      {
        vke::PresentMode mode {};

        if (i + 1 >= argc || !parsePresentMode(argv[++i], mode))
        {
          return false;
        }

        options.presentMode = mode;
        continue;
      }

      uint32_t* target = optionTarget(options, argv[i]);

      if (target == nullptr || i + 1 >= argc || !parseValue(argv[++i], *target))
      {
        return false;
      }
    }

    return options.burst >= 1;
  }

  [[nodiscard]] vke::EngineConfig createConfig()
  {
    return {
      .window {
        .width = 1280,
        .height = 720,
        .title = "Volume Surface",
        .closeOnEscape = true
      },
      .camera {
        .position = { 16.0f, 9.5f, -8.0f },
        .speed = 5.0f
      },
      // A detached window would change what is rendered and presented between runs
      .imGui {
        .detachableWindows = false
      }
    };
  }

  // The ray under the cursor, in cells
  [[nodiscard]] Ray cursorRay(const vke::SceneViewRect& rect,
                              const ImVec2 mouse,
                              const glm::mat4& view)
  {
    const float aspect = rect.width / std::max(rect.height, 1.0f);
    const float tangent = std::tan(glm::radians(FIELD_OF_VIEW) * 0.5f);
    const float ndcX = (mouse.x - rect.x) / std::max(rect.width, 1.0f) * 2.0f - 1.0f;
    const float ndcY = (mouse.y - rect.y) / std::max(rect.height, 1.0f) * 2.0f - 1.0f;

    const glm::vec3 viewDirection(ndcX * tangent * aspect, -ndcY * tangent, -1.0f);
    const glm::mat4 inverseView = glm::inverse(view);

    return {
      .origin = glm::vec3(inverseView[3]) / CELL_SIZE,
      .direction = glm::normalize(glm::vec3(inverseView * glm::vec4(viewDirection, 0.0f)))
    };
  }

  [[nodiscard]] bool cursorInside(const vke::SceneViewRect& rect,
                                  const ImVec2 mouse)
  {
    return mouse.x >= rect.x && mouse.y >= rect.y && mouse.x < rect.x + rect.width && mouse.y < rect.y + rect.height;
  }

  void applyMouseEdit(vke::VulkanEngine& renderer,
                      Session& session)
  {
    const auto rendering = renderer.getRenderingManager();
    const vke::SceneViewRect rect = rendering->getSceneViewRect();
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    if (!rendering->isSceneHovered() || !cursorInside(rect, mouse))
    {
      return;
    }

    const Ray ray = cursorRay(rect, mouse, renderer.getCamera()->getViewMatrix());
    const std::optional<glm::vec3> hit = session.source->raycast(ray);

    if (!hit)
    {
      return;
    }

    session.brush.center = *hit;
    session.brush.add = !(session.remove || ImGui::GetIO().KeyShift);

    // Where the brush would land, drawn as the engine's debug sphere
    renderer.getRenderingManager()->getRenderer3D()->renderSphere(*hit * CELL_SIZE, session.brush.radius * CELL_SIZE,
      { .color = session.brush.add ? glm::vec4(0.2f, 1.0f, 0.2f, 1.0f) : glm::vec4(1.0f, 0.3f, 0.2f, 1.0f), .depth = vke::DebugDepth::xray });

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
      session.source->edit(session.brush, *session.surface);
      ++session.stats.edits;
    }
  }

  // Bursts of one edit per frame separated by pauses, alternating adding and carving along the same path
  [[nodiscard]] std::optional<Brush> scriptedBrush(const Options& options,
                                                   const uint64_t frameIndex)
  {
    if (frameIndex < options.warmup)
    {
      return std::nullopt;
    }

    const uint64_t scripted = frameIndex - options.warmup;
    const uint64_t period = static_cast<uint64_t>(options.burst) + options.pause;
    const uint64_t local = scripted % period;
    const uint64_t burstIndex = scripted / period;

    if (local >= options.burst)
    {
      return std::nullopt;
    }

    const float angle = static_cast<float>(burstIndex / 2) * 0.9f + static_cast<float>(local) * 0.06f;
    const float x = 64.0f + 34.0f * std::cos(angle);
    const float z = 64.0f + 34.0f * std::sin(angle);

    return Brush { .center = { x, groundHeight(x, z), z }, .radius = 6.0f, .add = burstIndex % 2 == 0 };
  }

  void runScriptedEdit(const Options& options,
                       Session& session,
                       const uint64_t frameIndex)
  {
    const std::optional<Brush> brush = scriptedBrush(options, frameIndex);

    if (!brush)
    {
      return;
    }

    session.brush = *brush;
    session.source->edit(*brush, *session.surface);

    ++session.stats.edits;
    session.stats.outstanding.push_back(frameIndex);
  }

  // After the frame: how many frames the edits waited to show, once the surface is fully shown again
  void observeSettled(Session& session,
                      const uint64_t frameIndex)
  {
    Stats& stats = session.stats;

    if (session.surface->pendingFraction() != 0.0f)
    {
      return;
    }

    if (!stats.seenSettled)
    {
      stats.seenSettled = true;
      stats.firstSettledFrame = static_cast<uint32_t>(frameIndex);
    }

    for (const uint64_t editFrame : stats.outstanding)
    {
      stats.maxSettleFrames = std::max(stats.maxSettleFrames, static_cast<uint32_t>(frameIndex - editFrame));
    }

    stats.outstanding.clear();
  }

  void dockWindow(const std::shared_ptr<vke::ImGuiInstance>& gui,
                  const uint64_t frameIndex)
  {
    if (frameIndex == 1)
    {
      gui->dockCenter("Scene View");
      gui->dockBottom("Volume Surface");
    }
  }

  void displayWindow(const Options& options,
                     Session& session)
  {
    ImGui::Begin("Volume Surface");
    ImGui::Text("Pending: %.0f%%", static_cast<double>(session.surface->pendingFraction()) * 100.0);
    ImGui::Text("Chunks shown: %zu", session.surface->getDisplayedChunks().size());
    ImGui::SliderFloat("Brush radius (cells)", &session.brush.radius, 2.0f, 20.0f);
    ImGui::Checkbox("Remove (or hold Shift)", &session.remove);
    ImGui::Text("Edits: %llu", static_cast<unsigned long long>(session.stats.edits));
    ImGui::Text("Frame: %.3f ms (%.1f FPS)", static_cast<double>(session.frameMs),
                session.frameMs > 0.0f ? static_cast<double>(1000.0f / session.frameMs) : 0.0);
    ImGui::TextUnformatted(options.frames > 0 ? "Scripted edits" : "Hold the left mouse button over the scene to edit");
    ImGui::End();
  }

  [[nodiscard]] double milliseconds(const Clock::time_point from,
                                    const Clock::time_point to)
  {
    return std::chrono::duration<double, std::milli>(to - from).count();
  }

  [[nodiscard]] double percentile(const std::vector<double>& sorted,
                                  const double fraction)
  {
    const auto index = static_cast<size_t>(std::ceil(fraction * static_cast<double>(sorted.size()))) - 1;

    return sorted[std::min(index, sorted.size() - 1)];
  }

  void runFrame(vke::VulkanEngine& renderer,
                const Options& options,
                Session& session,
                const uint64_t frameIndex,
                const bool scripted)
  {
    dockWindow(renderer.getImGuiInstance(), frameIndex);
    displayWindow(options, session);

    const auto r3d = renderer.getRenderingManager()->getRenderer3D();
    // The surface's local space is already in world units (cells times the cell size)
    r3d->renderVolumeSurface(session.surface, glm::mat4(1.0f));

    if (options.frames == 0)
    {
      applyMouseEdit(renderer, session);
    }
    else if (scripted)
    {
      runScriptedEdit(options, session, frameIndex);
    }

    renderer.render();

    observeSettled(session, frameIndex);
  }

  struct RunResult {
    std::vector<double> frameTimes;
    uint64_t frameIndex = 0;
  };

  [[nodiscard]] RunResult runLoop(vke::VulkanEngine& renderer,
                                  const Options& options,
                                  Session& session)
  {
    RunResult result;

    while (renderer.isActive() && (options.frames == 0 || result.frameTimes.size() < options.frames))
    {
      const auto start = Clock::now();

      runFrame(renderer, options, session, result.frameIndex, true);

      session.frameMs = static_cast<float>(milliseconds(start, Clock::now()));

      if (result.frameIndex >= options.warmup && options.frames > 0)
      {
        result.frameTimes.push_back(session.frameMs);
      }

      ++result.frameIndex;
    }

    return result;
  }

  // No more edits: the surface must show everything within a bounded number of frames
  [[nodiscard]] bool settle(vke::VulkanEngine& renderer,
                            const Options& options,
                            Session& session,
                            uint64_t& frameIndex)
  {
    for (uint32_t i = 0; i < SETTLE_FRAME_CAP && renderer.isActive(); ++i)
    {
      runFrame(renderer, options, session, frameIndex++, false);

      if (session.surface->pendingFraction() == 0.0f)
      {
        return true;
      }
    }

    return false;
  }

  void printSummary(const Options& options,
                    const vke::RenderingManager& rendering,
                    const Session& session,
                    std::vector<double>& frameTimes)
  {
    std::ranges::sort(frameTimes);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "frames " << frameTimes.size() << " warmup " << options.warmup << " burst " << options.burst << " pause "
              << options.pause << " (times in ms)\n";
    std::cout << "frame rate limit ";

    if (options.frameRateLimit == 0)
    {
      std::cout << "none";
    }
    else
    {
      std::cout << options.frameRateLimit << " fps";
    }

    std::cout << " present mode " << presentModeName(rendering.getActivePresentMode()) << "\n";

    if (!frameTimes.empty())
    {
      std::cout << "frame p50 " << percentile(frameTimes, 0.5) << " p99 " << percentile(frameTimes, 0.99) << " max "
                << frameTimes.back() << "\n";
    }

    std::cout << "edits " << session.stats.edits << " first settled at frame " << session.stats.firstSettledFrame
              << " max frames from an edit to a settled surface " << session.stats.maxSettleFrames << "\n";
    std::cout << "chunks shown " << session.surface->getDisplayedChunks().size() << std::endl;
  }

  [[nodiscard]] vke::VolumeSurfaceDescription createDescription()
  {
    return { .cellSize = CELL_SIZE, .chunkCells = 32 };
  }

  [[nodiscard]] int run(const Options& options)
  {
    vke::VulkanEngine renderer(createConfig());

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    const auto rendering = renderer.getRenderingManager();
    rendering->setFrameRateLimit(options.frameRateLimit);

    if (options.presentMode)
    {
      rendering->setPresentMode(*options.presentMode);
    }

    rendering->getRenderer3D()->setProjectionParameters(FIELD_OF_VIEW, 0.1f, 500.0f);

    const auto assets = renderer.getAssetManager();

    Session session;
    session.source = std::make_shared<GridSource>();
    session.surface = assets->createVolumeSurface(createDescription(), session.source);

    RunResult result = runLoop(renderer, options, session);

    int exitCode = EXIT_SUCCESS;

    if (options.frames > 0)
    {
      const bool settled = settle(renderer, options, session, result.frameIndex);

      printSummary(options, *rendering, session, result.frameTimes);

      std::cout << "settled after the last edit: " << (settled ? "yes" : "NO") << std::endl;

      if (!settled || session.surface->getDisplayedChunks().empty())
      {
        exitCode = EXIT_FAILURE;
      }
    }

    assets->release(std::move(session.surface));

    return exitCode;
  }

} // namespace

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
      std::cerr << "Usage: volumeSurface [--frames N] [--warmup N] [--burst N (>= 1)] [--pause N]"
                << " [--frame-rate-limit N (0 = unlimited)] [--present-mode fifo|fifoRelaxed|mailbox|immediate]" << std::endl;
      return EXIT_FAILURE;
    }

    return run(options);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
