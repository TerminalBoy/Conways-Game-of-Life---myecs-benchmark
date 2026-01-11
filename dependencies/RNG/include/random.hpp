#pragma once

#include <cstdint>
#include <chrono>
namespace mgl { // my game library
  
  std::uint32_t xorshift32(std::uint32_t& state) {
    state ^= state << 13; // magic constants
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  
  inline std::uint32_t make_seed_xorshift32() {
    std::uint64_t t = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return static_cast<std::uint32_t>(t ^ (t >> 32));
  }

}