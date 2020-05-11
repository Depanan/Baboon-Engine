

#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <vector>
#include "Renderer/Common/GLMInclude.h"
#include "../Renderer/Common/Mesh.h"

class AABB
{
  public:
	AABB();

	AABB(const glm::vec3 &min, const glm::vec3 &max);

	/**
	 * @brief Update the bounding box based on the given vertex position
	 * @param point The 3D position of a point
	 */
	void update(const glm::vec3 &point);

	/**
	 * @brief Update the bounding box based on the given submesh vertices
	 * @param vertex_data The position vertex data
	 * @param index_data The index vertex data
	 */
	void update(const std::vector<glm::vec3> &vertex_data, const std::vector<uint32_t> &index_data);

  void update(const glm::vec3* vertex_data, size_t nVertices, const uint32_t* index_data, size_t nIndices);

	/**
	 * @brief Apply a given matrix transformation to the bounding box
	 * @param transform The matrix transform to apply
	 */
	void transform(glm::mat4 &transform);

	/**
	 * @brief Scale vector of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_scale() const;

	/**
	 * @brief Center position of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_center() const;

	/**
	 * @brief Minimum position of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_min() const;

	/**
	 * @brief Maximum position of the bounding box
	 * @return vector in 3D space
	 */
	glm::vec3 get_max() const;

	/**
	 * @brief Resets the min and max position coordinates
	 */
	void reset();

  bool pointInside(const glm::vec3& point) const;
  


  private:
	glm::vec3 min;

	glm::vec3 max;
};
