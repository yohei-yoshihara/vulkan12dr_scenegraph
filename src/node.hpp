#ifndef __NODE_HPP__
#define __NODE_HPP__

#include "common.hpp"

#include <memory>

class Mesh;

class Node : public std::enable_shared_from_this<Node> {
  std::weak_ptr<Node> m_parent;
  glm::vec3 m_pos{0.0f, 0.0f, 0.0f};
  glm::quat m_quat{glm::vec3{0.0f, 0.0f, 0.0f}};
  std::shared_ptr<Mesh> m_mesh;

public:
  Node() = default;
  Node(const std::shared_ptr<Mesh> &mesh) : m_mesh(mesh) {}
  // position
  void setPosition(const glm::vec3 &pos) { m_pos = pos; }
  const glm::vec3 &position() const { return m_pos; }

  // quat
  void setQuat(const glm::quat &quat) { m_quat = quat; }
  const glm::quat &quat() const { return m_quat; }

  // euler angle
  void setEulerAngle(const glm::vec3 &angle) {
    m_quat = glm::quat(glm::vec3(angle.x, angle.y, angle.z));
  }
  glm::vec3 eulearAngle() const { return glm::eulerAngles(m_quat); }

  // mesh
  void setMesh(const std::shared_ptr<Mesh> &mesh) { m_mesh = mesh; }
  const std::shared_ptr<Mesh> &mesh() const { return m_mesh; }

  glm::mat4 localMatrix() const;
  glm::mat4 worldMatrix() const;
};

#endif