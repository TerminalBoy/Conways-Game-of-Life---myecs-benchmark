#include <cstddef>
#include <iostream>
#include <vector>
#include "../dependencies/Custom_ECS/include/EntityComponentSystem.hpp"
#include <SFML/Graphics.hpp>

namespace cgl { // Conways's Game of Life
  // what shoud we call the cells / the live or dead entities ? 
  // we are calling it entity/entities
  
  namespace display_metadata {
    component::type::WidthPix CellWidth{};
    component::type::HeightPix CellHeight{};

    component::type::WidthGrid GridWidth{};
    component::type::HeightGrid GridHeight{};

    namespace cell_color_dead { // RGB
      std::int32_t r = 255; // picked from coolors.io
      std::int32_t g = 251;
      std::int32_t b = 189;
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

  void create_entities(std::vector<entity>& entity_array, std::size_t amount) {
    entity_array.resize(amount);
    for (std::size_t i = 0; i < amount; i++) {
      entity_array[i] = myecs::create_entity();
    }
  }

  void init_entities(const std::vector<entity>& entity_array, const component::type::WidthPix cell_width, component::type::HeightPix cell_height) {
    std::size_t entity_array_size = entity_array.size();
    for (std::size_t i = 0; i < entity_array_size; i++) {
      myecs::add_comp_to<comp::position>(entity_array[i]);
      myecs::add_comp_to<comp::position_grid>(entity_array[i]);
      myecs::add_comp_to<comp::rectangle>(entity_array[i]);
      myecs::add_comp_to<comp::alive>(entity_array[i]);
      myecs::add_comp_to<comp::color>(entity_array[i]);

      ecs_access(comp::alive, entity_array[i], value).set(false);

      ecs_access(comp::rectangle, entity_array[i], width).set(cell_width.get());
      ecs_access(comp::rectangle, entity_array[i], height).set(cell_height.get());
      
      ecs_access(comp::color, entity_array[i], r) = display_metadata::cell_color_dead::r;
      ecs_access(comp::color, entity_array[i], g) = display_metadata::cell_color_dead::g;
      ecs_access(comp::color, entity_array[i], b) = display_metadata::cell_color_dead::b;
    }
  }

  void init_entities_pos(const std::vector<entity>& entity_array,
                         const component::type::WidthGrid display_width_grid, 
                         const component::type::HeightGrid display_height_grid,
                         const component::type::WidthPix cell_width,
                         const component::type::HeightPix cell_height) {

    std::size_t entity_array_size = entity_array.size();
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::size_t entity_index = 0;
    std::int32_t x_pix = 0;
    std::int32_t y_pix = 0;

    assert(display_height_grid.get() * display_width_grid.get() == entity_array_size && "Entity array not equal to width * height");

    for (y = 0; y < display_height_grid.get(); ++y) {
      for (x = 0; x < display_width_grid.get(); ++x) {

        x_pix = x * cell_width.get();
        y_pix = y * cell_height.get();
        ecs_access(comp::position, entity_array[entity_index], x).set(x_pix);
        ecs_access(comp::position, entity_array[entity_index], y).set(y_pix);
        entity_index = entity_index + 1;
      
      }
    }

  }

  void update_entities_VertexArray(const std::vector<entity>& entity_array) {
    std::size_t entity_array_size = entity_array.size();
    std::size_t base = 0;
    sf::Color current_entity_color;
    const std::int32_t width = ecs_access(comp::rectangle, entity_array.front(), width).get();
    const std::int32_t height = ecs_access(comp::rectangle, entity_array.front(), height).get();

    for (std::size_t i = 0; i < entity_array_size; i++) {
      
      const std::int32_t x = ecs_access(comp::position, entity_array[i], x).get();
      const std::int32_t y = ecs_access(comp::position, entity_array[i], y).get();

      current_entity_color.r = ecs_access(comp::color, entity_array[i], r);
      current_entity_color.g = ecs_access(comp::color, entity_array[i], g);
      current_entity_color.b = ecs_access(comp::color, entity_array[i], b);

      base = i * 4;

      Renderables::entities_VertexArray[base + 0].position.x = x;          // top left
      Renderables::entities_VertexArray[base + 0].position.y = y;

      Renderables::entities_VertexArray[base + 1].position.x = x + width;  // top right
      Renderables::entities_VertexArray[base + 1].position.y = y;

      Renderables::entities_VertexArray[base + 2].position.x = x + width;  // bottom right
      Renderables::entities_VertexArray[base + 2].position.y = y + height; 

      Renderables::entities_VertexArray[base + 3].position.x = x;
      Renderables::entities_VertexArray[base + 3].position.y = y + height; // bottom left

      Renderables::entities_VertexArray[base + 0].color = current_entity_color;
      Renderables::entities_VertexArray[base + 1].color = current_entity_color;
      Renderables::entities_VertexArray[base + 2].color = current_entity_color;
      Renderables::entities_VertexArray[base + 3].color = current_entity_color;

    }
    
  }

