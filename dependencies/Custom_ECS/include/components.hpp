#pragma once

#include "memory.hpp"
//#include "../SFML/include/SFML/Graphics.hpp"
//#include <SFML/Graphics.hpp>
// ##-START_PARSE-##

namespace comp {
  struct position {
    myecs::d_array<component::type::PosPix_x> x;
    myecs::d_array<component::type::PosPix_y> y;
  };

  struct position_grid {
    myecs::d_array<component::type::PosGrid_x> x;
    myecs::d_array<component::type::PosGrid_y> y;
  };

  struct rectangle {
    myecs::d_array<component::type::WidthPix> width;
    myecs::d_array<component::type::HeightPix> height;
  };

  struct circle {
    myecs::d_array<component::type::RadiusPix> radius;
  };

  struct color {
    myecs::d_array<std::uint8_t> r;
    myecs::d_array<std::uint8_t> g;
    myecs::d_array<std::uint8_t> b;
  };

  struct physics {
    myecs::d_array<component::type::Speed> speed;
    myecs::d_array<component::type::Direction> direction;
  };

  struct speed_handler {
    myecs::d_array<float> move_interval;
    myecs::d_array<float> time_accumulator;
    myecs::d_array<float> dt;
  };

  struct magnitude {
    myecs::d_array<component::type::Size> size;
  };

  struct alive {
    myecs::d_array<component::type::Bool> value;
  };

}

// ##-STOP_PARSE-##
