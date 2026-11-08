# Frame pacing

Two settings control how frames reach the screen and how often they are produced:

- the **present mode**: whether presentation waits for vertical sync, and whether it may tear
- the **frame rate cap**: an engine-side limit on frames per second

Both are set at construction in `EngineConfig::Rendering`. Both can be changed at runtime through `RenderingManager`
(`VulkanEngine::getRenderingManager()`), without restarting.

```cpp
vke::VulkanEngine engine(vke::EngineConfig {
  .rendering {
    .presentMode = vke::PresentMode::fifo, // vsync on
    .frameRateLimit = 0.0                  // unlimited
  }
});

const auto rendering = engine.getRenderingManager();

// Later, between frames:
const vke::PresentMode expected = rendering->setPresentMode(vke::PresentMode::immediate);
const double cap = rendering->setFrameRateLimit(144.0);
```

Call the setters and getters from the thread that runs the frame loop, between calls to `VulkanEngine::render()`. They
are not synchronized for use from other threads.

## Present mode

| `PresentMode` | Vsync | May tear | Blocks the loop | Falls back to |
|---|---|---|---|---|
| `fifo` | yes | no | yes, at the display rate | (always supported) |
| `fifoRelaxed` | yes, unless a frame is late | when a frame is late | yes, unless a frame is late | `fifo` |
| `mailbox` (default) | shown at vertical blank, but the loop is not held to it; the newest frame replaces a queued one | no | no | `fifo` |
| `immediate` | no | yes | no | `mailbox`, then `fifo` |

`mailbox` never falls back to a mode that tears. The choice is made by `choosePresentMode`
(`source/components/window/PresentMode.h`), a pure function of the preference and the modes the surface reports.

- `setPresentMode(mode)` stores the preference and returns the mode **expected** to be used, judged from the surface
  support read at the last swapchain rebuild. A request that falls back is reported this way, not silently.
- The swapchain is rebuilt at the end of the next frame, through the same path as a window resize. A request that would
  not change the mode in use does not by itself schedule a rebuild. While the main window is minimized, the rebuild
  waits until it is restored.
- `getPresentMode()` is the stored preference. `getActivePresentMode()` is the mode the current swapchain uses.
  `getSupportedPresentModes()` lists the modes the surface offers, as of the last rebuild, in enum order. `fifo` is
  always listed.
- The preference survives later rebuilds, so a surface that gains or loses a mode (another monitor, say) is
  re-resolved from the preference each time.
- ImGui's detached windows have their own swapchains, which keep the ImGui backend's choice (mailbox, else immediate,
  else FIFO). Normally they don't block the loop; on a surface with neither mailbox nor immediate they use FIFO and can.

## Frame rate cap

- `setFrameRateLimit(framesPerSecond)` caps the loop; `0` means unlimited. It returns the cap now in effect. A
  negative or non-finite value, or one whose interval rounds to zero nanoseconds or exceeds 1e15 ns (11.6 days), is
  rejected as unlimited and reported as `0`. `getFrameRateLimit()`
  reads the cap back.
- Setting the same cap again, for example every frame, keeps the schedule. A different cap starts a new schedule, and
  takes effect from the next frame.
- The cap is enforced at one point: the first statement of `VulkanEngine::render()`, before window events and input
  are polled.

### How it waits

`FrameLimiter` (`source/utilities/FrameLimiter.h`) keeps a grid of deadlines one interval apart.

- Each frame starts at its deadline, or immediately if the deadline has passed. The next deadline is the previous one
  plus one interval, so a slightly late frame does not shift the grid and the average rate stays exact.
- A frame more than one interval late (a stall, a minimized window, or a blocking present slower than the cap) restarts
  the grid at the current time. The limiter never bursts frames to catch up.
- The wait sleeps until 1 ms before the deadline, then spins (yielding) for the rest.
  - On Windows the sleep uses a high-resolution waitable timer. Without one (Windows 10 before 1803), sleeps round up
    to the 15.6 ms timer tick, so the limiter only sleeps while more than 16 ms remain and spins the rest.
  - Elsewhere it uses `std::this_thread::sleep_for`.

### With vsync

When presentation blocks (`fifo`, `fifoRelaxed`), the display rate already paces the loop:

- A cap above the display rate is a no-op. Each frame arrives late, so the limiter never waits.
- A cap below the display rate wins. The loop waits for the cap, and presentation then waits for the next vertical
  blank.

### Accuracy

Measured with `manyObjects --objects 200 --frames 600 --warmup 120` (Windows 11, NVIDIA, Debug build, high-resolution
timer). Values are mean loop times; p95 in parentheses.

| Present mode | Cap | Mean ms (p95) | Target ms |
|---|---|---|---|
| immediate | none | 3.126 (3.546) | |
| immediate | 30 | 33.332 (33.651) | 33.333 |
| immediate | 60 | 16.665 (16.900) | 16.667 |
| immediate | 144 | 6.943 (7.206) | 6.944 |
| immediate | 240 | 4.166 (4.529) | 4.167 |
| fifo, 170 Hz display | none | 5.887 (6.019) | |
| fifo, 170 Hz display | 240 | 5.887 (6.016) | no-op |
| fifo, 170 Hz display | 30 | 33.332 (33.605) | 33.333 |

Reproduce with `--present-mode` and `--frame-rate-limit`. Linux and macOS were not measured. There the accuracy depends
on how far `sleep_for` overshoots; overshoot within the 1 ms spin margin costs nothing.

### Caveats

- **Input latency.** Only the camera reads input after the wait. Whatever the application builds before calling
  `render()` (its GUI, picking) used input polled before the previous wait, so a low cap adds up to one interval of
  latency there.
- **Window events.** Events are not processed during the wait. A cap far below 1 fps leaves the window unresponsive for
  that long.
- **CPU.** The spin uses a core for up to 1 ms per frame (about 1000 fps and up only spins). On Windows without the
  high-resolution timer the margin is 16 ms, so caps of about 60 fps and up spin for most or all of each frame.
