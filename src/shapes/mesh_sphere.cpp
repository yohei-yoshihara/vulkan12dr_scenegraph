#include "mesh_sphere.hpp"
#include "main.hpp"
#include <cmath>
#include <numbers>

static void build(float radius, [[maybe_unused]] const glm::vec2 &uv,
                  int longitudinalSegments, int latitudinalSegments,
                  Mesh &mesh) {
  // TODO:
  // 以下のアルゴリズムだと、上部と下部において、面積が0のTriangleができてしまう。
  // なので、頂点は追加するが、インデックスには追加しない頂点がある。
  // このため、頂点のチェックを有効にするととエラーになる。
  for (int latNumber = 0; latNumber <= latitudinalSegments;
       ++latNumber) { // 高さ方向
    for (int longNumber = 0; longNumber <= longitudinalSegments;
         ++longNumber) { // 円周方向
      float theta = latNumber * std::numbers::pi_v<float> / latitudinalSegments;
      float phi =
          longNumber * 2.0f * std::numbers::pi_v<float> / longitudinalSegments;

      float sinTheta = std::sin(theta);
      float sinPhi = std::sin(phi);
      float cosTheta = std::cos(theta);
      float cosPhi = std::cos(phi);

      float x = cosPhi * sinTheta;
      float y = cosTheta;
      float z = sinPhi * sinTheta;

      //      NSLog(@"lat = %d, long = %d, (%g, %g, %g)", latNumber, longNumber,
      //      x, y, z);
      Vertex vertex;
      vertex.position.x = radius * x;
      vertex.position.y = radius * y;
      vertex.position.z = radius * z;

      vertex.normal.x = x;
      vertex.normal.y = y;
      vertex.normal.z = z;

      mesh.addVertex(vertex);
    }
  }

  for (int latNumber = 0; latNumber < latitudinalSegments; ++latNumber) {
    for (int longNumber = 0; longNumber < longitudinalSegments; ++longNumber) {

      int first = (latNumber * (longitudinalSegments + 1)) + longNumber;
      int second = first + (longitudinalSegments + 1);
      int third = first + 1;
      int fourth = second + 1;

      // 上部以外の場合は、インデックスを追加する（上部の場合は面積が0の三角形になるので除く）
      if (latNumber != 0) {
        mesh.addIndex(first);
        mesh.addIndex(third);
        mesh.addIndex(second);
      }

      // 下部以外の場合は、インデックスを追加する（下部の場合は面積が0の三角形になるので除く）
      if (latNumber != (latitudinalSegments - 1)) {
        mesh.addIndex(second);
        mesh.addIndex(third);
        mesh.addIndex(fourth);
      }
    }
  }
}

std::shared_ptr<Mesh> Sphere::generate(float radius, int longitudinalSegments,
                                       int latitudinalSegments) {
  auto mesh = std::make_shared<Mesh>();
  build(radius, {-1.0f, -1.0f}, longitudinalSegments, latitudinalSegments,
        *mesh);
  return mesh;
}

std::shared_ptr<Mesh> Sphere::generate(float radius, int longitudinalSegments,
                                       int latitudinalSegments,
                                       const glm::vec3 &color) {
  auto mesh = std::make_shared<Mesh>();
  build(radius, color, longitudinalSegments, latitudinalSegments, *mesh);
  return mesh;
}
