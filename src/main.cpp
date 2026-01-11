//
//
// "Implicity kills simplicity, be loud, be clear and hold solid ground"  
//                                                            ~ Probably Me
//
//
#include <cstddef>
#include <iostream>
#include <stdio.h>
#include <vector>
#include "../dependencies/Custom_ECS/include/EntityComponentSystem.hpp"
#include "../dependencies/RNG/include/random.hpp" // seed based random number generator - xorshift32
#include <SFML/Graphics.hpp>

// PLEASE READ DOCUMENATTION BEFORE FORKING OR CONTRIBUTING
// 
// NOTE : 
// 
// PHYSICAL GRID IS WHAT EXISTS IN ARRAY/MEMORY !!!
// 
// LOGICAL GRID IS JUST A LOGICAL MAPPING/(VIEW ONLY) ON TOP OF THE PHYSICAL GRID
// LOGICAL GRID DOES NOT EXISTS IN THE MEMORY, ITS A SUBSET OF THE PHYSICAL GRID // ACCESSED USING DIFFERENT METHODS
// 
// 
// LOGICAL GRID EXISTS BECAUSE THE PHYSICAL GRID COMPRISES OF PADDINGS TO IMPORVE PERFOMANCE IN HOT LOOPS AS BY MINIMIZING BRANCHING

namespace cgl { // Conways's Game of Life
  // what shoud we call the cells / the live or dead entities ? 
  // we are calling it entity/entities
  
  namespace display_metadata { //
    
    using namespace component::type;
    
    WidthPix CellWidth{};
    HeightPix CellHeight{};

    WidthGrid Logical_GridWidth{}; // logical
    HeightGrid Logical_GridHeight{}; // logical

    WidthGrid Physical_GridWidth{}; // physical
    HeightGrid Physical_GridHeight{}; // physical

    namespace cell_color_dead { // RGB
      std::uint8_t r = 255; // picked from coolors.io
      std::uint8_t g = 251;
      std::uint8_t b = 189;
    }

    namespace cell_color_alive {
      std::uint8_t r = 59; // picked from coolors.io
      std::uint8_t g = 193;
      std::uint8_t b = 74;
    }

  }

  namespace grid_metadata {
    std::int32_t padding{1}; // physical padding around the edges
    std::int32_t total_logical{}; // total logical cells/entities
  }

  // WORKINGS ARE ALWAYS DONE ON THE PHYSICAL GRID, as PHYSICAL GRID IS WHAT EXISTS IN ARRAY/MEMORY !!!
  // LOGICAL GRID IS JUST A LOGICAL MAPPING/(VIEW ONLY) ON TOP OF THE PHYSICAL GRID
  // LOGICAL GRID DOES NOT EXISTS IN THE MEMORY, ITS A SUBSET OF THE PHYSICAL GRID
  namespace grid_helper_p { // _p = physical, refers to the actual physical(in memory/array) grid 
    using namespace component::type;
    PosPix_x grid_x_to_pixel__x(PosGrid_x physical_x) {
      return PosPix_x{ physical_x.get() * cgl::display_metadata::CellWidth.get() };
    }

    PosPix_y grid_y_to_pixel__y(PosGrid_y physical_y) {
      return PosPix_y{ physical_y.get() * cgl::display_metadata::CellHeight.get() };
    }

    PosGrid_x pixel_x_to_grid__x(std::int32_t x) {
      return PosGrid_x{ x / cgl::display_metadata::CellWidth.get() };
    }

    PosGrid_y pixel_y_to_grid__y(std::int32_t y) {
      return PosGrid_y{ y / cgl::display_metadata::CellHeight.get() };
    }

    std::int32_t grid_xy_to_array_index(PosGrid_x physical_x, PosGrid_y physical_y) {
      return (physical_y.get() * cgl::display_metadata::Physical_GridWidth.get()) + physical_x.get();
    }
  }
  

