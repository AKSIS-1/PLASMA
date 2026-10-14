#pragma once
#include <cmath>

#include "Math.h"
#include <complex>
#include <vector>

namespace plasma {

using Cplx = std::complex<double>;

// In-place radix-2 FFT. sign=-1 forward, +1 inverse (unscaled).
inline void fft(std::vector<Cplx>& a, int sign) {
  const size_t n = a.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) std::swap(a[i], a[j]);
  }
  for (size_t len = 2; len <= n; len <<= 1) {
    const double ang = sign * 2.0 * kPi / (double)len;
    const Cplx wl(std::cos(ang), std::sin(ang));
    for (size_t i = 0; i < n; i += len) {
      Cplx w(1.0, 0.0);
      for (size_t k = 0; k < len / 2; ++k) {
        const Cplx u = a[i + k], v = a[i + k + len / 2] * w;
        a[i + k] = u + v;
        a[i + k + len / 2] = u - v;
        w *= wl;
      }
    }
  }
}

}  // namespace plasma
