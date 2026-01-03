#pragma once

namespace myutils{
  
  // is_same compile time type == check

  template <typename T1, typename T2>
  struct is_same_type {
    enum { value = 0 };
  };

  template <typename T>
  struct is_same_type<T, T> {
    enum { value = 1 };
  };

  // end is_same

  template <typename T>
  struct is_unsigned_number_type {
    enum { 
      value = (
        // only unsigned number types
        myutils::is_same_type<T, unsigned short>::value ||
        myutils::is_same_type<T, unsigned int>::value ||
        myutils::is_same_type<T, unsigned long>::value ||
        myutils::is_same_type<T, unsigned long long>::value
        )
    }; // could have implemented varaiadic templates here, but now there is no need for it and destroying redablity
  };

}