  void init_entities_VertexArray(const std::vector<entity>& entity_array) {
    Renderables::entities_VertexArray.setPrimitiveType(sf::Quads);
    Renderables::entities_VertexArray.resize(entity_array.size() * 4);
    

    update_entities_VertexArray(entity_array);
  }

  void print_entities_pos(std::vector<entity>& entity_array) {
    for (std::size_t i = 0; i < entity_array.size(); i++) {
      std::cout << "ecs_access(comp::position, entity_array[" << i << "], x).get() = " << ecs_access(comp::position, entity_array[i], x).get() << std::endl;
      std::cout << "ecs_access(comp::position, entity_array[" << i << "], y).get() = " << ecs_access(comp::position, entity_array[i], y).get() << std::endl;
    }
  }

  void update_border_VertexArray() {
    std::int32_t total_h = display_metadata::GridHeight.get() + 1; // total horizontal lines, includes overlapped
    std::int32_t total_v = display_metadata::GridWidth.get() + 1; // total vertical lines, includes overlapped

    std::int32_t max_x = (display_metadata::GridWidth.get() * display_metadata::CellWidth.get()) - 1;
    std::int32_t max_y = (display_metadata::GridHeight.get() * display_metadata::CellHeight.get()) - 1;
    
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

    Renderables::border_horizontal[base + 0].position.x = x;
    Renderables::border_horizontal[base + 0].position.y = y;

    Renderables::border_horizontal[base + 1].position.x = x + width; // top right
    Renderables::border_horizontal[base + 1].position.y = y;

    Renderables::border_horizontal[base + 2].position.x = x + width; // bottom right 
    Renderables::border_horizontal[base + 2].position.y = y + height;

    Renderables::border_horizontal[base + 3].position.x = x; // bottom left 
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
      (display_metadata::GridHeight.get() + 1) * 4
    );

    Renderables::border_vertical.resize(
      (display_metadata::GridWidth.get() + 1) * 4
    );

    update_border_VertexArray();
  }

}

int main() {

  cgl::display_metadata::GridWidth.set(60);
  cgl::display_metadata::GridHeight.set(35);

  cgl::display_metadata::CellWidth.set(20);
  cgl::display_metadata::CellHeight.set(20);
  

  const component::type::WidthPix DisplayWindow_Width{ cgl::display_metadata::CellWidth.get() * cgl::display_metadata::GridWidth.get()};
  const component::type::HeightPix DisplayWindow_Height{ cgl::display_metadata::CellHeight.get() * cgl::display_metadata::GridHeight.get() };

  sf::RenderWindow DisplayWindow(sf::VideoMode(DisplayWindow_Width.get(), DisplayWindow_Height.get()), "Hellow world");
  sf::Event event;
  DisplayWindow.setFramerateLimit(60);

  
  std::vector<entity> entity_array;

  cgl::create_entities(
    entity_array, 
    cgl::display_metadata::GridWidth.get() * cgl::display_metadata::GridHeight.get()
  ); // creates the total number of entities
  

  cgl::init_entities(
    entity_array, 
    cgl::display_metadata::CellWidth, 
    cgl::display_metadata::CellHeight
  ); // adds necessarry components to the entities
  

  cgl::init_entities_pos(
    entity_array,
    cgl::display_metadata::GridWidth,
    cgl::display_metadata::GridHeight,
    cgl::display_metadata::CellWidth,
    cgl::display_metadata::CellHeight
  );
  
  cgl::init_entities_VertexArray(entity_array);

  cgl::init_border_VertexArray();
  
  // first we need to make DisplayWindow_Width * DisplayWindow_Height no. of entites
  /*
  for (std::size_t i = 0; i < entity_array.size(); i++) {
    std::cout << "ent X = " << ecs_access(comp::position, entity_array[i], x).get()
      << "\n" << "ent Y = " << ecs_access(comp::position, entity_array[i], y).get() << std::endl;
  }
  */

  std::cout << "Display max_x = " << DisplayWindow.getSize().x << std::endl;

  while (DisplayWindow.isOpen()) {
    
    while (DisplayWindow.pollEvent(event)) {
      if (event.type == sf::Event::Closed) DisplayWindow.close();
    }
    //cgl::update_entities_VertexArray(entity_array);
    DisplayWindow.clear(sf::Color::Black);
    DisplayWindow.draw(cgl::Renderables::entities_VertexArray);
    DisplayWindow.draw(cgl::Renderables::border_horizontal);
    DisplayWindow.draw(cgl::Renderables::border_vertical);
    
    DisplayWindow.display();
  }

  return 0;
}