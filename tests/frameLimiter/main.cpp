// Headless checks of the frame limiter schedule and hybrid wait, with made-up clocks. No engine, no GPU, no
// real sleeping: ctest runs this.
#include "source/utilities/FrameLimiter.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

namespace {

  using vke::FrameLimiter;
  using Clock = FrameLimiter::Clock;
  using ns = std::chrono::nanoseconds;

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

  const Clock::time_point T0 = Clock::time_point(std::chrono::seconds(100));
  constexpr ns ONE_MS = std::chrono::milliseconds(1);
  constexpr int FRAMES = 500;

  ns intervalOf(const double fps)
  {
    return ns(static_cast<int64_t>(std::llround(1e9 / fps)));
  }

  void checkUnlimited()
  {
    FrameLimiter limiter;
    check(limiter.getTargetFrameRate() == 0.0, "a new limiter is unlimited");

    bool allNow = true;
    Clock::time_point now = T0;
    for (int i = 0; i < 20; ++i)
    {
      allNow = allNow && limiter.schedule(now) == now;
      now += ONE_MS * (i % 3);
    }
    check(allNow, "unlimited returns now every time");
  }

  void checkInvalidRates()
  {
    const double rates[] { 0.0, -30.0, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), 1e12 };

    for (const double rate : rates)
    {
      FrameLimiter limiter;
      limiter.setTargetFrameRate(60.0);
      limiter.setTargetFrameRate(rate);
      check(limiter.getTargetFrameRate() == 0.0, "an invalid or absurd rate stores 0");
      check(limiter.schedule(T0) == T0 && limiter.schedule(T0 + ONE_MS) == T0 + ONE_MS, "an invalid or absurd rate is unlimited");
    }

