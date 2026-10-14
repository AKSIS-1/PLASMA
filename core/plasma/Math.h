#pragma once
// MSVC does not define M_PI unless _USE_MATH_DEFINES is set, so use our own.
#include <cmath>

namespace plasma {
constexpr double kPi = 3.14159265358979323846;
}
