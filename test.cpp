#include <iostream>
#include <type_traits>
#include <functional>
#include <cstddef>

namespace TEST {
  
}

int main()
{
  auto temp = []( int a, int b ) { return a + b; };
  auto f = TEST::make_fntor_wrapper( temp );
  std::cout << f.to_fnptr<int, int>()(1, 2) << std::endl;
  return 0;
}