    FrameLimiter limiter;
    limiter.setTargetFrameRate(144.0);
    check(limiter.getTargetFrameRate() == 144.0, "a valid rate reads back");
  }

  void checkFirstFrame()
  {
    FrameLimiter limiter;
    limiter.setTargetFrameRate(60.0);
    check(limiter.schedule(T0) == T0, "the first frame does not wait");
  }

  void checkRateChangeResets()
  {
    FrameLimiter limiter;
    limiter.setTargetFrameRate(60.0);
    check(limiter.schedule(T0) == T0, "reset: first frame");
    check(limiter.schedule(T0 + ONE_MS) == T0 + intervalOf(60.0), "reset: second frame waits for the grid");

    limiter.setTargetFrameRate(30.0);
    check(limiter.schedule(T0 + ONE_MS * 2) == T0 + ONE_MS * 2, "a change of rate resets the schedule");
    check(limiter.schedule(T0 + ONE_MS * 3) == T0 + ONE_MS * 2 + intervalOf(30.0), "the new rate applies after the reset");
  }

  void checkSameRateKeepsSchedule()
  {
    FrameLimiter limiter;
    limiter.setTargetFrameRate(60.0);
    check(limiter.schedule(T0) == T0, "same rate: first frame");

    limiter.setTargetFrameRate(60.0);
    check(limiter.schedule(T0 + ONE_MS) == T0 + intervalOf(60.0), "setting the same rate again keeps the schedule");
  }

  // Lateness up to one interval keeps the grid; beyond it the grid restarts.
  void checkResyncThreshold()
  {
    const ns interval = intervalOf(60.0);

    for (const auto& [lateness, resyncs] : { std::pair { interval * 9 / 10, false }, std::pair { interval, false },
                                             std::pair { interval * 11 / 10, true } })
    {
      FrameLimiter limiter;
      limiter.setTargetFrameRate(60.0);

      const Clock::time_point first = limiter.schedule(T0);
      const Clock::time_point late = first + interval + lateness;
      check(limiter.schedule(late) == late, "a late frame starts at once");

      // A zero-length frame, so the next start is the deadline itself
      const Clock::time_point next = limiter.schedule(late);
      const Clock::time_point expected = resyncs ? late + interval : first + interval * 2;
      check(next == expected, resyncs ? "more than one interval late restarts the grid"
                                      : "up to one interval late keeps the grid");
    }
  }

  void checkFastLoop()
  {
    for (const double fps : { 30.0, 60.0, 144.0, 240.0 })
    {
      FrameLimiter limiter;
      limiter.setTargetFrameRate(fps);
      const ns interval = intervalOf(fps);

      std::vector<Clock::time_point> starts;
      Clock::time_point now = T0;
      for (int i = 0; i <= FRAMES; ++i)
      {
        const Clock::time_point start = limiter.schedule(now);
        starts.push_back(start);
        now = start + ONE_MS;
      }

      bool exact = starts[0] == T0;
      for (size_t i = 1; i < starts.size(); ++i)
      {
        exact = exact && starts[i] - starts[i - 1] == interval;
      }
      check(exact, "a fast loop starts frames exactly one interval apart");
      check(starts.back() - starts.front() == interval * FRAMES, "a fast loop does not drift");
    }
  }

  void checkLateFrameKeepsGrid()
  {
    for (const double fps : { 30.0, 60.0, 144.0, 240.0 })
    {
      FrameLimiter limiter;
      limiter.setTargetFrameRate(fps);
      const ns interval = intervalOf(fps);

      Clock::time_point now = T0;
      std::vector<Clock::time_point> starts;
      for (int i = 0; i < 10; ++i)
      {
        const Clock::time_point start = limiter.schedule(now);
        starts.push_back(start);
        now = start + (i == 4 ? interval + interval / 5 : ONE_MS);
      }

      check(starts[5] == starts[4] + interval + interval / 5, "a late frame starts the next one immediately");
      check(starts[6] == starts[4] + interval * 2, "the frame after a late one lands on the original grid");
      check(starts[9] == starts[4] + interval * 5, "the grid holds after a late frame");
    }
  }

  void checkStallResyncs()
  {
    for (const double fps : { 30.0, 60.0, 144.0, 240.0 })
    {
      FrameLimiter limiter;
      limiter.setTargetFrameRate(fps);
      const ns interval = intervalOf(fps);

      Clock::time_point now = T0;
      const Clock::time_point first = limiter.schedule(now);
      now = first + ONE_MS;
      const Clock::time_point second = limiter.schedule(now);
      now = second + interval * 10;

      const Clock::time_point afterStall = limiter.schedule(now);
      check(afterStall == now, "after a stall the next start is now");

      const Clock::time_point following = limiter.schedule(afterStall + ONE_MS);
      check(following == afterStall + interval, "one interval after the resync, not a burst");

      const Clock::time_point third = limiter.schedule(following + ONE_MS);
      check(third == following + interval, "the grid continues after the resync");
    }
  }

  // The time at which a present that blocks until the next 60 Hz vblank returns, for work ending at `end`.
  Clock::time_point nextVblank(const Clock::time_point end)
  {
    const ns vblank = intervalOf(60.0);
    return T0 + vblank * ((end - T0 + vblank - ns(1)) / vblank);
  }

  void checkVsync()
  {
    // 61 fps against 60 Hz falls behind slowly, until a resync, and never waits either
    for (const double fps : { 61.0, 144.0, 240.0 })
    {
      FrameLimiter limiter;
      limiter.setTargetFrameRate(fps);

      bool noWait = true;
      Clock::time_point now = T0;
      for (int i = 0; i < FRAMES; ++i)
      {
        const Clock::time_point start = limiter.schedule(now);
        noWait = noWait && start == now;
        now = nextVblank(start + ONE_MS);
      }
      check(noWait, "a cap above the vsync rate never waits");
    }

    FrameLimiter limiter;
    limiter.setTargetFrameRate(30.0);

    Clock::time_point first;
    Clock::time_point last;
    Clock::time_point now = T0;
    for (int i = 0; i <= FRAMES; ++i)
    {
      const Clock::time_point start = limiter.schedule(now);
      if (i == 0)
      {
        first = start;
      }
      last = start;
      now = nextVblank(start + ONE_MS);
    }

    const double averageMs = std::chrono::duration<double, std::milli>(last - first).count() / FRAMES;
    check(std::abs(averageMs - 1000.0 / 30.0) < 0.1, "a cap below the vsync rate wins");
  }

  struct FakeClock {
    Clock::time_point now = T0;
    int sleeps = 0;
    int yields = 0;
    bool sleptPastMargin = false;
    ns overshoot = std::chrono::microseconds(300);
  };

  constexpr ns SPIN_MARGIN = ONE_MS;

  void checkSleepUntilFutureDeadlines()
  {
    constexpr ns margin = SPIN_MARGIN;

    for (const ns ahead : { ns(std::chrono::milliseconds(16)), ns(std::chrono::microseconds(2500)),
                            ns(std::chrono::microseconds(900)), ns(std::chrono::milliseconds(100)) })
    {
      FakeClock fake;
      const Clock::time_point deadline = T0 + ahead;

      vke::sleepUntil(
        deadline,
        [&fake] { return fake.now; },
        [&fake, deadline, margin](const Clock::duration d) {
          ++fake.sleeps;
          if (d <= Clock::duration::zero() || fake.now + d > deadline - margin)
          {
            fake.sleptPastMargin = true;
          }
          fake.now += d + fake.overshoot;
        },
        [&fake] {
          ++fake.yields;
          fake.now += std::chrono::microseconds(10);
        },
        margin);

      check(!fake.sleptPastMargin, "sleepUntil never asks to sleep past deadline minus the spin margin");
      check(fake.now >= deadline, "sleepUntil returns with now at or past the deadline");
      check(fake.now - deadline < std::chrono::microseconds(100), "sleepUntil does not overshoot by much");
    }
  }

  void checkSleepUntilNearDeadlines()
  {
    constexpr ns margin = SPIN_MARGIN;

    FakeClock past;
    vke::sleepUntil(
      T0 - ONE_MS,
      [&past] { return past.now; },
      [&past](const Clock::duration) { ++past.sleeps; },
      [&past] { ++past.yields; },
      margin);
    check(past.sleeps == 0 && past.yields == 0, "a deadline in the past neither sleeps nor spins");

    FakeClock exact;
    vke::sleepUntil(
      T0,
      [&exact] { return exact.now; },
      [&exact](const Clock::duration) { ++exact.sleeps; },
      [&exact] { ++exact.yields; },
      margin);
    check(exact.sleeps == 0 && exact.yields == 0, "a deadline of now neither sleeps nor spins");

    FakeClock close;
    vke::sleepUntil(
      T0 + std::chrono::microseconds(500),
      [&close] { return close.now; },
      [&close](const Clock::duration) { ++close.sleeps; },
      [&close] {
        ++close.yields;
        close.now += std::chrono::microseconds(100);
      },
      margin);
    check(close.sleeps == 0 && close.yields == 5, "a deadline inside the margin only spins");

    // The overload without a yield callable returns at once for a past deadline
    bool slept = false;
    vke::sleepUntil(T0 - ONE_MS, [] { return T0; }, [&slept](const Clock::duration) { slept = true; }, margin);
    check(!slept, "the default-yield overload returns for a past deadline");
  }

} // namespace

int main()
{
  checkUnlimited();
  checkInvalidRates();
  checkFirstFrame();
  checkRateChangeResets();
  checkSameRateKeepsSchedule();
  checkResyncThreshold();
  checkFastLoop();
  checkLateFrameKeepsGrid();
  checkStallResyncs();
  checkVsync();
  checkSleepUntilFutureDeadlines();
  checkSleepUntilNearDeadlines();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
