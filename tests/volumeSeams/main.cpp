// Volume surfaces placed edge to edge in a row along x over one curved field (rolling hills), each with its own source
// over bounds that share their border sample with the next tile. Each source reports sample bounds one sample wider on
// every side that borders another tile, so the shared borders shade identically on both sides. --no-apron reports the
// default sample bounds instead, which leaves the normals at the shared borders one-sided: a visible seam. The scene is
// rasterized with one light high above, from a fixed camera, and --frames N runs N frames, prints where the seams are
// on screen and exits non-zero if a surface never settled.
#include <source/components/assets/AssetManager.h>
#include <source/components/assets/volume/VolumeSource.h>
#include <source/components/assets/volume/VolumeSurface.h>
#include <source/components/camera/Camera.h>
#include <source/components/lighting/LightingManager.h>
#include <source/components/lighting/lights/Light.h>
#include <source/components/renderingManager/RenderingManager.h>
#include <source/components/renderingManager/renderer3D/Renderer3D.h>
#include <source/VulkanEngine.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

  constexpr float CELL_SIZE = 0.25f;
  constexpr float FIELD_OF_VIEW = 60.0f;
  constexpr float TILT_DEGREES = -50.0f;
  constexpr uint32_t WINDOW_WIDTH = 1280;
  constexpr uint32_t WINDOW_HEIGHT = 720;
  constexpr uint32_t SETTLE_FRAME_CAP = 600;
  // Frames to keep rendering once every surface has settled, so the capture shows the final meshes
  constexpr uint32_t SHOW_FRAMES = 10;
  constexpr uint32_t MAX_TILES = 8;
  constexpr uint32_t PRINTED_ROWS = 5;

  // Cells per tile along x, and the samples of a tile along y and z
  constexpr int TILE_CELLS = 48;
  constexpr int SAMPLES_Y = 33;
  constexpr int SAMPLES_Z = 65;

  struct Options {
    uint32_t frames = 0;
    uint32_t tiles = 3;
    bool apron = true;
  };

  // The terrain height in cells. Two waves along x per tile put every seam at the same phase, on a steep slope, where
  // the normals of a one-sided difference are far from the field's.
  [[nodiscard]] float terrainHeight(const float x,
                                    const float z)
  {
    constexpr float waveNumber = 6.2831853f * 2.0f / static_cast<float>(TILE_CELLS);

    return 14.0f + 5.0f * std::sin(waveNumber * x + 0.35f) * (0.7f + 0.3f * std::cos(0.17f * z)) +
           3.0f * std::sin(0.09f * x + 0.23f * z);
  }

  // One tile of the field: a pure function of the sample position, so it answers any sample, including the ones of a
  // neighboring tile.
  class TileSource final : public vke::VolumeSource {
  public:
    TileSource(const vke::VolumeRegion& bounds,
               const vke::VolumeRegion& sampleBounds)
      : m_bounds(bounds),
        m_sampleBounds(sampleBounds)
    {}

    void fill(const vke::VolumeRegion& region,
              uint32_t,
              std::span<float> distance,
              std::span<uint8_t> material) const override
    {
      const glm::ivec3 size = vke::extent(region);
      size_t out = 0;

      for (int z = 0; z < size.z; ++z)
      {
        for (int y = 0; y < size.y; ++y)
        {
          for (int x = 0; x < size.x; ++x, ++out)
          {
            const glm::ivec3 position = region.min + glm::ivec3(x, y, z);

            distance[out] = static_cast<float>(position.y) -
                            terrainHeight(static_cast<float>(position.x), static_cast<float>(position.z));
            material[out] = 0;
          }
        }
      }
    }

    [[nodiscard]] vke::VolumeRegion bounds() const override
    {
      return m_bounds;
    }

    [[nodiscard]] vke::VolumeRegion sampleBounds() const override
    {
      return m_sampleBounds;
    }

  private:
    vke::VolumeRegion m_bounds;
    vke::VolumeRegion m_sampleBounds;
  };

  [[nodiscard]] bool parseValue(const char* text,
                                uint32_t& value)
  {
    const std::string_view view(text);
    const auto result = std::from_chars(view.data(), view.data() + view.size(), value);

    return result.ec == std::errc() && result.ptr == view.data() + view.size();
  }

  [[nodiscard]] bool parseOptions(const int argc,
                                  char** argv,
                                  Options& options)
  {
    for (int i = 1; i < argc; ++i)
    {
      if (std::strcmp(argv[i], "--no-apron") == 0)
      {
        options.apron = false;
        continue;
      }

      uint32_t* target = nullptr;

      if (std::strcmp(argv[i], "--frames") == 0)
      {
        target = &options.frames;
      }
      else if (std::strcmp(argv[i], "--tiles") == 0)
      {
        target = &options.tiles;
      }

      if (target == nullptr || i + 1 >= argc || !parseValue(argv[++i], *target))
      {
        return false;
      }
    }

    return options.tiles >= 2 && options.tiles <= MAX_TILES;
  }

  // The terrain's local space is in world units: cells times the cell size. The model matrix tilts the terrain about
  // its center and puts that center in front of the camera, which looks along +z; the tilt shows it from above at an
  // angle, with the seams (planes of constant x) running up and down the screen.
  struct Scene {
    glm::vec3 center { 0.0f };
    glm::mat4 model { 1.0f };
    glm::vec3 cameraPosition { 0.0f };
    glm::mat4 view { 1.0f };
  };

  [[nodiscard]] Scene createScene(const Options& options)
  {
    Scene scene;

    const float width = static_cast<float>(options.tiles * TILE_CELLS) * CELL_SIZE;
    const float depth = static_cast<float>(SAMPLES_Z - 1) * CELL_SIZE;
    scene.center = { width * 0.5f, 14.0f * CELL_SIZE, depth * 0.5f };

    const float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
    const float tangent = std::tan(glm::radians(FIELD_OF_VIEW) * 0.5f);
    const float distance = width * 0.5f / (tangent * aspect) * 1.12f + depth * 0.5f;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, distance));
    model = glm::rotate(model, glm::radians(TILT_DEGREES), glm::vec3(1.0f, 0.0f, 0.0f));
    scene.model = glm::translate(model, -scene.center);

    return scene;
  }

  [[nodiscard]] vke::EngineConfig createConfig()
  {
    return {
      .window {
        .width = WINDOW_WIDTH,
        .height = WINDOW_HEIGHT,
        .title = "Volume Seams",
        .closeOnEscape = true
      },
      .camera {
        .position = { 0.0f, 0.0f, 0.0f },
        .speed = 5.0f
      },
      // The scene fills the window, so a screen position is a fraction of it
      .imGui {
        .useDockspace = false,
        .detachableWindows = false
      },
      .rendering {
        .rayTracingEnabled = false
      }
    };
  }

  struct Tile {
    std::shared_ptr<TileSource> source;
    std::shared_ptr<vke::VolumeSurface> surface;
  };

  [[nodiscard]] vke::VolumeRegion tileBounds(const int index)
  {
    return { { index * TILE_CELLS, 0, 0 }, { (index + 1) * TILE_CELLS + 1, SAMPLES_Y, SAMPLES_Z } };
  }

  [[nodiscard]] vke::VolumeRegion tileSampleBounds(const Options& options,
                                                   const int index)
  {
    vke::VolumeRegion sampleBounds = tileBounds(index);

    if (options.apron)
    {
      sampleBounds.min.x -= index > 0 ? 1 : 0;
      sampleBounds.max.x += index + 1 < static_cast<int>(options.tiles) ? 1 : 0;
    }

    return sampleBounds;
  }

  [[nodiscard]] bool allSettled(const std::vector<Tile>& tiles)
  {
    return std::ranges::all_of(tiles, [](const Tile& tile) {
      return tile.surface->pendingFraction() == 0.0f && !tile.surface->getDisplayedChunks().empty();
    });
  }

  // Where a point of the terrain lands on screen, as fractions of the window (0 to 1, y down)
  [[nodiscard]] glm::vec2 project(const Scene& scene,
                                  const glm::vec3 local)
  {
    const float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
    const float tangent = std::tan(glm::radians(FIELD_OF_VIEW) * 0.5f);
    const glm::vec4 viewSpace = scene.view * scene.model * glm::vec4(local, 1.0f);
    const float ndcX = viewSpace.x / (-viewSpace.z) / (tangent * aspect);
    const float ndcY = -(viewSpace.y / (-viewSpace.z)) / tangent;

    return { (ndcX + 1.0f) * 0.5f, (ndcY + 1.0f) * 0.5f };
  }

  void printSeams(const Options& options,
                  const Scene& scene)
  {
    std::cout << "seams: tiles=" << options.tiles << " apron=" << (options.apron ? "yes" : "no")
              << " window=" << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << " fov=" << FIELD_OF_VIEW
              << " tilt=" << TILT_DEGREES << " cameraPosition=(" << scene.cameraPosition.x << ", "
              << scene.cameraPosition.y << ", " << scene.cameraPosition.z << ")" << std::endl;

    for (uint32_t seam = 1; seam < options.tiles; ++seam)
    {
      const float x = static_cast<float>(seam * TILE_CELLS);

      for (uint32_t row = 0; row < PRINTED_ROWS; ++row)
      {
        const float z = static_cast<float>(SAMPLES_Z - 1) * (0.15f + 0.7f * static_cast<float>(row) / (PRINTED_ROWS - 1));
        const glm::vec2 screen = project(scene, glm::vec3(x, terrainHeight(x, z), z) * CELL_SIZE);

        std::cout << "seam " << seam << " row " << row << ": u=" << screen.x << " v=" << screen.y
                  << " pixel=(" << screen.x * static_cast<float>(WINDOW_WIDTH) << ", "
                  << screen.y * static_cast<float>(WINDOW_HEIGHT) << ")" << std::endl;
      }
    }

    std::cout << "SEAMS READY" << std::endl;
  }

  [[nodiscard]] std::vector<Tile> createTiles(const std::shared_ptr<vke::AssetManager>& assets,
                                              const Options& options)
  {
    std::vector<Tile> tiles;

    for (int i = 0; i < static_cast<int>(options.tiles); ++i)
    {
      Tile tile;
      tile.source = std::make_shared<TileSource>(tileBounds(i), tileSampleBounds(options, i));
      tile.surface = assets->createVolumeSurface({ .cellSize = CELL_SIZE, .chunkCells = 32 }, tile.source);
      tile.surface->setCastsShadows(false);
      tiles.push_back(std::move(tile));
    }

    return tiles;
  }

  // High above the terrain along its tilted up direction, with a shadow range too short to reach it
  [[nodiscard]] std::shared_ptr<vke::Light> createLight(vke::VulkanEngine& renderer,
                                                        const Scene& scene)
  {
    const glm::vec3 up = glm::vec3(scene.model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
    const glm::vec3 centerInWorld = glm::vec3(scene.model * glm::vec4(scene.center, 1.0f));
    const auto light = renderer.getLightingManager()->createPointLight(centerInWorld + up * 40.0f, { 1.0f, 1.0f, 1.0f }, 0.1f, 1.0f, 0.0f);
    light->setShadowRange(0.2f);

    return light;
  }

  template <typename RenderFrame>
  [[nodiscard]] bool finishSettling(vke::VulkanEngine& renderer,
                                    const std::vector<Tile>& tiles,
                                    const RenderFrame& renderFrame)
  {
    bool settled = allSettled(tiles);

    for (uint32_t i = 0; i < SETTLE_FRAME_CAP && !settled && renderer.isActive(); ++i)
    {
      renderFrame();
      settled = allSettled(tiles);
    }

    return settled;
  }

  [[nodiscard]] int run(const Options& options)
  {
    vke::VulkanEngine renderer(createConfig());

    renderer.getCamera()->disable();

    const auto rendering = renderer.getRenderingManager();
    const auto r3d = rendering->getRenderer3D();
    r3d->setProjectionParameters(FIELD_OF_VIEW, 0.1f, 500.0f);

    const auto assets = renderer.getAssetManager();

    std::vector<Tile> tiles = createTiles(assets, options);

    Scene scene = createScene(options);
    scene.cameraPosition = renderer.getCamera()->getPosition();
    scene.view = renderer.getCamera()->getViewMatrix();

    const auto light = createLight(renderer, scene);

    const auto renderFrame = [&] {
      r3d->setCameraParameters(scene.cameraPosition, scene.view);
      renderer.getLightingManager()->renderLight(light);

      for (const Tile& tile : tiles)
      {
        r3d->renderVolumeSurface(tile.surface, scene.model);
      }

      renderer.render();
    };

    bool printed = false;
    uint32_t settledFrames = 0;
    uint64_t frameIndex = 0;

    while (renderer.isActive() && (options.frames == 0 || frameIndex < options.frames))
    {
      renderFrame();
      ++frameIndex;

      settledFrames = allSettled(tiles) ? settledFrames + 1 : 0;

      if (!printed && settledFrames >= SHOW_FRAMES)
      {
        printSeams(options, scene);
        printed = true;
      }
    }

    const bool settled = finishSettling(renderer, tiles, renderFrame);

    if (options.frames > 0)
    {
      std::cout << "settled: " << (settled ? "yes" : "NO") << std::endl;
    }

    for (Tile& tile : tiles)
    {
      assets->release(std::move(tile.surface));
    }

    return options.frames > 0 && !settled ? EXIT_FAILURE : EXIT_SUCCESS;
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

#ifdef _WIN32
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif

  try
  {
    Options options;

    if (!parseOptions(argc, argv, options))
    {
      std::cerr << "Usage: volumeSeams [--tiles N (2 to " << MAX_TILES << ")] [--no-apron] [--frames N]" << std::endl;
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
