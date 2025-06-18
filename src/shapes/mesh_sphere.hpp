#ifndef __MESH_SPHERE_HPP__
#define __MESH_SPHERE_HPP__

#include "common.hpp"
#include "mesh.hpp"
#include "types.hpp"

#include <array>

class Sphere {
public:
  static std::shared_ptr<Mesh> generate(float radius, int longitudinalSegments,
                                        int latitudinalSegments);
  static std::shared_ptr<Mesh> generate(float radius, int longitudinalSegments,
                                        int latitudinalSegments,
                                        const glm::vec3 &color);
};

#endif
