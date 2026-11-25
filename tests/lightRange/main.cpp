// Headless checks of the light range math (LightRange.h). No engine, no GPU: ctest runs this.
#include "source/components/lighting/lights/LightRange.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

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

  void testValidity()
  {
    constexpr float nearPlane = 0.1f;

    check(vke::isValidLightRange(0.0f, nearPlane), "range 0 is valid");
    check(!vke::isValidLightRange(nearPlane, nearPlane), "range equal to the near plane is invalid");
    check(vke::isValidLightRange(0.11f, nearPlane), "range just above the near plane is valid");
    check(!vke::isValidLightRange(-1.0f, nearPlane), "negative range is invalid");
    check(!vke::isValidLightRange(std::numeric_limits<float>::quiet_NaN(), nearPlane), "NaN range is invalid");
    check(!vke::isValidLightRange(std::numeric_limits<float>::infinity(), nearPlane), "infinite range is invalid");
  }

  void testEffectiveShadowRange()
  {
    check(vke::effectiveShadowRange(1000.0f, 0.0f) == 1000.0f, "no range keeps the shadow range");
    check(vke::effectiveShadowRange(1000.0f, 50.0f) == 50.0f, "a smaller range clamps the shadow range");
    check(vke::effectiveShadowRange(1000.0f, 2000.0f) == 1000.0f, "a larger range keeps the shadow range");
  }

  void testAttenuation()
  {
    for (const float distance : { 0.0f, 10.0f, 1e6f })
    {
      check(vke::lightAttenuation(0.0f, distance) == 1.0f, "no range never attenuates");
    }

    check(vke::lightAttenuation(10.0f, 0.0f) == 1.0f, "full strength at the light");
    check(vke::lightAttenuation(10.0f, 10.0f) == 0.0f, "zero at the range");
    check(vke::lightAttenuation(10.0f, 10.5f) == 0.0f, "zero beyond the range");
    check(vke::lightAttenuation(10.0f, 1e6f) == 0.0f, "zero far beyond the range");
    check(vke::lightAttenuation(10.0f, 10.0f * (1.0f - 1e-4f)) < 1e-6f, "continuous at the range");

    // Halfway: (1 - 0.5^4)^2 / (5^2 + 1), which pins the window's shape and the inverse square
    check(std::abs(vke::lightAttenuation(10.0f, 5.0f) - 0.87890625f / 26.0f) < 1e-6f, "reference value halfway to the range");

    constexpr float range = 25.0f;
    constexpr int steps = 1000;
    float previous = vke::lightAttenuation(range, 0.0f);
    bool decreasing = true;
    bool inBounds = true;

    for (int i = 1; i <= steps; i++)
    {
      const float value = vke::lightAttenuation(range, range * static_cast<float>(i) / steps);

      decreasing = decreasing && value < previous;
      inBounds = inBounds && value >= 0.0f && value <= 1.0f;
      previous = value;
    }

    // The last step lands on the range itself, which is 0 and so below the one before it
    check(decreasing, "strictly decreasing from the light to the range");
    check(inBounds, "attenuation stays within 0 and 1");
  }

} // namespace

int main()
{
  testValidity();
  testEffectiveShadowRange();
  testAttenuation();

  std::cout << g_checks << " checks, " << g_failures << " failures\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
