#ifndef VKE_PICKREQUESTS_H
#define VKE_PICKREQUESTS_H

#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace vke {

  // The bookkeeping behind on-request mouse picking: callbacks wait as requests, become active at a frame boundary, ride
  // along with the frame that takes the pick and fire once its readback resolves. Engine-independent (generic over the
  // result type) so the rules are unit tested without a GPU.
  template <typename Result>
  class PickRequests {
  public:
    using Callback = std::function<void(const std::optional<Result>&)>;

    // Waits until the next promote, so a request never joins the frame that is already being submitted.
    void request(Callback callback)
    {
      m_pending.push_back(std::move(callback));
    }

    // Called at the end of every frame: the requests made so far become active for the next frame's submissions.
    void promote()
    {
      for (Callback& callback : m_pending)
      {
        m_active.push_back(std::move(callback));
      }

      m_pending.clear();
    }

    // Whether the frame being submitted has to pick. Stays true until a frame takes the requests, so a frame that never
    // reaches its readback hands them on to the next one.
    [[nodiscard]] bool hasActive() const
    {
      return !m_active.empty();
    }

    // Hands the active requests to a frame's readback slot, which fires them when the readback resolves. Does nothing
    // when none are active, so an idle frame costs a check and allocates nothing.
    void take(std::vector<Callback>& slotCallbacks)
    {
      for (Callback& callback : m_active)
      {
        slotCallbacks.push_back(std::move(callback));
      }

      m_active.clear();
    }

    // Calls each callback once, in request order, then empties the vector (keeping its capacity). If a callback throws,
    // the vector is still emptied: the ones after it are dropped, and none that already ran can run again.
    static void fire(std::vector<Callback>& slotCallbacks,
                     const std::optional<Result>& result)
    {
      struct Clear {
        std::vector<Callback>& callbacks;

        ~Clear()
        {
          callbacks.clear();
        }
      } clear { slotCallbacks };

      for (Callback& callback : slotCallbacks)
      {
        callback(result);
      }
    }

  private:
    std::vector<Callback> m_pending;
    std::vector<Callback> m_active;
  };

} // namespace vke

#endif //VKE_PICKREQUESTS_H