  // WORKINGS ARE ALWAYS DONE ON THE PHYSICAL GRID, as PHYSICAL GRID IS WHAT EXISTS IN ARRAY/MEMORY !!!
  // LOGICAL GRID IS JUST A LOGICAL MAPPING/(VIEW ONLY) ON TOP OF THE PHYSICAL GRID
  // LOGICAL GRID DOES NOT EXISTS IN THE MEMORY, ITS A SUBSET OF THE PHYSICAL GRID
  // 
  // Input = logical | Output = physical
  // _lo = logical, refers to the entities DISPLAYED in the grid // NEVER RETURNS A LOGICAL CO-ORDINATE
  namespace grid_helper_lo { 
    using namespace component::type;
    /*
    PosPix_x grid_x_to_pixel__x(PosGrid_x x) {
      return PosPix_x{ x.get() * cgl::display_metadata::CellWidth.get() };
    }

    PosPix_y grid_y_to_pixel__y(PosGrid_y y) {
      return PosPix_y{ y.get() * cgl::display_metadata::CellHeight.get() };
    }

    PosGrid_x pixel_x_to_grid__x(std::int32_t x) {
      return PosGrid_x{ x / cgl::display_metadata::CellWidth.get() };
    }

    PosGrid_y pixel_y_to_grid__y(std::int32_t y) {
      return PosGrid_y{ y / cgl::display_metadata::CellHeight.get() };
    }
    */

    std::size_t grid_xy_to_array_index(PosGrid_x logical_x, PosGrid_y logical_y) { // returns a physical index from logical co-ordinates
      logical_x.set(logical_x.get() + cgl::grid_metadata::padding); // increment both by the padding to convert to physical cords
      logical_y.set(logical_y.get() + cgl::grid_metadata::padding);
      return cgl::grid_helper_p::grid_xy_to_array_index(logical_x, logical_y);
    }

    // THIS RETURNS A LOGICAL INDEX FROM LOGICAL CORDS
    std::size_t grid_xy_to_array_index_RETURN_LOGICAL(PosGrid_x logical_x, PosGrid_y logical_y) { // returns a physical index from logical co-ordinates
      return ( logical_y.get() * cgl::display_metadata::Logical_GridWidth.get() ) + logical_x.get();
    }
    
    PosGrid_x grid_xLogical_to_grid_xPhysical(PosGrid_x logical_x) {
      return PosGrid_x{ logical_x.get() + cgl::grid_metadata::padding };
    }
  
    PosGrid_y grid_yLogical_to_grid_yPhysical(PosGrid_y logical_y) {
      return PosGrid_y{ logical_y.get() + cgl::grid_metadata::padding };
    }

  }

  struct Renderables {
    static inline sf::VertexArray entities_VertexArray;
    static inline sf::VertexArray border_vertical;
    static inline sf::VertexArray border_horizontal;
    static inline sf::Color entities_Color;
  };
  
  int cgl_true = 1;
  int cgl_false = 0;

  namespace grid_iterator::for_each {
  
    template <typename key, typename link, typename Fn>
    void physical_cell(const myecs::sparse_set<key, link>& cell_index_to_entity, Fn&& task) { //
      using namespace component::type;

      const WidthGrid physical_width{ cgl::display_metadata::Physical_GridWidth };
      const HeightGrid physical_height{ cgl::display_metadata::Physical_GridHeight };      

      for (PosGrid_y physical_y{ 0 }; physical_y.get() < physical_height.get(); physical_y.set(physical_y.get() + 1)) {
        for (PosGrid_x physical_x{ 0 }; physical_x.get() < physical_width.get(); physical_x.set(physical_x.get() + 1)) {
          
          const std::size_t index = cgl::grid_helper_p::grid_xy_to_array_index(physical_x, physical_y);
          task(physical_x, physical_y, index); // perfectly forwaded lamda

        }
      }

    }

    template <typename key, typename link, typename Fn>
    void logical_cell(const myecs::sparse_set<key, link>& cell_index_to_entity, Fn&& task) {
      using namespace component::type;

      const WidthGrid logical_width{ cgl::display_metadata::Logical_GridWidth };
      const HeightGrid logical_height{ cgl::display_metadata::Logical_GridHeight };


      for (PosGrid_y logical_y{ 0 }; logical_y.get() < logical_height.get(); logical_y.set(logical_y.get() + 1)) {
        for (PosGrid_x logical_x{ 0 }; logical_x.get() < logical_width.get(); logical_x.set(logical_x.get() + 1)) {
              
          const std::size_t index = cgl::grid_helper_lo::grid_xy_to_array_index(logical_x, logical_y);
          task(logical_x, logical_y, index);
        
        }
      }
    }

