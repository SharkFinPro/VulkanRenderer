// Headless checks of buildDrawRuns, which groups a pass's draws into instanced runs. No engine, no GPU: ctest runs this.
#include "source/components/renderingManager/renderer3D/DrawRun.h"
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

  const vke::Model* model(const int i)
  {
    return reinterpret_cast<const vke::Model*>(&g_models[i]);
  }

  const vke::Material* material(const int i)
  {
    return reinterpret_cast<const vke::Material*>(&g_materials[i]);
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

} // namespace

int main()
{
  checkEmpty();
  checkGroupsByModel();
  checkGroupsByMaterialThenModel();
  checkMaskValueBreaksRuns();
  checkAppendsToRuns();

  if (g_failures > 0)
  {
    std::cout << g_failures << " of " << g_checks << " checks failed\n";
    return EXIT_FAILURE;
  }

  std::cout << g_checks << " checks passed\n";
  return EXIT_SUCCESS;
}
