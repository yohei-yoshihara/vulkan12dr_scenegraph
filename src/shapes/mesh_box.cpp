#include "mesh_box.hpp"
#include "main.hpp"

static int buildPlane(int startingIndice, float width, float height,
                      float translateX, float translateY, float translateZ,
                      float normalX, float normalY, float normalZ,
                      const glm::vec3 &color, bool clockwise,
                      int horizontalSegments, int verticalSegments,
                      Mesh &mesh) {
  for (int i = 0; i <= horizontalSegments; ++i) {
    float x = -(width * 0.5f) + i * (width / horizontalSegments);

    for (int j = 0; j <= verticalSegments; ++j) {
      float y = -(height * 0.5f) + j * (height / verticalSegments);

      Vertex vertex;
      if (normalZ != 0.0f) {
        vertex.position.x = translateX + x;
        vertex.position.y = translateY + y;
        vertex.position.z = translateZ;
      } else if (normalY != 0.0f) {
        vertex.position.x = translateX + y;
        vertex.position.y = translateY;
        vertex.position.z = translateZ + x;
      } else {
        vertex.position.x = translateX;
        vertex.position.y = translateY + x;
        vertex.position.z = translateZ + y;
      }

      vertex.normal.x = normalX;
      vertex.normal.y = normalY;
      vertex.normal.z = normalZ;

      vertex.color = color;

      mesh.addVertex(vertex);
    }
  }

  for (int i = 0; i < horizontalSegments; ++i) {
    for (int j = 0; j < verticalSegments; ++j) {

      int first = startingIndice + j * (horizontalSegments + 1) + i;
      int second = first + (verticalSegments + 1);
      int third = first + 1;
      int fourth = second + 1;

      if (clockwise == true) {
        mesh.addIndex(first);
        mesh.addIndex(third);
        mesh.addIndex(second);

        mesh.addIndex(third);
        mesh.addIndex(fourth);
        mesh.addIndex(second);
      } else {
        mesh.addIndex(first);
        mesh.addIndex(second);
        mesh.addIndex(third);

        mesh.addIndex(third);
        mesh.addIndex(second);
        mesh.addIndex(fourth);
      }
    }
  }

  return startingIndice + (horizontalSegments + 1) * (verticalSegments + 1);
}

std::shared_ptr<Mesh> Box::generate(const glm::vec3 &halfExtents,
                                    int horizontalSegments,
                                    int verticalSegments) {
  float width = halfExtents.x * 2.0f;
  float height = halfExtents.y * 2.0f;
  void addIndex(int index);

  float depth = halfExtents.z * 2.0f;
  auto mesh = std::make_shared<Mesh>();
  int indice = 0;
  glm::vec3 color(1, 1, 1);
  // Top
  indice = buildPlane(indice, width, height, 0.0f, 0.0f, depth * 0.5f, 0.0f,
                      0.0f, 1.0f, color, false, horizontalSegments,
                      verticalSegments, *mesh);
  // Bottom
  indice = buildPlane(indice, width, height, 0.0f, 0.0f, -depth * 0.5f, 0.0f,
                      0.0f, -1.0f, color, true, horizontalSegments,
                      verticalSegments, *mesh);
  // Right
  indice = buildPlane(indice, width, height, width * 0.5f, 0.0f, 0.0f, 1.0f,
                      0.0f, 0.0f, color, false, horizontalSegments,
                      verticalSegments, *mesh);
  // Left
  indice = buildPlane(indice, width, height, -width * 0.5f, 0.0f, 0.0f, -1.0f,
                      0.0f, 0.0f, color, true, horizontalSegments,
                      verticalSegments, *mesh);
  // Front
  indice = buildPlane(indice, width, height, 0.0f, height * 0.5f, 0.0f, 0.0f,
                      1.0f, 0.0f, color, false, horizontalSegments,
                      verticalSegments, *mesh);
  // Rear
  indice = buildPlane(indice, width, height, 0.0f, -height * 0.5f, 0.0f, 0.0f,
                      -1.0f, 0.0f, color, true, horizontalSegments,
                      verticalSegments, *mesh);
  return mesh;
}

