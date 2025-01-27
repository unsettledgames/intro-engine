#pragma once

#include <glm/glm.hpp>

namespace Debut
{
	class Camera
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };

		Camera() = default;
		virtual ~Camera() {}

		inline glm::mat4 GetProjection() const { return m_ProjectionMatrix; }
		inline glm::mat4 GetView() const { return m_ViewMatrix; }
		inline glm::mat4 GetViewProjection() { return m_ProjectionMatrix * m_ViewMatrix; }
		inline ProjectionType GetProjectionType() const { return m_ProjectionType; }
		inline float GetNearPlane() const { return m_NearPlane; }
		inline float GetFarPlane() const { return m_FarPlane; }
		inline float GetAspectRatio() const { return m_AspectRatio; }

		inline void SetView(const glm::mat4& view) { m_ViewMatrix = view; }
		inline void SetProjection(const glm::mat4& proj) { m_ProjectionMatrix = proj; }
		inline void SetNearPlane(float val) { m_NearPlane = val; }
		inline void SetFarPlane(float val) { m_FarPlane = val; }
		inline void SetAspectRatio(float val) { m_AspectRatio = val; }
		inline void SetType(const ProjectionType& type) { m_ProjectionType = type; }

	protected:
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		ProjectionType m_ProjectionType;

		float m_NearPlane = 0.1f;
		float m_FarPlane = 1000.0f;
		float m_AspectRatio;
	};
}