    template <typename key, typename link, typename Fn>
    void ranged_physical_cell(const myecs::sparse_set<key, link>& cell_index_to_entity,
                              const component::type::PosGrid_x physical_x,
                              const component::type::PosGrid_y physical_y,
                              const component::type::WidthGrid range_width,
                              const component::type::HeightGrid range_height,
                              Fn&& task) { //

      using namespace component::type;

      const PosGrid_x start_x{ physical_x };
      const PosGrid_y start_y{ physical_y };

      const PosGrid_x end_x{ physical_x.get() + range_width.get() };
      const PosGrid_y end_y{ physical_y.get() + range_height.get() };

      for (PosGrid_y current_y{ start_y }; current_y.get() < end_y.get(); current_y.set(current_y.get() + 1)) {
        for (PosGrid_x current_x{ start_x }; current_x.get() < end_x.get(); current_x.set(current_x.get() + 1)) {

          const std::size_t index = cgl::grid_helper_p::grid_xy_to_array_index(current_x, current_y);
          task(current_x, current_y, index); // perfectly forwaded lamda

        }
      }

    }

  }

  template <typename key, typename link>
  void create_entities(myecs::sparse_set<key, link>& cell_index_to_entity, myecs::sparse_set<key, link>& NextGen_buffer, component::type::WidthGrid logical_width, component::type::HeightGrid logical_height) {
    using namespace cgl::grid_metadata;

    cgl::display_metadata::Physical_GridWidth.set(logical_width.get() + (padding * 2));
    cgl::display_metadata::Physical_GridHeight.set(logical_height.get() + (padding * 2));

    cgl::grid_metadata::total_logical = logical_width.get() * logical_height.get();

    std::int32_t amount = display_metadata::Physical_GridWidth.get() * display_metadata::Physical_GridHeight.get(); 
                                                                  // logical grid stays width * height
    for (std::size_t i = 0; i < amount; i++) {
      cell_index_to_entity.insert(i, myecs::create_entity());
      NextGen_buffer.insert(i, myecs::create_entity());
    }
  }
  
