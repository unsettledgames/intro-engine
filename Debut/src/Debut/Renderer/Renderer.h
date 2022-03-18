#pragma once

#include "RendererAPI.h"
#include "Shader.h"
#include "OrthographicCamera.h"

namespace Debut
{
	class Renderer
	{
	public:
		static void Init();
		static void BeginScene(OrthographicCamera& camera/*Cameras, lights*/);
		static void Submit(const Ref<VertexArray>& va, const Ref<Shader>& shader,
			const glm::mat4 transform = glm::mat4(1.0f));
		static void EndScene();

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		inline static void SetApi(RendererAPI::API val) { RendererAPI::SetAPI(val); }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};
}