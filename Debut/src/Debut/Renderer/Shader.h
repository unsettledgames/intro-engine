#pragma once

#include <Debut/Core/Core.h>
#include <glm/glm.hpp>

namespace Debut
{
	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual const std::string& GetName() const = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		
		virtual void SetFloat3(const std::string& name, const glm::vec3& uniform) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& uniform) = 0;

		virtual void SetMat4(const std::string& name, const glm::mat4& uniform) = 0;

		static Ref<Shader> Create(const std::string& name, const std::string& vertSrc, const std::string& fragSrc);
		static Ref<Shader> Create(const std::string& filePath);

	protected:
		unsigned int m_ProgramID;
	};

	class ShaderLibrary
	{
	public:
		void Add(const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader, const std::string& name);

		Ref<Shader> Load(const std::string& filePath);
		Ref<Shader> Load(const std::string& name, const std::string& filePath);

		Ref<Shader> Get(const std::string& name);
		bool Exists(const std::string& name) const;

	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};
}