// Headless checks of buildDrawRuns, which groups a pass's draws into instanced runs. No engine, no GPU: ctest runs this.
#include "source/components/renderingManager/renderer3D/DrawRun.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
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

  // Stand-ins for models and materials: buildDrawRuns only compares their addresses.
  std::array<int, 3> g_models {};
  std::array<int, 2> g_materials {};
  std::array<int, 3> g_meshes {};

  const vke::Model* model(const int i)
  {
    return reinterpret_cast<const vke::Model*>(&g_models[i]);
  }

  const vke::Material* material(const int i)
  {
    return reinterpret_cast<const vke::Material*>(&g_materials[i]);
  }

  const vke::Mesh* mesh(const int i)
  {
    return reinterpret_cast<const vke::Mesh*>(&g_meshes[i]);
  }

  void checkEmpty()
  {
    std::vector<vke::DrawItem> items;
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 4> slots {};

    check(vke::buildDrawRuns(items, slots.data(), 3, runs) == 3, "empty input returns the offset");
    check(runs.empty(), "empty input adds no runs");
  }

  void checkGroupsByModel()
  {
    // Submission order alternates models, as a host's objects often do.
    std::vector<vke::DrawItem> items {
      { model(0), nullptr, 0, 0 },
      { model(1), nullptr, 0, 1 },
      { model(0), nullptr, 0, 2 },
      { model(1), nullptr, 0, 3 },
      { model(0), nullptr, 0, 4 }
    };
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 8> slots {};

    const uint32_t end = vke::buildDrawRuns(items, slots.data(), 2, runs);

    check(end == 7, "returns the offset after the last slot");
    check(runs.size() == 2, "one run per model");

    uint32_t covered = 0;
    for (const vke::DrawRun& run : runs)
    {
      check(run.firstInstance == 2 + covered, "runs are contiguous from the offset");
      covered += run.instanceCount;

      // Within a run the slots stay in submission order.
      for (uint32_t i = 1; i < run.instanceCount; ++i)
      {
        check(slots[run.firstInstance + i - 1] < slots[run.firstInstance + i], "slots in submission order within a run");
      }

      for (uint32_t i = 0; i < run.instanceCount; ++i)
      {
        const uint32_t slot = slots[run.firstInstance + i];
        check(model(static_cast<int>(slot % 2)) == run.model, "every slot in a run has the run's model");
      }
    }

    check(covered == 5, "every item is in a run");
    check(slots[0] == 0 && slots[1] == 0, "slots before the offset are untouched");
  }

  void checkGroupsByMaterialThenModel()
  {
    std::vector<vke::DrawItem> items {
      { model(0), material(1), 0, 0 },
      { model(1), material(0), 0, 1 },
      { model(0), material(0), 0, 2 },
      { model(0), material(1), 0, 3 },
      { model(2), material(0), 0, 4 }
    };
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 5> slots {};

    [[maybe_unused]] const uint32_t end = vke::buildDrawRuns(items, slots.data(), 0, runs);

    check(runs.size() == 4, "one run per distinct material and model");

    // Runs of one material are adjacent, so a pass binds each material once.
    int materialChanges = 0;
    for (size_t i = 1; i < runs.size(); ++i)
    {
      materialChanges += runs[i].material != runs[i - 1].material ? 1 : 0;
    }
    check(materialChanges == 1, "runs are grouped by material");

    for (const vke::DrawRun& run : runs)
    {
      if (run.material == material(1))
      {
        check(run.model == model(0) && run.instanceCount == 2, "the two matching items share one run");
      }
    }
  }

  void checkMaskValueBreaksRuns()
  {
    std::vector<vke::DrawItem> items {
      { model(0), nullptr, 2, 0 },
      { model(0), nullptr, 1, 1 },
      { model(0), nullptr, 2, 2 }
    };
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 3> slots {};

    [[maybe_unused]] const uint32_t end = vke::buildDrawRuns(items, slots.data(), 0, runs);

    check(runs.size() == 2, "a different mask value starts a new run");
    check(runs.size() == 2 && runs[0].maskValue == 1 && runs[1].maskValue == 2, "runs are sorted by mask value");
    check(runs.size() == 2 && runs[1].instanceCount == 2, "equal mask values and models share a run");
  }

  void checkAppendsToRuns()
  {
    // Each pass's runs go into their own vector, but buildDrawRuns appends, so earlier runs survive.
    std::vector<vke::DrawRun> runs { { model(2), nullptr, 0, 0, 1 } };
    std::vector<vke::DrawItem> items { { model(1), nullptr, 0, 7 } };
    std::array<uint32_t, 2> slots {};

    check(vke::buildDrawRuns(items, slots.data(), 1, runs) == 2, "single item advances the offset by one");
    check(runs.size() == 2 && runs[0].model == model(2), "existing runs are kept");
    check(runs.size() == 2 && runs[1].firstInstance == 1 && slots[1] == 7, "the new run starts at the offset");
  }

  void checkMeshBreaksRuns()
  {
    // Volume surface chunks: no model, equal mask and material, different meshes
    std::vector<vke::DrawItem> items {
      { nullptr, nullptr, 0, 0, mesh(0) },
      { nullptr, nullptr, 0, 1, mesh(1) },
      { nullptr, nullptr, 0, 2, mesh(2) }
    };
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 3> slots {};

    [[maybe_unused]] const uint32_t end = vke::buildDrawRuns(items, slots.data(), 0, runs);

    check(runs.size() == 3, "different meshes form different runs");

    for (const vke::DrawRun& run : runs)
    {
      check(run.instanceCount == 1 && run.model == nullptr, "each mesh run is one instance without a model");
    }
  }

  void checkSameMeshSharesRun()
  {
    // Two submissions of the surface's chunk: the same mesh under different table slots
    std::vector<vke::DrawItem> items {
      { nullptr, nullptr, 0, 5, mesh(1) },
      { nullptr, nullptr, 0, 2, mesh(0) },
      { nullptr, nullptr, 0, 9, mesh(1) }
    };
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 3> slots {};

    [[maybe_unused]] const uint32_t end = vke::buildDrawRuns(items, slots.data(), 0, runs);

    check(runs.size() == 2, "the same mesh in two slots shares one run");

    for (const vke::DrawRun& run : runs)
    {
      check(run.mesh == mesh(1) ? run.instanceCount == 2 : run.instanceCount == 1, "the run carries its mesh and count");

      if (run.mesh == mesh(1))
      {
        check(slots[run.firstInstance] == 5 && slots[run.firstInstance + 1] == 9, "the shared run holds both slots in order");
      }
    }
  }

  // The scene pass's chunk runs drawn nearest first: each run moves whole, with the slots it was built with, and a run
  // shared by two items sorts by the nearer of them
  void checkFrontToBack()
  {
    // Offset 4, as for a list built after others. mesh(0) is shared: one item far, one nearest of all.
    std::vector<vke::DrawItem> items {
      { nullptr, nullptr, 0, 10, mesh(2), 9.0f },
      { nullptr, nullptr, 0, 11, mesh(0), 25.0f },
      { nullptr, nullptr, 0, 12, mesh(1), 4.0f },
      { nullptr, nullptr, 0, 13, mesh(0), 1.0f }
    };
    std::vector<vke::DrawRun> runs;
    std::array<uint32_t, 8> slots {};
    std::vector<std::pair<float, vke::DrawRun>> scratch;

    [[maybe_unused]] const uint32_t end = vke::buildDrawRuns(items, slots.data(), 4, runs);
    vke::sortRunsFrontToBack(items, 4, runs, scratch);

    check(runs.size() == 3 && runs[0].mesh == mesh(0) && runs[1].mesh == mesh(1) && runs[2].mesh == mesh(2),
          "runs are ordered by their nearest item");
    check(runs.size() == 3 && runs[0].instanceCount == 2 && slots[runs[0].firstInstance] == 11 &&
          slots[runs[0].firstInstance + 1] == 13, "a moved run keeps its instances and slots");
    check(runs.size() == 3 && slots[runs[1].firstInstance] == 12 && slots[runs[2].firstInstance] == 10,
          "every run still points at its own slot");

    // A run takes the palette of its first item; a different mesh is another run with its own
    const auto* paletteA = reinterpret_cast<const vke::VolumePalette*>(&slots[0]);
    const auto* paletteB = reinterpret_cast<const vke::VolumePalette*>(&slots[1]);
    std::vector<vke::DrawItem> palettes {
      { nullptr, nullptr, 0, 1, mesh(0), 1.0f, paletteA },
      { nullptr, nullptr, 0, 2, mesh(0), 2.0f, paletteB },
      { nullptr, nullptr, 0, 3, mesh(1), 3.0f, paletteB }
    };
    std::vector<vke::DrawRun> paletteRuns;

    [[maybe_unused]] const uint32_t paletteEnd = vke::buildDrawRuns(palettes, slots.data(), 0, paletteRuns);

    check(paletteRuns.size() == 2 && paletteRuns[0].palette == paletteA && paletteRuns[0].instanceCount == 2,
          "a run takes its first item's palette");
    check(paletteRuns.size() == 2 && paletteRuns[1].palette == paletteB, "another run has its own palette");

    // Equal distances keep the grouping order
    std::vector<vke::DrawItem> tied {
      { nullptr, nullptr, 0, 1, mesh(1), 2.0f },
      { nullptr, nullptr, 0, 2, mesh(0), 2.0f }
    };
    std::vector<vke::DrawRun> tiedRuns;

    [[maybe_unused]] const uint32_t tiedEnd = vke::buildDrawRuns(tied, slots.data(), 0, tiedRuns);
    const std::vector<vke::DrawRun> grouped = tiedRuns;
    vke::sortRunsFrontToBack(tied, 0, tiedRuns, scratch);

    check(tiedRuns.size() == 2 && tiedRuns[0].firstInstance == grouped[0].firstInstance &&
          tiedRuns[1].firstInstance == grouped[1].firstInstance, "runs at equal distances keep the grouping order");
  }

  // Renderer3D sorts each frame's candidates once with drawsBefore and cuts every list from that order: a list cut from it
  // must give exactly what sorting it gives, and a list in any other order is still sorted
  void checkPresortedInput()
  {
    const std::vector<vke::DrawItem> candidates {
      { model(2), nullptr, 0, 0 },
      { model(0), nullptr, 0, 1 },
      { nullptr, nullptr, 0, 7, mesh(1) },
      { model(2), nullptr, 0, 3 },
      { model(1), nullptr, 0, 4 },
      { nullptr, nullptr, 0, 6, mesh(0) },
      { model(0), nullptr, 0, 5 }
    };

    std::vector<vke::DrawItem> presorted = candidates;
    std::ranges::sort(presorted, vke::drawsBefore);

    // A subset in the presorted order, as a culled list is
    std::vector<vke::DrawItem> cut;
    for (size_t i = 0; i < presorted.size(); i += 2)
    {
      cut.push_back(presorted[i]);
    }

    std::vector<vke::DrawItem> shuffled(cut.rbegin(), cut.rend());

    std::vector<vke::DrawRun> cutRuns;
    std::vector<vke::DrawRun> shuffledRuns;
    std::array<uint32_t, 8> cutSlots {};
    std::array<uint32_t, 8> shuffledSlots {};

    [[maybe_unused]] const uint32_t cutEnd = vke::buildDrawRuns(cut, cutSlots.data(), 0, cutRuns);
    [[maybe_unused]] const uint32_t shuffledEnd = vke::buildDrawRuns(shuffled, shuffledSlots.data(), 0, shuffledRuns);

    bool sameRuns = cutRuns.size() == shuffledRuns.size();

    for (size_t i = 0; sameRuns && i < cutRuns.size(); ++i)
    {
      sameRuns = cutRuns[i].model == shuffledRuns[i].model && cutRuns[i].mesh == shuffledRuns[i].mesh &&
                 cutRuns[i].firstInstance == shuffledRuns[i].firstInstance &&
                 cutRuns[i].instanceCount == shuffledRuns[i].instanceCount;
    }

    check(sameRuns && cutSlots == shuffledSlots, "a list cut from the presorted order gives what sorting it gives");
    check(std::ranges::is_sorted(shuffled, vke::drawsBefore), "a list in any other order is still sorted");
  }

} // namespace

int main()
{
  checkFrontToBack();
  checkPresortedInput();
  checkEmpty();
  checkGroupsByModel();
  checkGroupsByMaterialThenModel();
  checkMaskValueBreaksRuns();
  checkAppendsToRuns();
  checkMeshBreaksRuns();
  checkSameMeshSharesRun();

  if (g_failures > 0)
  {
    std::cout << g_failures << " of " << g_checks << " checks failed\n";
    return EXIT_FAILURE;
  }

  std::cout << g_checks << " checks passed\n";
  return EXIT_SUCCESS;
}
