#pragma once

#include "Debut/Core/Time.h"
#include "Debut/Scene/SceneCamera.h"
#include <glm/glm.hpp>
#include <Debut/Scene/ScriptableEntity.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <Debut/Renderer/Texture.h>
#include <Debut/Core/Core.h>

namespace Debut
{
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
		glm::vec3 Translation;
		glm::vec3 Rotation = glm::vec3(0,0,0);
		glm::vec3 Scale = glm::vec3(1.0f);

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation) : Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 transform(1.0f);
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(transform, Translation) 
				* rotation 
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

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
		glm::vec4 Color;
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() : Color(glm::vec4(1.0f)) {}
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color) : Color(color) {}
	};

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