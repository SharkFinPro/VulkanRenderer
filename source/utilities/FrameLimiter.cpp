#include "FrameLimiter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

namespace vke {

  namespace {

    constexpr auto SPIN_MARGIN = std::chrono::milliseconds(1);

#ifdef _WIN32
    // Without the high-resolution timer, sleeps round up to the default timer tick (about 15.6 ms), so only sleep
    // when more than a tick remains
    constexpr auto FALLBACK_SPIN_MARGIN = std::chrono::milliseconds(16);

    // One per thread; the handle is closed with the thread.
    class HighResolutionTimer {
    public:
      HighResolutionTimer()
        : m_handle(CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS))
      {
      }

      ~HighResolutionTimer()
      {
        if (m_handle != nullptr)
        {
          CloseHandle(m_handle);
        }
      }

      HighResolutionTimer(const HighResolutionTimer&) = delete;
      HighResolutionTimer& operator=(const HighResolutionTimer&) = delete;

      [[nodiscard]] bool isValid() const
      {
        return m_handle != nullptr;
      }

      void sleep(const FrameLimiter::Clock::duration duration) const
      {
        LARGE_INTEGER dueTime;
        // Negative means relative, in 100 ns units
        dueTime.QuadPart = -std::max<int64_t>(1, std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() / 100);

        if (SetWaitableTimer(m_handle, &dueTime, 0, nullptr, nullptr, FALSE))
        {
          WaitForSingleObject(m_handle, INFINITE);
        }
        else
        {
          std::this_thread::sleep_for(duration);
        }
      }

    private:
      HANDLE m_handle;
    };

    const HighResolutionTimer& threadTimer()
    {
      thread_local const HighResolutionTimer timer;
      return timer;
    }
#endif

  } // namespace

  double FrameLimiter::setTargetFrameRate(const double framesPerSecond)
  {
    double frameRate = 0.0;
    Clock::duration interval = Clock::duration::zero();

    if (std::isfinite(framesPerSecond) && framesPerSecond > 0.0)
    {
      const double nanoseconds = std::round(1e9 / framesPerSecond);

      // Zero nanoseconds is unlimited, and so is an interval over 1e15 ns (11.6 days), which keeps deadline arithmetic
      // far from the clock's range
      if (nanoseconds >= 1.0 && nanoseconds <= 1e15)
      {
        frameRate = framesPerSecond;
        interval = std::chrono::duration_cast<Clock::duration>(std::chrono::nanoseconds(static_cast<int64_t>(nanoseconds)));
      }
    }

    // Setting the same rate again (say, every frame) must not restart the grid
    if (frameRate == m_frameRate && interval == m_interval)
    {
      return m_frameRate;
    }

    m_hasSchedule = false;
    m_frameRate = frameRate;
    m_interval = interval;

    return m_frameRate;
  }

  double FrameLimiter::getTargetFrameRate() const
  {
    return m_frameRate;
  }

  FrameLimiter::Clock::time_point FrameLimiter::schedule(const Clock::time_point now)
  {
    if (m_interval == Clock::duration::zero())
    {
      m_hasSchedule = false;
      return now;
    }

    if (!m_hasSchedule || now - m_nextDeadline > m_interval)
    {
      m_hasSchedule = true;
      m_nextDeadline = now + m_interval;
      return now;
    }

    const Clock::time_point wake = m_nextDeadline;
    m_nextDeadline += m_interval;
    return std::max(wake, now);
  }

  void FrameLimiter::wait()
  {
    const Clock::time_point deadline = schedule(Clock::now());
    const auto now = [] { return Clock::now(); };
    const auto sleepFor = [](const Clock::duration d) { std::this_thread::sleep_for(d); };

#ifdef _WIN32
    const HighResolutionTimer& timer = threadTimer();

    if (timer.isValid())
    {
      sleepUntil(deadline, now, [&timer](const Clock::duration d) { timer.sleep(d); }, SPIN_MARGIN);
    }
    else
    {
      sleepUntil(deadline, now, sleepFor, FALLBACK_SPIN_MARGIN);
    }
#else
    sleepUntil(deadline, now, sleepFor, SPIN_MARGIN);
#endif
  }

} // namespace vke
