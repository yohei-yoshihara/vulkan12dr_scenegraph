#include "node.hpp"

glm::mat4 Node::localMatrix() const {
  glm::mat4 modelMatrix = glm::mat4_cast(m_quat);
  modelMatrix[3][0] = m_pos.x;
  modelMatrix[3][1] = m_pos.y;
  modelMatrix[3][2] = m_pos.z;
  modelMatrix[3][3] = 1.0f;
  return modelMatrix;
}

glm::mat4 Node::worldMatrix() const {
  if (std::shared_ptr<Node> r = m_parent.lock()) {
    return r->worldMatrix() * localMatrix();
  }
  return localMatrix();
}
