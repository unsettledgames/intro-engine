#include "Debut/dbtpch.h"
#include "MathUtils.h"
#include <earcut.hpp>
#include <array>

using Coord = float;
using N = uint32_t;
using Point = std::array<Coord, 2>;


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace Debut
{
	bool MathUtils::DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		DBT_PROFILE_SCOPE("Transform::Decompose");
		// From glm::decompose in matrix_decompose.inl

		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3];

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		rotation.y = asin(-Row[0][2]);
		if (cos(rotation.y) != 0) {
			rotation.x = atan2(Row[1][2], Row[2][2]);
			rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			rotation.x = atan2(-Row[2][0], Row[1][1]);
			rotation.z = 0;
		}


		return true;
	}

	std::vector<uint32_t> MathUtils::Triangulate(const std::vector<glm::vec2>& vertices)
	{
		std::vector<Point> input;
		input.resize(vertices.size());

		for (uint32_t i = 0; i < vertices.size(); i++)
			input[i] = { vertices[i].x, vertices[i].y };

		std::vector<std::vector<Point>> polygon;
		polygon.push_back(input);
		
		return mapbox::earcut<N>(polygon);
	}

	glm::mat4 MathUtils::CreateTransform(const glm::vec3& trans, const glm::vec3& rot, const glm::vec3& scale)
	{
		return glm::translate(glm::mat4(1.0f), trans)
			* (glm::mat4(glm::quat(rot))
			* glm::scale(glm::mat4(1.0f), scale));
	}

	std::vector<float> MathUtils::ComputeGaussianKernel(int size, float sigma, float mu)
	{
		std::vector<float> ret(size * 2 + 1);
		float sum = 0;

		for (uint32_t i = 0; i < size * 2 + 1; i++)
		{
			ret[i] = (1.0f / (sigma * std::sqrt(2.0f * glm::pi<float>()))) *
				glm::exp(-glm::pow(((i - (float)size) - mu), 2.0f) / (2.0f * glm::pow(sigma, 2)));
			sum += ret[i];
		}

		for (uint32_t i = 0; i < ret.size(); i++)
			ret[i] /= sum;

		return ret;
	}
}