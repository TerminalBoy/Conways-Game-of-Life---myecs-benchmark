#include <cstddef> // for std::size_t
#include <memory>
#include <iostream> // only  for debugging
#include <cstdlib>  
#include <vector>
#include <string>

#include <limits>
#include <cassert>

#include <algorithm>
#include <execution>

#include "memory.hpp"
#include "components.hpp"
#include "generated_components_create.hpp"
#include "generated_components_delete.hpp"





// 

// ECS : enitity component system 
// 
// why use it instead of OOP style
// : it provides better cache usage, by utilizing contigious memory and making the compiler easy to apply SIMD
// optimizations, lesser missed cache, lesser ram fetchinmg = overall better perfomance
// And its very easy to scale than the classic OOP structure

// Entities :
// its is just an id referrinmg to a game object (not the OOP one), like snake, snake bodypart, window, food

// Components :
// this the data of game objects stored in struct or arrays (SoA), acessed by specifying the entitiy id or by a internal Hash Map (unordered_map)

// System :
// this is the actual logic of the game, stored in functions

// 



// trying ECS


// Entity
// 
// entity defined in memory.hpp in "Dependencies/Custom_ECS/memory.hpp"
//
// using entity = std::size_t;
//

// COMPONENTS :

// defined in "components.hpp"


// SYSTEM :

// i am sorry for these messy comments, having a tough time making the code architecture, but i am tring my best

// creation of entity == entity++; // only counter is incremented

// initialization of component (from scratch) == first component is stored in heap with its adress stored in 
// static unique_ptr (just initial address storage) -> this is just the creation of component container / structure

// no bridging needed now!
// (only initiliazed if used -> type id dispatch, only compile-time)


// creation of component == as the component is initialized now -> we will increment its static size counter 
// to keep track of its array sizes at runtime -> bridging of components is done with the entities

// component contents acessed only by its static - unique_ptr


namespace myecs {
  // moved the global variables to myecs namespace
  entity GLOBAL_ENTITY_COUNTER = 0;

  constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();
  constexpr std::size_t MULTITHREADING_SEED = 50000; // (no. of allocations )from the threshold where multithreading should start


  // A new way to store metadata 
  // completely elimininates, function statics' guard checks,
  // increasing prefomance exponentially in large ecs data and systems
  // here the metadata is initialized before main() and dosent involve runtime guard checks
  // only pure fast, type id dispatched fetching // as fast as fetching a normal global variable
  // even at the cpu instruction level (no extra cycles)
  template <typename component>
  struct storage {
    inline static std::size_t size = 0;
    inline static std::unique_ptr<component> pointer = std::make_unique<component>();
    inline static myecs::d_array<std::size_t> sparse; // sparse<component>[entity_id] = component_index 
                                            //^^^^^^there is not even a need for a dense array because the components arrays are always dense by design
    inline static myecs::d_array<std::size_t> reverse_sparse; // reverse_sparse<component>[component_index] = owning_entity_id;
  };

  template <typename component>
  void sparse_allocator(const entity& id, std::size_t corresponding_comp_index) { // allocates and links the values
    assert(id < GLOBAL_ENTITY_COUNTER && "The provided entity never existed");
    std::size_t old_reverse_sparse_size = myecs::storage<component>::reverse_sparse.size();
    std::size_t old_sparse_size = myecs::storage<component>::sparse.size();


    // I dont know why guards, may it bring peace my mind
    if (old_sparse_size < GLOBAL_ENTITY_COUNTER)
      myecs::storage<component>::sparse.resize(GLOBAL_ENTITY_COUNTER);

    if (old_reverse_sparse_size < myecs::storage<component>::size)
      myecs::storage<component>::reverse_sparse.resize(myecs::storage<component>::size);

    // trying multithreading, i recently got introcuded to this concept
    if (GLOBAL_ENTITY_COUNTER - old_sparse_size >= MULTITHREADING_SEED) { // guards for where multithreading is not required (thread safety) preventing undefined behavior
      //std::cerr << "\nYes multithreading\n";
      std::fill(
        std::execution::par_unseq,
        myecs::storage<component>::sparse.begin() + old_sparse_size,
        myecs::storage<component>::sparse.end(),
        INVALID_INDEX
      );
    }
    else { // no multithreading
      //std::cerr << "\nNo multithreading\n";
      std::fill(
        myecs::storage<component>::sparse.begin() + old_sparse_size,
        myecs::storage<component>::sparse.end(),
        INVALID_INDEX
      );
    }

    if (GLOBAL_ENTITY_COUNTER - old_reverse_sparse_size >= MULTITHREADING_SEED) {
      //std::cerr << "\nYes multithreading\n";
      std::fill(
        std::execution::par_unseq,
        myecs::storage<component>::reverse_sparse.begin() + old_reverse_sparse_size,
        myecs::storage<component>::reverse_sparse.end(),
        INVALID_INDEX
      );
    }
    else {
      //std::cerr << "\nNo multithreading\n";
      std::fill(
        myecs::storage<component>::reverse_sparse.begin() + old_reverse_sparse_size,
        myecs::storage<component>::reverse_sparse.end(),
        INVALID_INDEX
      );
    }

    myecs::storage<component>::sparse[id] = corresponding_comp_index;
    myecs::storage<component>::reverse_sparse[corresponding_comp_index] = id;
  }


