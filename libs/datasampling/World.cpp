///
/// @file    World.cpp
/// @author  Barthelemy von Haller
///

#include "../datasampling/World.h"
#include <iostream>

namespace AliceO2 {
namespace Hello {

void World::greet()
{
  std::cout << "Hello world!!" << std::endl;
}

int World::returnsN(int n)
{

  /// \todo This is how you can markup a todo in your code that will show up in the documentation of your project.
  /// \bug This is how you annotate a bug in your code.
  return n;
}

} // namespace Hello
} // namespace AliceO2
