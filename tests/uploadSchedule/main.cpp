// Headless checks of UploadSchedule, the FIFO with a per-frame byte budget behind the mesh upload queue. No engine, no
// GPU: ctest runs this.
#include "source/utilities/UploadSchedule.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
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

  using Schedule = vke::UploadSchedule<int>;

  // Takes one frame's share and returns the ids in the order they were taken.
  std::vector<int> takeFrame(Schedule& schedule,
                             const uint64_t budget,
                             uint64_t* bytesTaken = nullptr)
  {
    std::vector<int> ids;
    const uint64_t bytes = schedule.take(budget, [&ids](int&& id) {
      ids.push_back(id);
    });

    if (bytesTaken != nullptr)
    {
      *bytesTaken = bytes;
    }

    return ids;
  }

  void checkEmpty()
  {
    Schedule schedule;
    int calls = 0;

    const uint64_t bytes = schedule.take(100, [&calls](int&&) {
      ++calls;
    });

    check(bytes == 0, "empty take returns 0");
    check(calls == 0, "empty take calls nothing");
    check(schedule.empty() && schedule.size() == 0 && schedule.pendingBytes() == 0, "empty bookkeeping");
  }

  void checkOrderWithinAndAcrossFrames()
  {
    Schedule schedule;
    for (int id = 0; id < 6; ++id)
    {
      schedule.push(id, 10);
    }

    check(takeFrame(schedule, 30) == std::vector<int> { 0, 1, 2 }, "first frame takes the oldest three");
    check(takeFrame(schedule, 30) == std::vector<int> { 3, 4, 5 }, "second frame continues in order");
    check(schedule.empty(), "all taken");
  }

  void checkExactBudget()
  {
    Schedule schedule;
    schedule.push(0, 40);
    schedule.push(1, 60);
    schedule.push(2, 1);

    uint64_t bytes = 0;
    check(takeFrame(schedule, 100, &bytes) == std::vector<int> { 0, 1 }, "items summing to exactly the budget are taken");
    check(bytes == 100, "exact budget reports the bytes");
    check(takeFrame(schedule, 100) == std::vector<int> { 2 }, "the next item waits for the next frame");
  }

  void checkCarryOver()
  {
    Schedule schedule;
    schedule.push(0, 60);
    schedule.push(1, 60);

    check(takeFrame(schedule, 100) == std::vector<int> { 0 }, "the second item does not fit");

    schedule.push(2, 10);

    // 2 would fit beside 1 only if it jumped the queue; 1 goes first.
    check(takeFrame(schedule, 100) == std::vector<int> { 1, 2 }, "what waited is taken before newer items");
    check(schedule.empty(), "carry-over drained");
  }

  void checkNoSkippingAhead()
  {
    Schedule schedule;
    schedule.push(0, 50);
    schedule.push(1, 80);
    schedule.push(2, 10);

    check(takeFrame(schedule, 100) == std::vector<int> { 0 }, "stops at the first item that does not fit");
    check(schedule.size() == 2 && schedule.pendingBytes() == 90, "skipped-over items stay queued");
    check(takeFrame(schedule, 100) == std::vector<int> { 1, 2 }, "the small item stays behind the big one");
  }

  void checkOversizedHead()
  {
    Schedule schedule;
    schedule.push(0, 500);
    schedule.push(1, 1);

    uint64_t bytes = 0;
    check(takeFrame(schedule, 100, &bytes) == std::vector<int> { 0 }, "an oversized head goes alone");
    check(bytes == 500, "the oversized upload reports its own size");
    check(takeFrame(schedule, 100) == std::vector<int> { 1 }, "the next frame resumes normally");
  }

  void checkOversizedBehindSmallerWaits()
  {
    Schedule schedule;
    schedule.push(0, 10);
    schedule.push(1, 500);
    schedule.push(2, 10);

    check(takeFrame(schedule, 100) == std::vector<int> { 0 }, "an oversized item behind a smaller one waits");
    check(takeFrame(schedule, 100) == std::vector<int> { 1 }, "then goes alone in a frame of its own");
    check(takeFrame(schedule, 100) == std::vector<int> { 2 }, "and the item behind it follows");
  }

  void checkZeroBudget()
  {
    Schedule schedule;
    for (int id = 0; id < 4; ++id)
    {
      schedule.push(id, 8);
    }

    for (int id = 0; id < 4; ++id)
    {
      check(takeFrame(schedule, 0) == std::vector<int> { id }, "a zero budget takes exactly one per call");
    }

    check(schedule.empty(), "zero budget drains one at a time");
  }

  void checkBookkeeping()
  {
    Schedule schedule;
    schedule.push(0, 30);
    schedule.push(1, 20);
    schedule.push(2, 10);

    check(schedule.size() == 3 && schedule.pendingBytes() == 60 && !schedule.empty(), "bookkeeping after push");

    static_cast<void>(takeFrame(schedule, 50));

    check(schedule.size() == 1 && schedule.pendingBytes() == 10, "bookkeeping after take");

    schedule.clear();

    check(schedule.empty() && schedule.pendingBytes() == 0, "bookkeeping after clear");
  }

  void checkMoveOnlyItems()
  {
    vke::UploadSchedule<std::unique_ptr<int>> schedule;
    schedule.push(std::make_unique<int>(7), 4);

    int value = 0;
    schedule.take(4, [&value](std::unique_ptr<int>&& item) {
      value = *item;
    });

    check(value == 7, "move-only items are taken");
  }

  struct RandomRun {
    std::vector<uint64_t> sizes;
    std::vector<int> taken;
    uint64_t bytesIn = 0;
    uint64_t bytesOut = 0;
    bool budgetRespected = true;
    bool bookkeepingMatches = true;
  };

  void runRandomFrames(Schedule& schedule,
                       RandomRun& run)
  {
    std::mt19937 random(12345);
    std::uniform_int_distribution<uint64_t> sizeDistribution(0, 300);
    std::uniform_int_distribution<int> pushDistribution(0, 5);
    std::uniform_int_distribution<uint64_t> budgetDistribution(0, 400);

    for (int frame = 0; frame < 5000; ++frame)
    {
      const int pushes = pushDistribution(random);
      for (int i = 0; i < pushes; ++i)
      {
        const uint64_t bytes = sizeDistribution(random);
        schedule.push(static_cast<int>(run.sizes.size()), bytes);
        run.sizes.push_back(bytes);
        run.bytesIn += bytes;
      }

      const uint64_t budget = budgetDistribution(random);
      uint64_t frameBytes = 0;
      const std::vector<int> ids = takeFrame(schedule, budget, &frameBytes);

      uint64_t sum = 0;
      for (const int id : ids)
      {
        run.taken.push_back(id);
        sum += run.sizes[static_cast<size_t>(id)];
      }

      run.bytesOut += frameBytes;
      run.bookkeepingMatches = run.bookkeepingMatches && sum == frameBytes &&
                               schedule.pendingBytes() == run.bytesIn - run.bytesOut &&
                               schedule.size() == run.sizes.size() - run.taken.size();

      // Within budget, unless a single oversized item went alone.
      if (sum > budget && ids.size() != 1)
      {
        run.budgetRespected = false;
      }
    }
  }

  // Bounded, so a take() that stops making progress fails here instead of hanging.
  void drainRemaining(Schedule& schedule,
                      RandomRun& run)
  {
    for (int frame = 0; frame < 100000 && !schedule.empty(); ++frame)
    {
      uint64_t frameBytes = 0;
      for (const int id : takeFrame(schedule, 250, &frameBytes))
      {
        run.taken.push_back(id);
      }

      run.bytesOut += frameBytes;
    }
  }

  void checkRandomized()
  {
    Schedule schedule;
    RandomRun run;

    runRandomFrames(schedule, run);
    drainRemaining(schedule, run);

    bool inOrder = run.taken.size() == run.sizes.size();
    for (size_t i = 0; inOrder && i < run.taken.size(); ++i)
    {
      inOrder = run.taken[i] == static_cast<int>(i);
    }

    check(schedule.empty(), "everything drains");
    check(inOrder, "every id taken exactly once, in order");
    check(run.bytesIn == run.bytesOut, "bytes in equal bytes taken");
    check(run.budgetRespected, "no frame exceeds the budget except a lone oversized item");
    check(run.bookkeepingMatches, "bookkeeping matches every frame");
    check(schedule.pendingBytes() == 0, "nothing pending at the end");
  }

} // namespace

int main()
{
  checkEmpty();
  checkOrderWithinAndAcrossFrames();
  checkExactBudget();
  checkCarryOver();
  checkNoSkippingAhead();
  checkOversizedHead();
  checkOversizedBehindSmallerWaits();
  checkZeroBudget();
  checkBookkeeping();
  checkMoveOnlyItems();
  checkRandomized();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? 0 : 1;
}
