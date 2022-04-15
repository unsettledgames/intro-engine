#pragma once

#include "Debut/Core/Time.h"
#include "Debut/Scene/SceneCamera.h"
#include <glm/glm.hpp>
#include <Debut/Scene/ScriptableEntity.h>

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
		glm::mat4 Transform = glm::mat4(1.0f);

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::mat4& mat) : Transform(mat) {}

		operator const glm::mat4& () const { return Transform; }
		operator glm::mat4& () { return Transform; }
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

		SpriteRendererComponent() : Color(glm::vec4(1.0f)) {}
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color) : Color(color) {}
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		std::function<void()> InstantiateFunction;
		std::function<void()> DestroyFunction;

		std::function<void(ScriptableEntity* instance)> OnCreateFunction;
		std::function<void(ScriptableEntity* instance)> OnDestroyFunction;
		std::function<void(ScriptableEntity* instance, Timestep ts) > OnUpdateFunction;


		template<typename T>
		void Bind()
		{
			InstantiateFunction = [&]() {Instance = new T(); };
			DestroyFunction = [&]() {delete (T*)Instance; };

			OnCreateFunction = [](ScriptableEntity* Instance) { ((T*)Instance)->OnCreate(); };
			OnDestroyFunction = [](ScriptableEntity* Instance) { ((T*)Instance)->OnDestroy(); };
			OnUpdateFunction = [](ScriptableEntity* Instance, Timestep ts) { ((T*)Instance)->OnUpdate(ts); };
		}
	};
}