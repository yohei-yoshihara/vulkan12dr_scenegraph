#ifndef __MESH_BOX_HPP__
#define __MESH_BOX_HPP__

#include "common.hpp"
#include "types.hpp"
#include "mesh.hpp"

#include <array>

class Box {
public:
  static std::shared_ptr<Mesh> generate(const glm::vec3 &halfExtents, int horizontalSegments,
                       int verticalSegments);
  static std::shared_ptr<Mesh> generate(const glm::vec3 &halfExtents, int horizontalSegments,
                       int verticalSegments, const glm::vec3 &color);
  static std::shared_ptr<Mesh> generate(const glm::vec3 &halfExtents, int horizontalSegments,
                       int verticalSegments,
                       const std::array<glm::vec3, 6> &colors);
};

#endif /* defined(__b3__b3Box__) */
