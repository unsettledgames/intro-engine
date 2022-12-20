#pragma once

#include <Debut/Core/Core.h>
#include <Debut/Rendering/Structures/FrameBuffer.h>
#include <Debut/Rendering/Shader.h>

#include <unordered_map>

namespace Debut
{
	class FrameBuffer;
	class VertexBuffer;
	class IndexBuffer;
	class VertexArray;

	class Shader;
	class PostProcessingStack;

	enum class RenderTextureMode {Color = 0, Depth};

	class RenderTexture
	{
	public:
		static Ref<RenderTexture> Create(float width, float height, Ref<FrameBuffer> buffer, RenderTextureMode mode);
		RenderTexture(FrameBufferSpecifications bufferSpecs);
		~RenderTexture() = default;

		void Draw(Ref<Shader> shader, std::unordered_map<std::string, 
			ShaderUniform>& properties = std::unordered_map<std::string, ShaderUniform>());
		void Draw(Ref<FrameBuffer> startBuffer, Ref<Shader> startShader, Ref<PostProcessingStack> postProcessing);

		virtual void BindTexture() = 0;
		virtual void UnbindTexture() = 0;

		void Begin(Ref<FrameBuffer> originalScene, Ref<Shader> startShader);
		void End();

		inline Ref<FrameBuffer> GetTopFrameBuffer() { return m_PrevBuffer; }
		inline void SetFrameBuffer(Ref<FrameBuffer> buffer) { m_FrameBuffer = buffer; }

		inline Ref<FrameBuffer> GetFrameBuffer() { return m_FrameBuffer; }
		inline uint32_t GetRendererID() { return m_RendererID; }
		inline float GetWidth() { return m_Width; }
		inline float GetHeight() { return m_Height; }

	protected:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Ref<VertexArray> m_VertexArray;
		Ref<FrameBuffer> m_FrameBuffer;

		uint32_t m_RendererID;

		float m_Width;
		float m_Height;
		RenderTextureMode m_Mode;
		Ref<RenderTexture> m_Target;

		Ref<FrameBuffer> m_PrevBuffer;
		Ref<FrameBuffer> m_NextBuffer;
	};
}