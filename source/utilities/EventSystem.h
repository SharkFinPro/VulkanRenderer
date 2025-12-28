#ifndef VULKANPROJECT_EVENTSYSTEM_H
#define VULKANPROJECT_EVENTSYSTEM_H

#include <functional>
#include <vector>
#include <tuple>

namespace vke {

  template<typename... EventTypes>
  class EventSystem {
  public:
    template<typename EventType, typename Callback>
    void on(Callback&& callback)
    {
      getListeners<EventType>().emplace_back(std::forward<Callback>(callback));
    }

    template<typename EventType>
    void emit(const EventType& event) const
    {
      for (const auto& callback : getListeners<EventType>())
      {
        callback(event);
      }
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