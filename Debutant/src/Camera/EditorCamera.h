#pragma once
#include <Debut/Scene/SceneCamera.h>

#include <utility>

namespace Debut
{
	class Event;
	class MouseScrolledEvent;
	class Timestep;

	class EditorCamera : public SceneCamera
	{
	public:
		EditorCamera() : SceneCamera() {}
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);
		~EditorCamera() = default;

		void OnUpdate(Timestep& ts);
		void OnEvent(Event& e);

		inline void SetDistance(float distance) { m_Distance = distance; UpdateView();}
		inline void SetFocalPoint(const glm::vec3& focalPoint) { m_FocalPoint = focalPoint; UpdateView(); }
		inline void SetPosition(const glm::vec3& position) { m_Position = position; UpdateView(); }
		inline void SetPitch(const float pitch) { m_Pitch = pitch; UpdateView(); }
		inline void SetYaw(const float yaw) { m_Yaw = yaw; UpdateView(); }

		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

		inline float GetDistance() const { return m_Distance; }
		inline glm::vec3 GetFocalPoint() const { return m_FocalPoint; }
	private:
		void UpdateProjection();
		void UpdateView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);
		void WasdMove(const glm::vec2& mousePos);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};
}