  template <typename key, typename link>
  void init_entities(const myecs::sparse_set<key, link>& cell_index_to_entity, const myecs::sparse_set<key, link>& NextGen_buffer, const component::type::WidthPix cell_width, component::type::HeightPix cell_height, std::uint32_t initial_seed = 0) {
    using namespace component::type;
    std::size_t total_entities = cell_index_to_entity.dense.size(); // size of total data
    std::size_t index = 0;

    for (std::size_t i = 0; i < total_entities; i++) {
      myecs::add_comp_to<comp::position>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::position_grid>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::rectangle>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::alive>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::color>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::neighbour>(cell_index_to_entity.at(i));

      myecs::add_comp_to<comp::alive>(NextGen_buffer.at(i));
    }

    // unnessary component removal from the padded always dead entities

     // top left to top right (inclusive)
    for (component::type::PosGrid_x x{0}; x.get() < cgl::display_metadata::Physical_GridWidth.get(); x.set(x.get() + 1)) { 
      
      const PosGrid_y y{ 0 }; // extreme top
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));
    
      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
      ecs_access(comp::alive, NextGen_buffer.at(index), value).set(false);
    }

     // bottom left to bottom right (inclusive)
    for (component::type::PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Physical_GridWidth.get(); x.set(x.get() + 1)) {

      const PosGrid_y y{ cgl::display_metadata::Physical_GridHeight.get() - 1 };  // extreme bottom
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));

      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
      ecs_access(comp::alive, NextGen_buffer.at(index), value).set(false);
    }

      // top left to bottom left (EXCLUSIVE)
    for (component::type::PosGrid_y y{ 1 }; y.get() < cgl::display_metadata::Physical_GridHeight.get() - 1; y.set(y.get() + 1)) {

      const PosGrid_x x{ 0 }; // extreme left
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));
    
      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
      ecs_access(comp::alive, NextGen_buffer.at(index), value).set(false);
    }

    // top right to bottom right (EXCLUSIVE)
    for (component::type::PosGrid_y y{ 1 }; y.get() < cgl::display_metadata::Physical_GridHeight.get() - 1; y.set(y.get() + 1)) {

      const PosGrid_x x{ cgl::display_metadata::Physical_GridWidth.get() - 1}; // extreme right
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));

      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
      ecs_access(comp::alive, NextGen_buffer.at(index), value).set(false);
    }

    // accessing logical entities (the ones which are actually displayed)
    // THIS NESTED LOOP OPERATES ON LOGICAL CORDS
    for (PosGrid_y y{ 0 }; y.get() < cgl::display_metadata::Logical_GridHeight.get(); y.set(y.get() + 1)) {
      for (PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Logical_GridWidth.get(); x.set(x.get() + 1)) {
        index = cgl::grid_helper_lo::grid_xy_to_array_index(x, y);
        
        
        ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(
          static_cast<bool>(mgl::xorshift32(initial_seed) % 2) // either 1 or 0
        );

        ecs_access(comp::rectangle, cell_index_to_entity.at(index), width).set(cell_width.get());
        ecs_access(comp::rectangle, cell_index_to_entity.at(index), height).set(cell_height.get());

        if (ecs_access(comp::alive, cell_index_to_entity.at(index), value).get() == false) {
          ecs_access(comp::color, cell_index_to_entity.at(index), r) = display_metadata::cell_color_dead::r;
          ecs_access(comp::color, cell_index_to_entity.at(index), g) = display_metadata::cell_color_dead::g;
          ecs_access(comp::color, cell_index_to_entity.at(index), b) = display_metadata::cell_color_dead::b;
        }
        else {
          ecs_access(comp::color, cell_index_to_entity.at(index), r) = display_metadata::cell_color_alive::r;
          ecs_access(comp::color, cell_index_to_entity.at(index), g) = display_metadata::cell_color_alive::g;
          ecs_access(comp::color, cell_index_to_entity.at(index), b) = display_metadata::cell_color_alive::b;
        }
      }
    }

  }

  template <typename key, typename link>
  void init_entities_pos(const myecs::sparse_set<key, link>& cell_index_to_entity) {
    using namespace component::type;

    std::size_t total_cells = cell_index_to_entity.dense.size();
    std::size_t cell_index = 0;
    std::int32_t x_pix = 0;
    std::int32_t y_pix = 0;

    assert(cgl::display_metadata::Physical_GridWidth.get() * cgl::display_metadata::Physical_GridHeight.get() == total_cells && "Entity array not equal to width * height");
    
    // accessing logical entities (the ones which are actually displayed)
    // THIS NESTED LOOP OPERATES ON LOGICAL CORDS
    for (PosGrid_y y{ 0 }; y.get() < cgl::display_metadata::Logical_GridHeight.get(); y.set(y.get() + 1)) {
      for (PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Logical_GridWidth.get(); x.set(x.get() + 1)) {

        x_pix = x.get() * display_metadata::CellWidth.get();
        y_pix = y.get() * display_metadata::CellHeight.get();;

        cell_index = cgl::grid_helper_lo::grid_xy_to_array_index(x, y);

        ecs_access(comp::position, cell_index_to_entity.at(cell_index), x).set(x_pix);
        ecs_access(comp::position, cell_index_to_entity.at(cell_index), y).set(y_pix);

        ecs_access(comp::position_grid, cell_index_to_entity.at(cell_index), x).set(x.get());
        ecs_access(comp::position_grid, cell_index_to_entity.at(cell_index), y).set(y.get());
        
      }
    }

  }

  template <typename key, typename link>
  void update_entities_VertexArray(const myecs::sparse_set<key, link>& cell_index_to_entity) {
    using namespace component::type;
    std::size_t base = 0;
    std::size_t i = 0; // plysical index for entity array
    std::size_t logical_i = 0; // logical index for accessing vertex arrays
    sf::Color current_entity_color;

    // accessing logical entities (the ones which are actually displayed)
    // THIS NESTED LOOP OPERATES ON LOGICAL CORDS
    for (PosGrid_y y{ 0 }; y.get() < cgl::display_metadata::Logical_GridHeight.get(); y.set(y.get() + 1)) {
      for (PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Logical_GridWidth.get(); x.set(x.get() + 1)) {

        i = cgl::grid_helper_lo::grid_xy_to_array_index(x, y);
        logical_i = cgl::grid_helper_lo::grid_xy_to_array_index_RETURN_LOGICAL(x, y);

        const std::int32_t x_pix = ecs_access(comp::position, cell_index_to_entity.at(i), x).get();
        const std::int32_t y_pix = ecs_access(comp::position, cell_index_to_entity.at(i), y).get();
        const std::int32_t width = ecs_access(comp::rectangle, cell_index_to_entity.at(i), width).get();
        const std::int32_t height = ecs_access(comp::rectangle, cell_index_to_entity.at(i), height).get();

        current_entity_color.r = ecs_access(comp::color, cell_index_to_entity.at(i), r);
        current_entity_color.g = ecs_access(comp::color, cell_index_to_entity.at(i), g);
        current_entity_color.b = ecs_access(comp::color, cell_index_to_entity.at(i), b);

        base = logical_i * 4;

        Renderables::entities_VertexArray[base + 0].position.x = x_pix;          // top left
        Renderables::entities_VertexArray[base + 0].position.y = y_pix;

        Renderables::entities_VertexArray[base + 1].position.x = x_pix + width;  // top right
        Renderables::entities_VertexArray[base + 1].position.y = y_pix;

        Renderables::entities_VertexArray[base + 2].position.x = x_pix + width;  // bottom right
        Renderables::entities_VertexArray[base + 2].position.y = y_pix + height;

        Renderables::entities_VertexArray[base + 3].position.x = x_pix;
        Renderables::entities_VertexArray[base + 3].position.y = y_pix + height; // bottom left

        Renderables::entities_VertexArray[base + 0].color = current_entity_color;
        Renderables::entities_VertexArray[base + 1].color = current_entity_color;
        Renderables::entities_VertexArray[base + 2].color = current_entity_color;
        Renderables::entities_VertexArray[base + 3].color = current_entity_color;

  
      }
    }
    
  }

  template <typename key, typename link>
  void init_entities_VertexArray(const myecs::sparse_set<key, link>& cell_index_to_entity) {
    Renderables::entities_VertexArray.setPrimitiveType(sf::Quads);
    Renderables::entities_VertexArray.resize(cgl::grid_metadata::total_logical * 4);
    

    update_entities_VertexArray(cell_index_to_entity);
  }


  template <typename key, typename link>
  void update_entities_VertexArray_state_only(const myecs::sparse_set<key, link>& cell_index_to_entity) {

    using namespace component::type;
    std::size_t base = 0;
    std::size_t i = 0; // plysical index for entity array
    std::size_t logical_i = 0; // logical index for accessing vertex arrays
    sf::Color current_entity_color;

    // accessing logical entities (the ones which are actually displayed)
    // THIS NESTED LOOP OPERATES ON LOGICAL CORDS
    for (PosGrid_y y{ 0 }; y.get() < cgl::display_metadata::Logical_GridHeight.get(); y.set(y.get() + 1)) {
      for (PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Logical_GridWidth.get(); x.set(x.get() + 1)) {

        i = cgl::grid_helper_lo::grid_xy_to_array_index(x, y);
        logical_i = cgl::grid_helper_lo::grid_xy_to_array_index_RETURN_LOGICAL(x, y);

        current_entity_color.r = ecs_access(comp::color, cell_index_to_entity.at(i), r);
        current_entity_color.g = ecs_access(comp::color, cell_index_to_entity.at(i), g);
        current_entity_color.b = ecs_access(comp::color, cell_index_to_entity.at(i), b);

        base = logical_i * 4;

        Renderables::entities_VertexArray[base + 0].color = current_entity_color;
        Renderables::entities_VertexArray[base + 1].color = current_entity_color;
        Renderables::entities_VertexArray[base + 2].color = current_entity_color;
        Renderables::entities_VertexArray[base + 3].color = current_entity_color;


      }
    }


  }

  void print_entities_pos(std::vector<entity>& entity_array) {
    for (std::size_t i = 0; i < entity_array.size(); i++) {
      std::cout << "ecs_access(comp::position, entity_array[" << i << "], x).get() = " << ecs_access(comp::position, entity_array[i], x).get() << std::endl;
      std::cout << "ecs_access(comp::position, entity_array[" << i << "], y).get() = " << ecs_access(comp::position, entity_array[i], y).get() << std::endl;
    }
  }

  void update_border_VertexArray() { // lo = logical
    // OPERATING ON THE VISUAL PART BY REFERRING TO LOGICAL GRID
    
    std::int32_t total_h = display_metadata::Logical_GridHeight.get() + 1; // total horizontal lines, includes overlapped
    std::int32_t total_v = display_metadata::Logical_GridWidth.get() + 1; // total vertical lines, includes overlapped

    std::int32_t max_x = (display_metadata::Logical_GridWidth.get() * display_metadata::CellWidth.get()) - 1;
    std::int32_t max_y = (display_metadata::Logical_GridHeight.get() * display_metadata::CellHeight.get()) - 1;
    
    std::int32_t width = 0; 
    std::int32_t height = 0;
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::size_t base = 0;
    
    // for horizontal lines -------------------------

    width = max_x + 1; // for horizontal lines
    height = 1;// for horizontal lines
    x = 0; // as they are horizontal lines, all starting x pos will be zero (0)
    for (std::int32_t i = 0; i < total_h; ++i) {
      
      base = i * 4;
     
      y = i * display_metadata::CellHeight.get();

      Renderables::border_horizontal[base + 0].position.x = x; // top left
      Renderables::border_horizontal[base + 0].position.y = y;

      Renderables::border_horizontal[base + 1].position.x = x + width; // top right
      Renderables::border_horizontal[base + 1].position.y = y;

      Renderables::border_horizontal[base + 2].position.x = x + width; // bottom right
      Renderables::border_horizontal[base + 2].position.y = y + height;
      
      Renderables::border_horizontal[base + 3].position.x = x;        // bottom left
      Renderables::border_horizontal[base + 3].position.y = y + height;

      Renderables::border_horizontal[base + 0].color = sf::Color::Cyan;
      Renderables::border_horizontal[base + 1].color = sf::Color::Cyan;
      Renderables::border_horizontal[base + 2].color = sf::Color::Cyan;
      Renderables::border_horizontal[base + 3].color = sf::Color::Cyan;
    }

    
    // dealing with the last quad
    base = (total_h - 1) * 4; // last quad index
    y = max_y; // we are drawing the last line at the last y index

    Renderables::border_horizontal[base + 0].position.x = x; // top left
    Renderables::border_horizontal[base + 0].position.y = y;

    Renderables::border_horizontal[base + 1].position.x = x + width; // top right
    Renderables::border_horizontal[base + 1].position.y = y;

    Renderables::border_horizontal[base + 2].position.x = x + width; // bottom right
    Renderables::border_horizontal[base + 2].position.y = y + height;

    Renderables::border_horizontal[base + 3].position.x = x;        // bottom left
    Renderables::border_horizontal[base + 3].position.y = y + height;

    


    // for vertical lines ------------------------
    width = 1;
    height = max_y + 1;
    y = 0; // as we are now dealing with vertical lines all starting y pos will be 0

    for (std::int32_t i = 0; i < total_v; ++i) { // vertical lines

      base = i * 4;
      x = i * display_metadata::CellWidth.get();

      Renderables::border_vertical[base + 0].position.x = x; //  top left
      Renderables::border_vertical[base + 0].position.y = y;

      Renderables::border_vertical[base + 1].position.x = x + width; // top right
      Renderables::border_vertical[base + 1].position.y = y; // top right

      Renderables::border_vertical[base + 2].position.x = x + width; // bottom right
      Renderables::border_vertical[base + 2].position.y = y + height;

      Renderables::border_vertical[base + 3].position.x = x; // bottom left 
      Renderables::border_vertical[base + 3].position.y = y + height;

      Renderables::border_vertical[base + 0].color = sf::Color::Cyan;
      Renderables::border_vertical[base + 1].color = sf::Color::Cyan;
      Renderables::border_vertical[base + 2].color = sf::Color::Cyan;
      Renderables::border_vertical[base + 3].color = sf::Color::Cyan;
    }

    base = (total_v - 1) * 4; // last quad index
    x = max_x;

    Renderables::border_vertical[base + 0].position.x = x; //  top left
    Renderables::border_vertical[base + 0].position.y = y;

    Renderables::border_vertical[base + 1].position.x = x + width; // top right
    Renderables::border_vertical[base + 1].position.y = y; // top right

    Renderables::border_vertical[base + 2].position.x = x + width; // bottom right
    Renderables::border_vertical[base + 2].position.y = y + height;

    Renderables::border_vertical[base + 3].position.x = x; // bottom left 
    Renderables::border_vertical[base + 3].position.y = y + height;

  }

  void init_border_VertexArray() {
    Renderables::border_vertical.setPrimitiveType(sf::Quads);
    Renderables::border_horizontal.setPrimitiveType(sf::Quads);
    
    Renderables::border_horizontal.resize(
      (display_metadata::Logical_GridHeight.get() + 1) * 4
    );

    Renderables::border_vertical.resize(
      (display_metadata::Logical_GridWidth.get() + 1) * 4
    );

    update_border_VertexArray();
  }


  template <typename key, typename link>
  std::int32_t nth_cell_alive_neighbours(const myecs::sparse_set<key, link>& cell_index_to_entity,
    const component::type::PosGrid_x logical_x,
    const component::type::PosGrid_y logical_y) { // takes logical index
    using namespace component::type;

    const std::size_t center_cell_index = cgl::grid_helper_lo::grid_xy_to_array_index(logical_x, logical_y);

    const std::int32_t TopmostLeft_offset = cgl::grid_metadata::padding;

    const PosGrid_x physical_neighbour_TopmostLeft_x{ grid_helper_lo::grid_xLogical_to_grid_xPhysical(logical_x).get() - TopmostLeft_offset };
    const PosGrid_y physical_neighbour_TopmostLeft_y{ grid_helper_lo::grid_yLogical_to_grid_yPhysical(logical_y).get() - TopmostLeft_offset };

    const PosGrid_x start_x{ physical_neighbour_TopmostLeft_x };
    const PosGrid_y start_y{ physical_neighbour_TopmostLeft_y };

    const WidthGrid range_width{ (cgl::grid_metadata::padding * 2) + 1 }; // *2 because padding is in both sides and + 1 to also include the center (main) cell
    const HeightGrid range_height{ (cgl::grid_metadata::padding * 2) + 1 };

    std::int32_t total_neighbours{ 0 };

    cgl::grid_iterator::
    for_each::ranged_physical_cell(
      cell_index_to_entity,
      start_x,
      start_y,
      range_width,
      range_height,
      [&](auto x, auto y, std::size_t index) {
        total_neighbours += ecs_access(comp::alive, cell_index_to_entity.at(index), value).get();
      }
    );


    total_neighbours -= ecs_access(comp::alive, cell_index_to_entity.at(center_cell_index), value).get();
    return total_neighbours;
  }


  template <typename key, typename link>
  void calculate_alive_neighbours(const myecs::sparse_set<key, link>& cell_index_to_entity) {
    
    using namespace component::type;

    std::int32_t index{};

    // iterating over the logical grid
    for (PosGrid_y logical_y{ 0 }; logical_y.get() < cgl::display_metadata::Logical_GridHeight.get(); logical_y.set(logical_y.get() + 1)) {
      for (PosGrid_x logical_x{ 0 }; logical_x.get() < cgl::display_metadata::Logical_GridWidth.get(); logical_x.set(logical_x.get() + 1)) {

        index = cgl::grid_helper_lo::grid_xy_to_array_index(logical_x, logical_y);

        ecs_access(comp::neighbour, cell_index_to_entity.at(index), count) =   
        nth_cell_alive_neighbours(
          cell_index_to_entity,
          logical_x,
          logical_y
        );

      }
    }
   
  }

  template <typename key, typename link>
  void print_everycell_neighbour_count(const myecs::sparse_set<key, link>& cell_index_to_entity) {
    using namespace cgl::grid_iterator;
    
    for_each::logical_cell(cell_index_to_entity,

      [&](auto x, auto y, std::size_t index) {
        
        const std::size_t logical_index = cgl::grid_helper_lo::grid_xy_to_array_index_RETURN_LOGICAL(x, y);
        
        std::cout << "cell " << logical_index << " total neighbours : "
          << ecs_access(comp::neighbour, cell_index_to_entity.at(index), count)
          << std::endl;
      }

    );
  }

  

  namespace rulebook {
    /* source: https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
      1.Any live cell with fewer than two live neighbours dies, as if by underpopulation.
      
      2.Any live cell with two or three live neighbours lives on to the next generation.
      
      3.Any live cell with more than three live neighbours dies, as if by overpopulation.
      
      4.Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
    */

    

    // rules implemented below
  }
}

