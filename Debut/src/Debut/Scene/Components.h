#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <Debut/Utils/MathUtils.h>
#include "Debut/Scene/SceneCamera.h"
#include <Debut/Scene/ScriptableEntity.h>

#include <Debut/Core/UUID.h>

/*
	OPTIMIZABLE:
		- Runtime physics material in Colliders?
*/

/*
	I want to add a component! What do I do?
	- Add a struct in this file, containing the necessary data
	- Add the entry in the AddComponent menu
		- Implement OnComponentAdded<ComponentType>
		- Implement CopyComponent in Scene.cpp for that component
	- Implement component rendering in the Inspector
	- Add (de)serialization in SceneSerializer
		- Implement DeserializeComponent and SerializeComponent for that component
		- Add calls to them in SerializeText / DeserializeText
	- Implement actual behaviour
*/

namespace Debut
{
	enum class ColliderType { Circle2D = 0, Box2D, Polygon, None, Box, Sphere, Mesh };
	struct Collider2DComponent
	{
		ColliderType Type;
	};
	
	struct LightComponent
	{
		enum class LightType {Directional = 0, Point, Area, Shape};
		LightType Type;
	};

	// CORE
	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;
		std::string Name;

		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& name) : Tag("Untagged"), Name(name) {}
		TagComponent(const std::string& name, const std::string& tag) : Tag(tag), Name(name) {}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::vec3 Rotation = glm::vec3(0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);

		Entity Parent = {};

		TransformComponent() = default;
		TransformComponent(const TransformComponent& other) = default;
		TransformComponent(const glm::vec3& translation) : Translation(translation) {}

		glm::mat4 GetTransform()
		{
			// Get the parent matrix
			glm::mat4 worldMatrix;
			if (!Parent)
				worldMatrix = glm::mat4(1.0);
			else
				worldMatrix = Parent.Transform().GetTransform();

			// Compose the local matrix
			glm::mat4 transform(1.0f);
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
			
			// Apply the parent matrix
			return worldMatrix * (glm::translate(transform, Translation) 
				* rotation 
				* glm::scale(glm::mat4(1.0f), Scale));
		}

		glm::mat4 GetLocalTransform()
		{
			// Compose the local matrix
			glm::mat4 transform(1.0f);
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			// Apply the parent matrix
			return (glm::translate(transform, Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale));
		}

		void SetParent(Entity parent) 
		{ 
			glm::mat4 finalTransform = GetLocalTransform();
			glm::quat rotation;
			glm::vec3 skew;
			glm::vec4 persp;

			if (Parent)
				finalTransform = Parent.Transform().GetTransform() * finalTransform;
			if (parent)
				finalTransform = glm::inverse(parent.Transform().GetTransform()) * finalTransform;

			glm::decompose(finalTransform, Scale, rotation, Translation, skew, persp);
			Rotation = glm::eulerAngles(rotation);

			Parent = parent; 
		}
	};

	// GEOMETRY

	struct CameraComponent
	{
		Debut::SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = glm::vec4(1.0f);
		UUID Texture = 0;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() : Color(glm::vec4(1.0f)) {}
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color) : Color(color) {}
	};

	struct MeshRendererComponent
	{
		UUID Material = 0;
		UUID Mesh = 0;

		bool Instanced = false;

		MeshRendererComponent()  {}
		MeshRendererComponent(const MeshRendererComponent&) = default;
		MeshRendererComponent(UUID mesh, UUID material) : Material(material), Mesh(mesh), Instanced(false) {}
	};

	// LIGHTING
	struct DirectionalLightComponent : LightComponent
	{
		glm::vec3 Direction = glm::vec3(1.0f);
		glm::vec3 Color = glm::vec3(1.0f);
		float Intensity = 1.0f;

		DirectionalLightComponent() { Type = LightType::Directional; }
		DirectionalLightComponent(const DirectionalLightComponent&) = default;
	};

	struct PointLightComponent : LightComponent
	{
		glm::vec3 Color = glm::vec3(1.0f);
		glm::vec3 Position = glm::vec3(0.0f);

		float Intensity = 1.0f;
		float Radius = 5.0f;

		PointLightComponent() { Type = LightType::Point; }
		PointLightComponent(const PointLightComponent&) = default;
	};

	// PHYSICS AND COLLIDERS
	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };

		BodyType Type = BodyType::Static;
		// TODO: constraints
		bool FixedRotation = false;

		void* RuntimeBody = nullptr;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;

		static BodyType StrToRigidbody2DType(const std::string& type)
		{
			if (type == "Static") return BodyType::Static;
			if (type == "Dynamic") return BodyType::Dynamic;
			if (type == "Kinematic") return BodyType::Kinematic;

			return BodyType::Dynamic;
		}
	};

	struct BoxCollider2DComponent : Collider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 1.0f, 1.0f };

		UUID Material = 0;

		void* RuntimeFixture = nullptr;

		void SetPoint(glm::vec2& point, std::string& type)
		{
			glm::vec2 diff;
			if (type == "TopLeft")
			{
				// Get the current top left 
				glm::vec2 current = glm::vec2(-Size.x / 2.0 + Offset.x, Size.y / 2.0 + Offset.y);
				diff = point - current;
				// Half the difference goes in offset
				Offset += diff / 2.0f;
				// The difference goes in size
				Size.x += -diff.x;
				Size.y += diff.y;
			}
			else if (type == "TopRight")
			{
				glm::vec2 current = glm::vec2(Size.x / 2.0 + Offset.x, Size.y / 2.0 + Offset.y);
				diff = point - current;
				Offset += diff / 2.0f;
				Size.x += diff.x;
				Size.y += diff.y;
			}
			else if (type == "BottomLeft")
			{
				glm::vec2 current = glm::vec2(-Size.x / 2.0 + Offset.x, -Size.y / 2.0 + Offset.y);
				diff = point - current;
				Offset += diff / 2.0f;
				Size.x += -diff.x;
				Size.y += -diff.y;
			}
			else if (type == "BottomRight")
			{
				glm::vec2 current = glm::vec2(Size.x / 2.0 + Offset.x, -Size.y / 2.0 + Offset.y);
				diff = point - current;
				Offset += diff / 2.0f;
				Size.x += diff.x;
				Size.y += -diff.y;
			}
		}

		BoxCollider2DComponent() { Type = ColliderType::Box2D; };
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	struct CircleCollider2DComponent : Collider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 1.0f;

		UUID Material = 0;

		void* RuntimeFixture = nullptr;

		void SetPoint(glm::vec2& point, std::string& type)
		{
			glm::vec2 diff;
			if (type == "Left")
			{
				// Get the current left 
				glm::vec2 current = glm::vec2(-Radius + Offset.x, 0.0f + Offset.y);
				diff = point - current;
				// Half the difference goes in offset
				Offset.x += diff.x / 2.0f;
				// The difference goes in size
				Radius += -diff.x / 2.0f;
			}
			else if (type == "Top")
			{
				glm::vec2 current = glm::vec2(0.0f + Offset.x, Radius + Offset.y);
				diff = point - current;
				Offset.y += diff.y / 2.0f;
				Radius += diff.y / 2.0f;
			}
			else if (type == "Right")
			{
				glm::vec2 current = glm::vec2(Radius + Offset.x, 0.0f + Offset.y);
				diff = point - current;
				Offset.x += diff.x / 2.0f;
				Radius += diff.x / 2.0f;
			}
			else if (type == "Bottom")
			{
				glm::vec2 current = glm::vec2(0.0f, -Radius + Offset.y);
				diff = point - current;
				Offset.y += diff.y / 2.0f;
				Radius += -diff.y / 2.0f;
			}
		}

		CircleCollider2DComponent() { Type = ColliderType::Circle2D; };
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	struct PolygonCollider2DComponent : Collider2DComponent
	{
		UUID Material = 0;
		void* RuntimeFixture = nullptr;
		
		glm::vec2 Offset = glm::vec2(0.0f);
		std::vector<glm::vec2> Points;
		std::vector<uint32_t> Indices;

		void SetPoint(int index, glm::vec2 value)
		{
			// Update point
			Points[index] = value;
			// Add point
			// Keep anti-clockwise
			// Triangulate
		}

		std::vector<std::vector<glm::vec2>> GetTriangles()
		{
			std::vector<std::vector<glm::vec2>> ret;
			ret.resize(Indices.size() / 3);

			for (uint32_t i = 0; i < Indices.size(); i += 3)
				ret[i / 3] = { Points[Indices[i]], Points[Indices[i + 1]], Points[Indices[i + 2]]};
			
			return ret;
		}

		PolygonCollider2DComponent() 
		{ 
			Type = ColliderType::Polygon;
			Points = { {-0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, -0.5f}, {-0.5f, -0.5f} };
			Indices = MathUtils::Triangulate(Points);
		}
		PolygonCollider2DComponent(const PolygonCollider2DComponent&) = default;
	};

	// SCRIPT
	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()	
		{
			InstantiateScript = []() {return static_cast<ScriptableEntity*>(new T()); };	   
			DestroyScript = [](NativeScriptComponent* nsc) {delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};
}