#define NOMINMAX
#include "aabb.h"



AABB::AABB()
{
	reset();
}

AABB::AABB(const glm::vec3 &min, const glm::vec3 &max) :
    min{min},
    max{max}
{
}


void AABB::update(const glm::vec3 &point)
{
	min = glm::min(min, point);
	max = glm::max(max, point);
}

void AABB::update(const std::vector<glm::vec3> &vertex_data, const std::vector<uint32_t> &index_data)
{
	// Check if submesh is indexed
	if (index_data.size() > 0)
	{
		// Update bounding box for each indexed vertex
		for (size_t index_id = 0; index_id < index_data.size(); index_id++)
		{
			update(vertex_data[index_data[index_id]]);
		}
	}
	else
	{
		// Update bounding box for each vertex
		for (size_t vertex_id = 0; vertex_id < vertex_data.size(); vertex_id++)
		{
			update(vertex_data[vertex_id]);
		}
	}
}


void AABB::update(const glm::vec3* vertex_data, size_t nVertices, const uint32_t* index_data, size_t nIndices)
{
    // Check if submesh is indexed
    if (nIndices > 0)
    {
        // Update bounding box for each indexed vertex
        for (size_t index_id = 0; index_id < nIndices; index_id++)
        {
            update(vertex_data[index_data[index_id]]);
        }
    }
    else
    {
        // Update bounding box for each vertex
        for (size_t vertex_id = 0; vertex_id < nVertices; vertex_id++)
        {
            update(vertex_data[vertex_id]);
        }
    }
}

void AABB::transform(glm::mat4 &transform)
{

  glm::vec3 right = glm::vec3(transform[0][0], transform[1][0], transform[2][0]);
  glm::vec3 up = glm::vec3(transform[0][1], transform[1][1], transform[2][1]);
  glm::vec3 forward = glm::vec3(transform[0][2], transform[1][2], transform[2][2]);
  glm::vec3 translation = glm::vec3(transform[3]);

  glm::vec3 xa = right * min.x;
  glm::vec3 xb = right * max.x;

  glm::vec3 ya = up * min.y;
  glm::vec3 yb = up * max.y;

  glm::vec3 za = -forward * min.z;
  glm::vec3 zb = -forward * max.z;
  min = glm::min(xa, xb) + glm::min(ya, yb) + glm::min(za, zb) + translation;
  max = glm::max(xa, xb) + glm::max(ya, yb) + glm::max(za, zb) + translation;

}

glm::vec3 AABB::get_scale() const
{
	return (max - min);
}

glm::vec3 AABB::get_center() const
{
	return (min + max) * 0.5f;
}

glm::vec3 AABB::get_min() const
{
	return min;
}

glm::vec3 AABB::get_max() const
{
	return max;
}

void AABB::reset()
{
	min = glm::vec3(std::numeric_limits<float>::max());

	max = glm::vec3(std::numeric_limits<float>::min());
}

bool AABB::pointInside(const glm::vec3& point) const
{

    //Check if the point is less than max and greater than min
    if (point.x > min.x&& point.x < max.x &&
        point.y > min.y&& point.y < max.y &&
        point.z > min.z&& point.z < max.z)
    {
        return true;
    }

    //If not, then return false
    return false;

}
