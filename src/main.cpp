//
//
// "Implicity kills simplicity, be loud, be clear and hold solid ground"  
//                                                            ~ Probably Me
//
//
#include <cstddef>
#include <iostream>
#include <vector>
#include "../dependencies/Custom_ECS/include/EntityComponentSystem.hpp"
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
  }

  struct Renderables {
    static inline sf::VertexArray entities_VertexArray;
    static inline sf::VertexArray border_vertical;
    static inline sf::VertexArray border_horizontal;
    static inline sf::Color entities_Color;
  };
  
  int cgl_true = 1;
  int cgl_false = 0;

  template <typename key, typename link>
  void create_entities(myecs::sparse_set<key, link>& cell_index_to_entity, component::type::WidthGrid logical_width, component::type::HeightGrid logical_height) {
    using namespace cgl::grid_metadata;

    cgl::display_metadata::Physical_GridWidth.set(logical_width.get() + (padding * 2));
    cgl::display_metadata::Physical_GridHeight.set(logical_height.get() + (padding * 2));

    cgl::grid_metadata::total_logical = logical_width.get() * logical_height.get();

    std::int32_t amount = display_metadata::Physical_GridWidth.get() * display_metadata::Physical_GridHeight.get(); 
                                                                  // logical grid stays width * height
    for (std::size_t i = 0; i < amount; i++) {
      cell_index_to_entity.insert(i, myecs::create_entity());
    }
  }
  
  template <typename key, typename link>
  void init_entities(const myecs::sparse_set<key, link>& cell_index_to_entity, const component::type::WidthPix cell_width, component::type::HeightPix cell_height) {
    using namespace component::type;
    std::size_t total_entities = cell_index_to_entity.dense.size(); // size of total data
    std::size_t index = 0;

    for (std::size_t i = 0; i < total_entities; i++) {
      myecs::add_comp_to<comp::position>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::position_grid>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::rectangle>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::alive>(cell_index_to_entity.at(i));
      myecs::add_comp_to<comp::color>(cell_index_to_entity.at(i));
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
    }

     // bottom left to bottom right (inclusive)
    for (component::type::PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Physical_GridWidth.get(); x.set(x.get() + 1)) {

      const PosGrid_y y{ cgl::display_metadata::Physical_GridHeight.get() - 1 };  // extreme bottom
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));

      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
    }

      // top left to bottom left (EXCLUSIVE)
    for (component::type::PosGrid_y y{ 1 }; y.get() < cgl::display_metadata::Physical_GridHeight.get() - 1; y.set(y.get() + 1)) {

      const PosGrid_x x{ 0 }; // extreme left
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));
    
      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
    }

    // top right to bottom right (EXCLUSIVE)
    for (component::type::PosGrid_y y{ 1 }; y.get() < cgl::display_metadata::Physical_GridHeight.get() - 1; y.set(y.get() + 1)) {

      const PosGrid_x x{ cgl::display_metadata::Physical_GridWidth.get() - 1}; // extreme right
      index = cgl::grid_helper_p::grid_xy_to_array_index(x, y);

      myecs::remove_comp_from<comp::position>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::rectangle>(cell_index_to_entity.at(index));
      myecs::remove_comp_from<comp::color>(cell_index_to_entity.at(index));

      ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false); // permanentely dead
    }

    // accessing logical entities (the ones which are actually displayed)
    // THIS NESTED LOOP OPERATES ON LOGICAL CORDS
    for (PosGrid_y y{ 0 }; y.get() < cgl::display_metadata::Logical_GridHeight.get(); y.set(y.get() + 1)) {
      for (PosGrid_x x{ 0 }; x.get() < cgl::display_metadata::Logical_GridWidth.get(); x.set(x.get() + 1)) {
        index = cgl::grid_helper_lo::grid_xy_to_array_index(x, y);
        
        ecs_access(comp::alive, cell_index_to_entity.at(index), value).set(false);

        ecs_access(comp::rectangle, cell_index_to_entity.at(index), width).set(cell_width.get());
        ecs_access(comp::rectangle, cell_index_to_entity.at(index), height).set(cell_height.get());

        ecs_access(comp::color, cell_index_to_entity.at(index), r) = display_metadata::cell_color_dead::r;
        ecs_access(comp::color, cell_index_to_entity.at(index), g) = display_metadata::cell_color_dead::g;
        ecs_access(comp::color, cell_index_to_entity.at(index), b) = display_metadata::cell_color_dead::b;

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

  void make_Nextgen_buffer() {}


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
  void apply_rule1(entity id) {

  }
}

int main() {

  cgl::display_metadata::Logical_GridWidth.set(20);
  cgl::display_metadata::Logical_GridHeight.set(20);

  cgl::display_metadata::CellWidth.set(20);
  cgl::display_metadata::CellHeight.set(20);
  

  const component::type::WidthPix DisplayWindow_Width{ cgl::display_metadata::CellWidth.get() * cgl::display_metadata::Logical_GridWidth.get()};
  const component::type::HeightPix DisplayWindow_Height{ cgl::display_metadata::CellHeight.get() * cgl::display_metadata::Logical_GridHeight.get() };

  sf::RenderWindow DisplayWindow(sf::VideoMode(DisplayWindow_Width.get(), DisplayWindow_Height.get()), "Hellow world");
  sf::Event event;
  DisplayWindow.setFramerateLimit(60);

  
  myecs::sparse_set<std::uint32_t, entity> cell_index_to_entity; // REFERRES TO PHYSICAL //  will have padding of one cell around the edges
  std::vector<entity> paddedNextgen_buffer; // will only hold grid position and dead or alive, only for the next generation / tick
 
  cgl::create_entities(
    cell_index_to_entity, 
    cgl::display_metadata::Logical_GridWidth, 
    cgl::display_metadata::Logical_GridHeight
  ); // creates the total number of entities
  

  cgl::init_entities(
    cell_index_to_entity, 
    cgl::display_metadata::CellWidth, 
    cgl::display_metadata::CellHeight
  ); // adds necessarry components to the entities
  

  cgl::init_entities_pos(cell_index_to_entity);
  
  cgl::init_entities_VertexArray(cell_index_to_entity);

  cgl::init_border_VertexArray();
  
  // first we need to make DisplayWindow_Width * DisplayWindow_Height no. of entites
  /*
  for (std::size_t i = 0; i < entity_array.size(); i++) {
    std::cout << "ent X = " << ecs_access(comp::position, entity_array[i], x).get()
      << "\n" << "ent Y = " << ecs_access(comp::position, entity_array[i], y).get() << std::endl;
  }
  */

  

  while (DisplayWindow.isOpen()) {
    
    while (DisplayWindow.pollEvent(event)) {
      if (event.type == sf::Event::Closed) DisplayWindow.close();
    }

    cgl::update_entities_VertexArray(cell_index_to_entity);
    DisplayWindow.clear(sf::Color::Black);
    DisplayWindow.draw(cgl::Renderables::entities_VertexArray);
    DisplayWindow.draw(cgl::Renderables::border_horizontal);
    DisplayWindow.draw(cgl::Renderables::border_vertical);
    
    DisplayWindow.display();
  }

  return 0;
}