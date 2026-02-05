#ifndef VULKANPROJECT_EVENTSYSTEM_H
#define VULKANPROJECT_EVENTSYSTEM_H

#include <functional>
#include <vector>
#include <tuple>

namespace vke {

  template<typename EventType>
  struct EventListener {
    std::function<void(const EventType&)>* callback;
  };

  template<typename... EventTypes>
  class EventSystem {
  public:
    template<typename EventType, typename Callback>
    [[nodiscard]] EventListener<EventType> on(Callback&& callback)
    {
      auto& listeners = getListeners<EventType>();
      listeners.emplace_back(std::forward<Callback>(callback));

      return {
        &listeners.back()
      };
    }

    template<typename EventType>
    void emit(const EventType& event) const
    {
      for (const auto& callback : getListeners<EventType>())
      {
        callback(event);
      }
    }

    template<typename EventType>
    void removeListener(EventListener<EventType>& listener)
    {
      auto& listeners = getListeners<EventType>();

      listeners.erase(std::remove_if(
        listeners.begin(),
        listeners.end(),
        [&](auto& cb) { return &cb == listener.callback; }),
        listeners.end()
      );
    }

  protected:
    EventSystem() = default;

  private:
    template<typename EventType>
    using ListenerVector = std::vector<std::function<void(const EventType&)>>;

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