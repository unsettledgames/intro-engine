#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Debut
{
	namespace MathUtils
	{
		std::vector<float> ComputeGaussianKernel(int size, float sigma, float mu);

		bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
		glm::mat4 CreateTransform(const glm::vec3& trans, const glm::vec3& rot, const glm::vec3& scale);
		
		std::vector<uint32_t> Triangulate(const std::vector<glm::vec2>& vertices);

		inline float Lerp(float a, float b, float t) { return a + t * (b - a); }
	}
}