namespace cgl::rulebook {
  void apply_rules(const entity current_id, const entity NextGen_id) {
   
    if (ecs_access(comp::alive, current_id, value).get() == true &&
        ecs_access(comp::neighbour, current_id, count) < 2) {

      ecs_access(comp::alive, NextGen_id, value).set(false);
    }
    else if (ecs_access(comp::alive, current_id, value).get() == true &&
             (ecs_access(comp::neighbour, current_id, count) == 2 || ecs_access(comp::neighbour, current_id, count) == 3)) {
      ecs_access(comp::alive, NextGen_id, value).set(true);
    }
    else if (ecs_access(comp::alive, current_id, value).get() == true &&
             ecs_access(comp::neighbour, current_id, count) > 3) {

      ecs_access(comp::alive, NextGen_id, value).set(false);
    }
    else if (ecs_access(comp::alive, current_id, value).get() == false &&
             ecs_access(comp::neighbour, current_id, count) == 3) {

      ecs_access(comp::alive, NextGen_id, value).set(true);
    }
    else {
      ecs_access(comp::alive, NextGen_id, value).set(false);
    }
  }
}

template <typename key, typename link>
void conways_game_of_life(const myecs::sparse_set<key, link>& cell_index_to_entity,
                          const myecs::sparse_set<key, link>& NextGen_buffer) {
  using namespace cgl::grid_iterator;

  for_each::logical_cell(cell_index_to_entity,
    [&](auto x, auto y, std::size_t index) {
      cgl::rulebook::apply_rules(cell_index_to_entity.at(index), NextGen_buffer.at(index));
    }
  );

  for_each::logical_cell(cell_index_to_entity,
    [&](auto x, auto y, std::size_t index) {
      
      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(
        ecs_access(comp::alive, NextGen_buffer.at(index), value).get()
      );

      if (ecs_access(comp::alive, cell_index_to_entity.at(index), value).get() == true) {

        ecs_access(comp::color, cell_index_to_entity.at(index), r) = cgl::display_metadata::cell_color_alive::r;
        ecs_access(comp::color, cell_index_to_entity.at(index), g) = cgl::display_metadata::cell_color_alive::g;
        ecs_access(comp::color, cell_index_to_entity.at(index), b) = cgl::display_metadata::cell_color_alive::b;

      }
      else {
        ecs_access(comp::color, cell_index_to_entity.at(index), r) = cgl::display_metadata::cell_color_dead::r;
        ecs_access(comp::color, cell_index_to_entity.at(index), g) = cgl::display_metadata::cell_color_dead::g;
        ecs_access(comp::color, cell_index_to_entity.at(index), b) = cgl::display_metadata::cell_color_dead::b;
      }

    }
  );

}

