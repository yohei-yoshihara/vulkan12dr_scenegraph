#include "mesh_cone.hpp"
#include "main.hpp"
#include <cmath>
#include <numbers>

static void build(float height, float topRadius, float bottomRadius,
                  int radialSegments, UpAxis up, int verticalSegments,
                  bool openEnded, [[maybe_unused]] const glm::vec2 &uv,
                  Mesh &mesh) {
  // TODO:
  // 以下のアルゴリズムだと、上部と下部において、面積が0のTriangleができてしまう。
  // なので、頂点は追加するが、インデックスには追加しない頂点がある。
  // このため、頂点のチェックを有効にするととエラーになる。

  // float totalHeight;
  // if (openEnded) {
  //   totalHeight = height;
  // } else {
  //   totalHeight = bottomRadius + height + topRadius;
  // }

  float dR = bottomRadius - topRadius;
  float length = std::sqrt(height * height + dR * dR);

  if (!openEnded) {
    // Create bottom
    for (int s = 0; s <= radialSegments; ++s) {
      Vertex vertex;
      vertex.position.x = 0.0f;
      vertex.position.y = -(height / 2.0f);
      vertex.position.z = 0.0f;

      vertex.normal.x = 0.0f;
      vertex.normal.y = -1.0f;
      vertex.normal.z = 0.0f;

      mesh.addVertex(vertex);
    }
  }

  // float vOffset;
  // if (openEnded) {
  //   vOffset = 0;
  // } else {
  //   vOffset = bottomRadius;
  // }

  // Create middle
  for (int t = 0; t <= verticalSegments; ++t) {
    float radius =
        bottomRadius - (bottomRadius - topRadius) * t / verticalSegments;
    for (int s = 0; s <= radialSegments; ++s) {
      float theta = s * 2.0f * std::numbers::pi_v<float> / radialSegments;
      Vertex vertex;
      vertex.position.x = radius * std::sin(theta);
      vertex.position.y = -(height / 2.0f) + (t * height / verticalSegments);
      vertex.position.z = radius * std::cos(theta);

      vertex.normal.x = std::sin(theta) * height / length;
      vertex.normal.y = dR / length;
      vertex.normal.z = std::cos(theta) * height / length;

      mesh.addVertex(vertex);
    }
  }

  if (!openEnded) {
    // Create top
    for (int s = 0; s <= radialSegments; ++s) {
      Vertex vertex;
      vertex.position.x = 0.0f;
      vertex.position.y = height / 2.0f;
      vertex.position.z = 0.0f;

      vertex.normal.x = 0.0f;
      vertex.normal.y = 1.0f;
      vertex.normal.z = 0.0f;

      mesh.addVertex(vertex);
    }
  }

  int totalNT;
  if (openEnded) {
    totalNT = verticalSegments;
  } else {
    totalNT = verticalSegments + 2;
  }

  for (int t = 0; t < totalNT; ++t) {
    for (int s = 0; s < radialSegments; ++s) {

      int first = (t * (radialSegments + 1)) + s;
      int second = first + (radialSegments + 1);
      int third = first + 1;
      int fourth = second + 1;

      bool excludeFirstTriangle = false;
      bool excludeSecondTriangle = false;

      // 上部の場合は面積が0の三角形になるので除く
      if (t == 0) {
        excludeFirstTriangle = true;
      }
      // 柱部分で上部の半径が0で、且つ、上から2番目の位置の場合は、インデックスを除く
      if (t == (totalNT - 2) && topRadius == 0.0f) {
        excludeSecondTriangle = true;
      }
      // 下部の場合は面積が0の三角形になるので除く
      if (t == (totalNT - 1)) {
        excludeSecondTriangle = true;
      }
      // 柱部分で下部の半径が0で、且つ、下から2番目の位置の場合は、インデックスを除く
      if (t == 1 && bottomRadius == 0.0f) {
        excludeFirstTriangle = true;
      }

      if (!excludeFirstTriangle) {
        mesh.addIndex(first);
        mesh.addIndex(third);
        mesh.addIndex(second);
      }

      if (!excludeSecondTriangle) {
        mesh.addIndex(second);
        mesh.addIndex(third);
        mesh.addIndex(fourth);
      }
    }
  }

  // UpAxis対応
  if (up == UpAxis::X) {
    for (int i = 0; i < mesh.size(); ++i) {
      float x = mesh.vertex(i).position.x;
      float y = mesh.vertex(i).position.y;
      float nx = mesh.vertex(i).normal.x;
      float ny = mesh.vertex(i).normal.y;
      mesh.vertex(i).position.x = y;
      mesh.vertex(i).position.y = -x;
      mesh.vertex(i).normal.x = ny;
      mesh.vertex(i).normal.y = -nx;
    }
  } else if (up == UpAxis::Z) {
    for (int i = 0; i < mesh.size(); ++i) {
      float y = mesh.vertex(i).position.y;
      float z = mesh.vertex(i).position.z;
      float ny = mesh.vertex(i).normal.y;
      float nz = mesh.vertex(i).normal.z;
      mesh.vertex(i).position.y = -z;
      mesh.vertex(i).position.z = y;
      mesh.vertex(i).normal.y = -nz;
      mesh.vertex(i).normal.z = ny;
    }
  }
}

std::shared_ptr<Mesh> Cone::generate(float height, float topRadius,
                                     float bottomRadius, UpAxis up,
                                     int radialSegments, int verticalSegments,
                                     bool openEnded) {
  auto mesh = std::make_shared<Mesh>();
  build(height, topRadius, bottomRadius, radialSegments, up, verticalSegments,
        openEnded, {-1.0f, -1.0f}, *mesh);
  return mesh;
}

std::shared_ptr<Mesh> Cone::generate(float height, float topRadius,
                                     float bottomRadius, UpAxis up,
                                     int radialSegments, int verticalSegments,
                                     bool openEnded, const glm::vec3 &color) {
  auto mesh = std::make_shared<Mesh>();
  build(height, topRadius, bottomRadius, radialSegments, up, verticalSegments,
        openEnded, color, *mesh);
  return mesh;
}