  // ecs functions

  entity create_entity() { // first step
    GLOBAL_ENTITY_COUNTER++;
    return GLOBAL_ENTITY_COUNTER - 1;
  }

  template <typename component> // helper only, no need to call explictly
  inline void entity_component_linker(const entity& id, std::size_t corresponding_comp_index) {
    assert(id < GLOBAL_ENTITY_COUNTER && "The provided entity never existed");
    sparse_allocator<component>(id, corresponding_comp_index);
  }


  template <typename component>
  bool has_component(const entity& id) { // (has branching) only to be used in asserts
    assert(id < GLOBAL_ENTITY_COUNTER && "The provided entity never existed");
    if (id < myecs::storage<component>::sparse.size() && myecs::storage<component>::sparse[id] != INVALID_INDEX) return true;
    return false;
  }

  template <typename component>
  bool has_entity(const std::size_t& comp_index) { // (has branching) only to be used in asserts
    if (comp_index < myecs::storage<component>::reverse_sparse.size() && myecs::storage<component>::reverse_sparse[comp_index] != INVALID_INDEX) return true;
    return false;
  }

  template <typename component>
  void add_comp_to(const entity& id) { // third step
    //assert((id >= myecs::storage<component>::sparse.size() || myecs::storage<component>::sparse[id] == INVALID_INDEX) && "Component already exists for the provided entity id");
    assert(id < GLOBAL_ENTITY_COUNTER && "The provided entity never existed");
    assert(myecs::has_component<component>(id) == false && "Component already exists for the provided entity id");
    myecs::create_component(myecs::storage<component>::pointer); // from "generated_components_create.hpp"
    myecs::storage<component>::size++;
    entity_component_linker<component>(id, myecs::storage<component>::size - 1);

  }

  template <typename component>
  void remove_comp_from(const entity& id) {
    assert(id < GLOBAL_ENTITY_COUNTER && "The provided entity never existed");
    //assert((id < myecs::storage<component>::sparse.size() && myecs::storage<component>::sparse[id] != INVALID_INDEX) && "Component does not exist for the provided entity id");
    assert(myecs::has_component<component>(id) && "Component does not exist for the provided entity id");
    myecs::d_array<std::size_t>& sparse = myecs::storage<component>::sparse;
    myecs::d_array<std::size_t>& reverse_sparse = myecs::storage<component>::reverse_sparse;

    std::size_t component_remove_index = sparse[id];
    std::size_t component_last_index_bd = myecs::storage<component>::size - 1; //_bd = before deletion
    std::size_t entity_id_at_last_component_bd = reverse_sparse[component_last_index_bd]; //_bd = before deletion
    std::size_t entity_id_at_component_remove_index = reverse_sparse[component_remove_index];

    if (component_remove_index != component_last_index_bd) {
      // deleting component at component_remove_index
      myecs::delete_component(myecs::storage<component>::pointer, component_remove_index); // from "generated_components_delete.hpp"

      // updating maps of deleted components
      sparse[id] = INVALID_INDEX;
      reverse_sparse[component_last_index_bd] = INVALID_INDEX; // not updating reverse_sparse[component_remove_index] because at the place of that removed old component, last component will be copied

      // updating maps of shifted components 
      sparse[entity_id_at_last_component_bd] = component_remove_index;
      reverse_sparse[component_remove_index] = entity_id_at_last_component_bd;
    }
    else {
      myecs::delete_component(myecs::storage<component>::pointer, component_remove_index); // from "generated_components_delete.hpp"
      sparse[id] = INVALID_INDEX;
      reverse_sparse[component_last_index_bd] = INVALID_INDEX;
    }
    myecs::storage<component>::size--;
  }


  template <typename component>
  inline const std::size_t& comp_index_of(const entity& id) {
    assert(id < GLOBAL_ENTITY_COUNTER && "The provided entity never existed");
    assert(myecs::has_component<component>(id) && "Entity does not have the component");
    return myecs::storage<component>::sparse[id];
  }

  template <typename component>
  inline const std::size_t& entity_index_of(const std::size_t& comp_index) {
    assert(comp_index < myecs::storage<component>::reverse_sparse.size() && "Component index out of bounds");
    assert(myecs::has_entity<component>(comp_index) && "Component index does not have an entity");
    return myecs::storage<component>::reverse_sparse[comp_index];
  }
}


#define comp_pointer(component_type_only) myecs::storage<component_type_only>::pointer
#define comp_index_of_en(entity, component_type) myecs::comp_index_of<component_type>(entity)
#define ecs_access(component_type, entity_id, element) myecs::storage<component_type>::pointer->element[myecs::comp_index_of<component_type>(entity_id)]
#define access_fill(component_type, entity_id, element, value) myecs::storage<component_type>::pointer->element[myecs::comp_index_of<component_type>(entity_id)] = value
