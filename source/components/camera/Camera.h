#ifndef VKE_CAMERA_H
#define VKE_CAMERA_H

#include "../../EngineConfig.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <chrono>
#include <memory>

namespace vke {

  class Window;

  class Camera {
  public:
    explicit Camera(const EngineConfig::Camera& config);

    [[nodiscard]] glm::mat4 getViewMatrix() const;

    [[nodiscard]] glm::vec3 getPosition() const;

    void setSpeed(float cameraSpeed);

    // Keyboard movement (when move) and rotation. Rotation is applied only when rotate is true; the engine passes
    // whether a right-drag started over the scene. The wheel is separate because it should follow the cursor, not focus.
    void processInput(const std::shared_ptr<Window>& window, bool move, bool rotate);

    void processScroll(const std::shared_ptr<Window>& window);

    void enable();

    void disable();

    [[nodiscard]] bool isEnabled() const;

  private:
    bool m_enabled = true;

    glm::vec3 m_position;
    glm::vec3 m_direction{};

    struct SpeedSettings {
      float speed = 0;
      float cameraSpeed = 0;
      float scrollSpeed = 0;
      float swivelSpeed = 0;
    } m_speedSettings;

    struct Rotation {
      float pitch = 0;
      float yaw = 90;
    } m_rotation;

    std::chrono::time_point<std::chrono::steady_clock> m_previousTime;

    void handleMovement(const std::shared_ptr<Window>& window, float dt);
    void handleRotation(const std::shared_ptr<Window>& window, bool rotate);
    void handleZoom(const std::shared_ptr<Window>& window);
    void updateDirection();
  };

} // namespace vke

#endif //VKE_CAMERA_H
