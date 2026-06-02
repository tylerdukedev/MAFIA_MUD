#include "utils/seed_hash.h"

namespace Utils {

uint32_t hashSeedMix(uint64_t seed, int32_t x, int32_t y) {
    uint64_t hash = seed;
    hash ^= static_cast<uint64_t>(x) * 0x9E3779B97F4A7C15ULL;
    hash ^= static_cast<uint64_t>(y) * 0xBF58476D1CE4E5B9ULL;
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;
    return static_cast<uint32_t>(hash);
}

} // namespace Utils
