#ifndef VKE_FRAMELIMITER_H
#define VKE_FRAMELIMITER_H

#include <chrono>
#include <thread>

namespace vke {

  // Paces a loop to a target frame rate. wait() is the per-frame call; schedule() is the deadline math alone, with
  // no clock reads, so it can be checked with made-up times.
  class FrameLimiter {
  public:
    using Clock = std::chrono::steady_clock;

    // Frames per second. Returns the rate now in effect: 0 (unlimited) for 0, a negative or non-finite value, or one
    // whose interval rounds to zero nanoseconds or exceeds 1e15 ns. A change of rate resets the schedule; the same
    // rate again does not.
    double setTargetFrameRate(double framesPerSecond);

    [[nodiscard]] double getTargetFrameRate() const;

    // Returns when the frame starting at `now` may start (`now` itself means no wait) and advances the schedule.
    // The deadline moves by whole intervals, so a slightly late frame keeps the grid; a frame more than one
    // interval late (a stall, or a blocking present slower than the cap) restarts the grid at `now` instead of
    // bursting to catch up.
    [[nodiscard]] Clock::time_point schedule(Clock::time_point now);

    // Sleeps, then spins, until the next frame may start. Window events are not processed meanwhile. The spin margin
    // is 1 ms, or 16 ms on Windows without a high-resolution timer; a cap whose interval is within it only spins.
    void wait();

  private:
    double m_frameRate = 0.0;

    Clock::duration m_interval = Clock::duration::zero();

    Clock::time_point m_nextDeadline;

    bool m_hasSchedule = false;
  };

  // Sleeps while more than spinMargin remains, never past deadline - spinMargin, then spins until the deadline.
  template <typename Now, typename SleepFor, typename Yield>
  void sleepUntil(const FrameLimiter::Clock::time_point deadline,
                  Now&& now,
                  SleepFor&& sleepFor,
                  Yield&& yield,
                  const FrameLimiter::Clock::duration spinMargin)
  {
    for (auto remaining = deadline - now(); remaining > spinMargin; remaining = deadline - now())
    {
      sleepFor(remaining - spinMargin);
    }

    while (now() < deadline)
    {
      yield();
    }
  }

  template <typename Now, typename SleepFor>
  void sleepUntil(const FrameLimiter::Clock::time_point deadline,
                  Now&& now,
                  SleepFor&& sleepFor,
                  const FrameLimiter::Clock::duration spinMargin)
  {
    sleepUntil(deadline, now, sleepFor, [] { std::this_thread::yield(); }, spinMargin);
  }

} // namespace vke

#endif //VKE_FRAMELIMITER_H
