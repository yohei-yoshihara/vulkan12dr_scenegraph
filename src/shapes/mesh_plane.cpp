#include "mesh_plane.hpp"
#include "main.hpp"

static void build(float width, float height,
                  const glm::vec2 &uv, UpAxis up,
                  int widthSegments, int heightSegments, Mesh &mesh) {
  for (int i = 0; i <= widthSegments; ++i) {
    float x = -(width / 2) + i * (width / widthSegments);

    for (int j = 0; j <= heightSegments; ++j) {
      float y = -(height / 2) + j * (height / heightSegments);

      Vertex vertex;
      vertex.position.x = x;
      vertex.position.y = y;
      vertex.position.z = 0.0f;

      vertex.normal.x = 0.0f;
      vertex.normal.y = 0.0f;
      vertex.normal.z = 1.0f;

      // vertex.texCoord.x =
      //     uv.x + static_cast<float>(i) / static_cast<float>(widthSegments);
      // vertex.texCoord.y =
      //     uv.y + static_cast<float>(j) / static_cast<float>(heightSegments);

      vertex.color = {-1.f, -1.f, -1.f};

      mesh.addVertex(vertex);
    }
  }

  for (int i = 0; i < widthSegments; ++i) {
    for (int j = 0; j < heightSegments; ++j) {

      int first = i * (heightSegments + 1) + j;
      int second = first + (heightSegments + 1);
      int third = first + 1;
      int fourth = second + 1;

      mesh.addIndex(first);
      mesh.addIndex(second);
      mesh.addIndex(third);

      mesh.addIndex(third);
      mesh.addIndex(second);
      mesh.addIndex(fourth);
    }
  }

  // UpAxis::Z以外の場合は向き先を変える
  if (up == UpAxis::X) {
    for (int i = 0; i < mesh.size(); ++i) {
      auto x = mesh.vertex(i).position.x;
      auto z = mesh.vertex(i).position.z;
      auto nx = mesh.vertex(i).normal.x;
      auto nz = mesh.vertex(i).normal.z;
      mesh.vertex(i).position.x = z;
      mesh.vertex(i).position.z = -x;
      mesh.vertex(i).normal.x = nz;
      mesh.vertex(i).normal.z = -nx;
    }
  } else if (up == UpAxis::Y) {
    for (int i = 0; i < mesh.size(); ++i) {
      auto y = mesh.vertex(i).position.y;
      auto z = mesh.vertex(i).position.z;
      auto ny = mesh.vertex(i).normal.y;
      auto nz = mesh.vertex(i).normal.z;
      mesh.vertex(i).position.y = z;
      mesh.vertex(i).position.z = -y;
      mesh.vertex(i).normal.y = nz;
      mesh.vertex(i).normal.z = -ny;
    }
  }
}

std::shared_ptr<Mesh> Plane::generate(float width, float height, UpAxis up, int widthSegments,
                     int heightSegments) {
  auto mesh = std::make_shared<Mesh>();
  build(width, height, {0.0f, 0.0f}, up, widthSegments, heightSegments, *mesh);
  return mesh;
}

std::shared_ptr<Mesh> Plane::generate(float width, float height, UpAxis up, int widthSegments,
                     int heightSegments, const glm::vec3 &color) {
  auto mesh = std::make_shared<Mesh>();
  build(width, height, {0.0f, 0.0f}, up, widthSegments, heightSegments, *mesh);
  mesh->setColor(color);
  return mesh;
}