int main() {

  //std::uint32_t CGL_SEED = mgl::make_seed_xorshift32(); // mgl = my game library
  std::uint32_t CGL_SEED = 50;

  cgl::display_metadata::Logical_GridWidth.set(40);
  cgl::display_metadata::Logical_GridHeight.set(40);

  cgl::display_metadata::CellWidth.set(20);
  cgl::display_metadata::CellHeight.set(20);
  

  const component::type::WidthPix DisplayWindow_Width{ cgl::display_metadata::CellWidth.get() * cgl::display_metadata::Logical_GridWidth.get()};
  const component::type::HeightPix DisplayWindow_Height{ cgl::display_metadata::CellHeight.get() * cgl::display_metadata::Logical_GridHeight.get() };

  sf::RenderWindow DisplayWindow(sf::VideoMode(DisplayWindow_Width.get(), DisplayWindow_Height.get()), "Hellow world");
  sf::Event event;
  DisplayWindow.setFramerateLimit(4);

  
  myecs::sparse_set<std::uint32_t, entity> cell_index_to_entity; // REFERRES TO PHYSICAL //  will have padding of one cell around the edges
  myecs::sparse_set<std::uint32_t, entity> NextGen_buffer; // will only hold dead or alive, only for the next generation / tick
 
  cgl::create_entities(
    cell_index_to_entity, NextGen_buffer,
    cgl::display_metadata::Logical_GridWidth, 
    cgl::display_metadata::Logical_GridHeight
  ); // creates the total number of entities
  

  cgl::init_entities(
    cell_index_to_entity, NextGen_buffer,
    cgl::display_metadata::CellWidth, 
    cgl::display_metadata::CellHeight,
    CGL_SEED // <-- this creates noise (random dead or alive)
  ); // adds necessarry components to the entities
  

  cgl::init_entities_pos(cell_index_to_entity);
  
  cgl::init_entities_VertexArray(cell_index_to_entity);

  cgl::init_border_VertexArray();
  
  //cgl::calculate_alive_neighbours(cell_index_to_entity);
  //cgl::print_everycell_neighbour_count(cell_index_to_entity);

  while (DisplayWindow.isOpen()) {
    
    while (DisplayWindow.pollEvent(event)) {
      if (event.type == sf::Event::Closed) DisplayWindow.close();
    }

    cgl::calculate_alive_neighbours(cell_index_to_entity);

    conways_game_of_life(cell_index_to_entity, NextGen_buffer);

    cgl::update_entities_VertexArray_state_only(cell_index_to_entity);
    DisplayWindow.clear(sf::Color::Black);
    DisplayWindow.draw(cgl::Renderables::entities_VertexArray);
    DisplayWindow.draw(cgl::Renderables::border_horizontal);
    DisplayWindow.draw(cgl::Renderables::border_vertical);
    
    DisplayWindow.display();
  }

  return 0;
}