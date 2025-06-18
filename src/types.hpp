#ifndef __TYPES_HPP__
#define __TYPES_HPP__

#include "common.hpp"

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
};

using IndexType = uint32_t;

enum class UpAxis { X, Y, Z };

#endif