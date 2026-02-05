#ifndef VULKANPROJECT_EVENTSYSTEM_H
#define VULKANPROJECT_EVENTSYSTEM_H

#include <functional>
#include <memory>
#include <tuple>
#include <vector>

namespace vke {

  template<typename EventType>
  struct EventListener {
    std::shared_ptr<std::function<void(const EventType&)>> pCallback = nullptr;
  };

  template<typename... EventTypes>
  class EventSystem {
  public:
    template<typename EventType, typename Callback>
    [[nodiscard]] EventListener<EventType> on(Callback&& callback)
    {
      auto& listeners = getListeners<EventType>();
      const auto pCallback = std::make_shared<std::function<void(const EventType&)>>(std::forward<Callback>(callback));

      listeners.push_back(pCallback);

      return {
        pCallback
      };
    }

    template<typename EventType>
    void emit(const EventType& event) const
    {
      for (const auto& pCallback : getListeners<EventType>())
      {
        if (pCallback)
        {
          (*pCallback)(event);
        }
      }
    }

    template<typename EventType>
    void removeListener(EventListener<EventType>& listener)
    {
      std::erase_if(getListeners<EventType>(), [&](auto& cb) { return cb == listener.pCallback; });

      listener.pCallback.reset();
    }

  protected:
    EventSystem() = default;

  private:
    template<typename EventType>
    using ListenerVector = std::vector<std::shared_ptr<std::function<void(const EventType&)>>>;

    std::tuple<ListenerVector<EventTypes>...> m_listeners;

    template<typename EventType>
    auto& getListeners()
    {
      return std::get<ListenerVector<EventType>>(m_listeners);
    }

    template<typename EventType>
    const auto& getListeners() const
    {
      return std::get<ListenerVector<EventType>>(m_listeners);
    }
  };

} // vke

#endif //VULKANPROJECT_EVENTSYSTEM_H