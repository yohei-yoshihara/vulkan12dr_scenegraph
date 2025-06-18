#ifndef __MESH_PLANE__
#define __MESH_PLANE__

#include "common.hpp"
#include "mesh.hpp"
#include "types.hpp"

#include <array>

class Plane {
public:
  static std::shared_ptr<Mesh> generate(float width, float height, UpAxis up,
                                        int widthSegments, int heightSegments);
  static std::shared_ptr<Mesh> generate(float width, float height, UpAxis up,
                                        int widthSegments, int heightSegments,
                                        const glm::vec3 &color);
};

#endif
