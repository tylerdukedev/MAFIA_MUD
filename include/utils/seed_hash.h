#pragma once

#include <cstdint>

namespace Utils {

uint32_t hashSeedMix(uint64_t seed, int32_t x, int32_t y);

} // namespace Utils
