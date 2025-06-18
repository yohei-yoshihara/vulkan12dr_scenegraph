#ifndef __MESH_CONE_HPP__
#define __MESH_CONE_HPP__

#include "common.hpp"
#include "types.hpp"
#include "mesh.hpp"

#include <array>

class Cone {
public:
  static std::shared_ptr<Mesh> generate(float height, float topRadius, float bottomRadius,
                       UpAxis up, int radialSegments, int verticalSegments,
                       bool openEnded);
  static std::shared_ptr<Mesh> generate(float height, float topRadius, float bottomRadius,
                       UpAxis up, int radialSegments, int verticalSegments,
                       bool openEnded, const glm::vec3 &color);
};

#endif
