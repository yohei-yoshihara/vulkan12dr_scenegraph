#ifndef __MESH_HPP__
#define __MESH_HPP__

#include "common.hpp"
#include "types.hpp"

class Mesh {
  std::vector<Vertex> m_vertices;
  std::vector<IndexType> m_indices;

public:
  IndexType addVertex(const Vertex &vertex);
  void addIndex(IndexType index);

  void setColor(const glm::vec3 &color);

  const std::vector<Vertex> vertices() const { return m_vertices; }
  const std::vector<IndexType> indices() const { return m_indices; }
  Vertex &vertex(size_t i) { return m_vertices[i]; }
  const Vertex &vertex(size_t i) const { return m_vertices[i]; }
  IndexType index(size_t i) const { return m_indices[i]; }
  size_t size() const { return m_vertices.size(); }
  size_t numberOfIndices() const { return m_indices.size(); }
};

#endif