std::shared_ptr<Mesh> Box::generate(const glm::vec3 &halfExtents,
                                    int horizontalSegments,
                                    int verticalSegments,
                                    const glm::vec3 &color) {
  float width = halfExtents.x * 2.0f;
  float height = halfExtents.y * 2.0f;
  float depth = halfExtents.z * 2.0f;
  auto mesh = std::make_shared<Mesh>();
  int indice = 0;
  // Top
  indice = buildPlane(indice, width, height, 0.0f, 0.0f, depth * 0.5f, 0.0f,
                      0.0f, 1.0f, color, false, horizontalSegments,
                      verticalSegments, *mesh);
  // Bottom
  indice = buildPlane(indice, width, height, 0.0f, 0.0f, -depth * 0.5f, 0.0f,
                      0.0f, -1.0f, color, true, horizontalSegments,
                      verticalSegments, *mesh);
  // Right
  indice = buildPlane(indice, width, height, width * 0.5f, 0.0f, 0.0f, 1.0f,
                      0.0f, 0.0f, color, false, horizontalSegments,
                      verticalSegments, *mesh);
  // Left
  indice = buildPlane(indice, width, height, -width * 0.5f, 0.0f, 0.0f, -1.0f,
                      0.0f, 0.0f, color, true, horizontalSegments,
                      verticalSegments, *mesh);
  // Front
  indice = buildPlane(indice, width, height, 0.0f, height * 0.5f, 0.0f, 0.0f,
                      1.0f, 0.0f, color, false, horizontalSegments,
                      verticalSegments, *mesh);
  // Rear
  indice = buildPlane(indice, width, height, 0.0f, -height * 0.5f, 0.0f, 0.0f,
                      -1.0f, 0.0f, color, true, horizontalSegments,
                      verticalSegments, *mesh);
  return mesh;
}

std::shared_ptr<Mesh> Box::generate(const glm::vec3 &halfExtents,
                                    int horizontalSegments,
                                    int verticalSegments,
                                    const std::array<glm::vec3, 6> &colors) {
  float width = halfExtents.x * 2.0f;
  float height = halfExtents.y * 2.0f;
  float depth = halfExtents.z * 2.0f;
  auto mesh = std::make_shared<Mesh>();
  int indice = 0;
  // Top
  indice = buildPlane(indice, width, height, 0.0f, 0.0f, depth * 0.5f, 0.0f,
                      0.0f, 1.0f, colors[0], false, horizontalSegments,
                      verticalSegments, *mesh);
  // Bottom
  indice = buildPlane(indice, width, height, 0.0f, 0.0f, -depth * 0.5f, 0.0f,
                      0.0f, -1.0f, colors[1], true, horizontalSegments,
                      verticalSegments, *mesh);
  // Right
  indice = buildPlane(indice, width, height, width * 0.5f, 0.0f, 0.0f, 1.0f,
                      0.0f, 0.0f, colors[2], false, horizontalSegments,
                      verticalSegments, *mesh);
  // Left
  indice = buildPlane(indice, width, height, -width * 0.5f, 0.0f, 0.0f, -1.0f,
                      0.0f, 0.0f, colors[3], true, horizontalSegments,
                      verticalSegments, *mesh);
  // Front
  indice = buildPlane(indice, width, height, 0.0f, height * 0.5f, 0.0f, 0.0f,
                      1.0f, 0.0f, colors[4], false, horizontalSegments,
                      verticalSegments, *mesh);
  // Rear
  indice = buildPlane(indice, width, height, 0.0f, -height * 0.5f, 0.0f, 0.0f,
                      -1.0f, 0.0f, colors[5], true, horizontalSegments,
                      verticalSegments, *mesh);
  return mesh;
}
