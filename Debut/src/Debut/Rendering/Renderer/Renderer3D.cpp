#include <Debut/dbtpch.h>


#include <Debut/Rendering/Texture.h>
#include <Debut/Rendering/Camera.h>

#include <Debut/Rendering/Material.h>
#include <Debut/Rendering/Resources/Skybox.h>
#include <Debut/Rendering/Structures/VertexArray.h>
#include <Debut/Rendering/Structures/Buffer.h>
#include <Debut/Rendering/Resources/Mesh.h>
#include <Debut/Scene/Components.h>

#include <Debut/AssetManager/AssetManager.h>
#include <Debut/Rendering/Renderer/Renderer3D.h>
#include <Debut/Core/Instrumentor.h>
#include <Debut/Rendering/Renderer/RenderCommand.h>


namespace Debut
{
	Renderer3DStorage Renderer3D::s_Data;

	void Renderer3D::Init()
	{
		DBT_PROFILE_FUNCTION();

		// Reserve space for the textures
		s_Data.Textures.resize(s_Data.MaxTextures);
		// Create generic VertexArray and IndexBuffer
		s_Data.VertexArray = VertexArray::Create();
		s_Data.IndexBuffer = IndexBuffer::Create();

		// Attach buffers to VertexArray
		ShaderDataType types[] = { ShaderDataType::Float3, ShaderDataType::Float4, ShaderDataType::Float3, ShaderDataType::Float3, ShaderDataType::Float3, ShaderDataType::Float2 };
		std::string attribNames[] = { "a_Position", "a_Color", "a_Normal", "a_Tangent", "a_Bitangent", "a_TexCoords0" };
		std::string names[] = { "Positions", "Colors", "Normals", "Tangents", "Bitangents", "TexCoords0" };
		
		for (uint32_t i = 0; i < sizeof(types); i++)
		{
			s_Data.VertexBuffers[names[i]] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
			s_Data.VertexBuffers[names[i]]->SetLayout({ {types[i], attribNames[i], false} });
			s_Data.VertexArray->AddVertexBuffer(s_Data.VertexBuffers[names[i]]);
		}
		s_Data.VertexArray->AddIndexBuffer(s_Data.IndexBuffer);
	}

	void Renderer3D::BeginScene(Camera& camera, Ref<Skybox> skybox, glm::mat4& cameraTransform, std::vector<LightComponent*>& lights,
		std::vector<ShaderUniform>& globalUniforms)
	{
		s_Data.CameraView = glm::inverse(cameraTransform);
		s_Data.CameraProjection = camera.GetProjection();
		s_Data.CameraTransform = s_Data.CameraProjection * s_Data.CameraView;
		
		s_Data.Lights = lights;
		s_Data.GlobalUniforms = globalUniforms;

		// Draw the skybox
		if (skybox != nullptr)
		{
			Ref<Material> skyboxMaterial = AssetManager::Request<Material>(skybox->GetMaterial());
			glm::mat4 skyboxTransform;
			if (camera.GetProjectionType() == Camera::ProjectionType::Perspective)
				skyboxTransform = camera.GetProjection() * glm::inverse(glm::mat4(glm::mat3(cameraTransform)));
			else
				skyboxTransform = glm::perspective(45.0f, 16.0f/9.0f, camera.GetNearPlane(), camera.GetFarPlane())
					* glm::inverse(glm::mat4(glm::mat3(cameraTransform)));
			
			skybox->Bind();
			skyboxMaterial->SetMat4("u_ViewProjection", skyboxTransform);
			skyboxMaterial->Use();

			std::vector<float>& positions = skybox->GetMesh().GetPositions();
			std::vector<int>& indices = skybox->GetMesh().GetIndices();
			s_Data.VertexBuffers["Positions"]->SetData(positions.data(), positions.size() * sizeof(float));
			s_Data.IndexBuffer->SetData(indices.data(), indices.size());

			RenderCommand::DrawIndexed(s_Data.VertexArray, indices.size());

			skybox->Unbind();
			skyboxMaterial->Unuse();
		}
	}

	void Renderer3D::DrawModel(const MeshRendererComponent& meshComponent, const glm::mat4& transform)
	{
		DBT_PROFILE_FUNCTION();

		Ref<Mesh> mesh;
		Ref<Material> material;

		{
			DBT_PROFILE_SCOPE("Renderer3D::GetMesh");
			mesh = AssetManager::Request<Mesh>(meshComponent.Mesh);
		}

		{
			DBT_PROFILE_SCOPE("Renderer3D::GetMaterial");
			material = AssetManager::Request<Material>(meshComponent.Material);
		}

		if (mesh == nullptr)
		{
			Log.CoreError("Couldn't find mesh to render. Did you reimport and overwrite the model?");
			return;
		}
		if (material == nullptr)
		{
			Log.CoreError("Couldn't find material to render with. Did you reimport and overwrite the model?");
			return;
		}

		DrawModel(*mesh.get(), *material.get(), transform, meshComponent.Instanced);
	}

	void Renderer3D::DrawModel(Mesh& mesh, Material& material, const glm::mat4& transform, bool instanced /* = false*/)
	{
		DBT_PROFILE_FUNCTION();

		// Instanced rendering if the MeshRenderer is instanced
		if (instanced)
		{
			if (s_Data.Batches.find(material.GetID()) == s_Data.Batches.end())
				AddBatch(material.GetID());
			RenderBatch3D* currBatch = s_Data.Batches[material.GetID()];

			// Send data to buffers
			{
				DBT_PROFILE_SCOPE("Renderer3D::PushData");
				currBatch->Buffers["Positions"]->PushData(mesh.GetPositions().data(), sizeof(float) * mesh.GetPositions().size());

				if (mesh.HasColors())
					currBatch->Buffers["Colors"]->PushData(mesh.GetColors().data(), sizeof(float) * mesh.GetColors().size());
				if (mesh.HasNormals())
					currBatch->Buffers["Normals"]->PushData(mesh.GetNormals().data(), sizeof(float) * mesh.GetNormals().size());
				if (mesh.HasTangents())
					currBatch->Buffers["Tangents"]->PushData(mesh.GetTangents().data(), sizeof(float) * mesh.GetTangents().size());
				if (mesh.HasBitangents())
					currBatch->Buffers["Bitangents"]->PushData(mesh.GetBitangents().data(), sizeof(float) * mesh.GetBitangents().size());

				// Add indices
				size_t currIndicesSize = currBatch->Indices.size();
				currBatch->Indices.resize(currBatch->Indices.size() + mesh.GetIndices().size());
				memcpy(currBatch->Indices.data() + currIndicesSize, mesh.GetIndices().data(), mesh.GetIndices().size() * sizeof(int));
			}
		}
		// Just draw the model otherwise
		else
		{
			{
				DBT_PROFILE_SCOPE("DrawModel::SendGeometry");
				std::vector<float>& positions = mesh.GetPositions();
				std::vector<int>& indices = mesh.GetIndices();
				s_Data.VertexBuffers["Positions"]->SetData(positions.data(), positions.size() * sizeof(float));
				s_Data.IndexBuffer->SetData(mesh.GetIndices().data(), mesh.GetIndices().size());

				if (mesh.HasColors())
				{
					std::vector<float>& colors = mesh.GetColors();
					s_Data.VertexBuffers["Colors"]->SetData(colors.data(), colors.size() * sizeof(float));
				}

				if (mesh.HasNormals())
				{
					std::vector<float>& normals = mesh.GetNormals();
					s_Data.VertexBuffers["Normals"]->SetData(normals.data(), normals.size() * sizeof(float));
				}

				if (mesh.HasTangents())
				{
					std::vector<float>& tangents = mesh.GetTangents();
					s_Data.VertexBuffers["Tangents"]->SetData(tangents.data(), tangents.size() * sizeof(float));
				}

				if (mesh.HasBitangents())
				{
					std::vector<float>& bitangents = mesh.GetBitangents();
					s_Data.VertexBuffers["Bitangents"]->SetData(bitangents.data(), bitangents.size() * sizeof(float));
				}

				if (mesh.HasTexCoords(0))
				{
					std::vector<float>& texCoords = mesh.GetTexCoords(0);
					s_Data.VertexBuffers["TexCoords0"]->SetData(texCoords.data(), texCoords.size() * sizeof(float));
				}
			}

			{
				DBT_PROFILE_SCOPE("DrawModel::UseMaterial");
				SendLights(material);
				SendGlobals(material);
				material.SetMat4("u_Transform", transform * mesh.GetTransform());
				material.SetMat4("u_ViewMatrix", s_Data.CameraView);
				material.SetMat4("u_ProjectionMatrix", s_Data.CameraProjection);
				material.SetMat4("u_ViewProjection", s_Data.CameraProjection * s_Data.CameraView);
				material.Use();
			}

			{
				DBT_PROFILE_SCOPE("DrawModel::DrawIndexed");
				RenderCommand::DrawIndexed(s_Data.VertexArray, mesh.GetIndices().size());
			}
		}
	}

	void Renderer3D::EndScene()
	{
		Flush();
	}

	void Renderer3D::Flush()
	{
		DBT_PROFILE_FUNCTION();

		for (auto& batch : s_Data.Batches)
		{
			batch.second->Material->SetMat4("u_ViewMatrix", s_Data.CameraView);
			batch.second->Material->SetMat4("u_ProjectionMatrix", s_Data.CameraProjection);
			batch.second->Material->SetMat4("u_ViewProjection", s_Data.CameraProjection * s_Data.CameraView);
			batch.second->Material->Use();

			// Setup buffers
			for (auto& buffer : batch.second->Buffers)
				buffer.second->SubmitData();
			batch.second->IndexBuffer->SetData(batch.second->Indices.data(), batch.second->Indices.size());

			// Issue draw call
			RenderCommand::DrawIndexed(batch.second->VertexArray, batch.second->Indices.size());
			// Clear buffers
			batch.second->Indices.clear();
		}
	}

	void Renderer3D::Shutdown()
	{
		// Delete all batches buffers
	}

	void Renderer3D::SendLights(Material& material)
	{
		std::vector<PointLightComponent> pointLights;

		for (LightComponent* light : s_Data.Lights)
		{
			switch (light->Type)
			{
			case LightComponent::LightType::Directional:
			{
				DirectionalLightComponent* dirLight = static_cast<DirectionalLightComponent*>(light);
				ShaderUniform::UniformData data;

				data.Vec3 = dirLight->Direction;
				material.m_Uniforms["u_DirectionalLightDir"] = {
					ShaderUniform("u_DirectionalLightDir", ShaderDataType::Float3, data) };

				data.Vec3 = dirLight->Color;
				material.m_Uniforms["u_DirectionalLightCol"] = {
					ShaderUniform("u_DirectionalLightCol", ShaderDataType::Float3, data) };

				data.Float = dirLight->Intensity;
				material.m_Uniforms["u_DirectionalLightIntensity"] = {
					ShaderUniform("u_DirectionalLightIntensity", ShaderDataType::Float, data) };
				break;
			}
			case LightComponent::LightType::Point:
			{
				PointLightComponent* pointLight = static_cast<PointLightComponent*>(light);
				pointLights.push_back(*pointLight);
				break;
			}
			}
		}

		// Send point lights
		for (uint32_t i = 0; i < pointLights.size(); i++)
		{
			std::stringstream lightName;
			ShaderUniform::UniformData data;
			lightName << "u_PointLights[" << i << "]";

			data.Vec3 = pointLights[i].Color;
			material.m_Uniforms[lightName.str() + ".Color"] = { ShaderUniform(lightName.str() + ".Color", ShaderDataType::Float3, data) };
			data.Vec3 = pointLights[i].Position;
			material.m_Uniforms[lightName.str() + ".Position"] = { ShaderUniform(lightName.str() + ".Position", ShaderDataType::Float3, data) };

			data.Float = pointLights[i].Intensity;
			material.m_Uniforms[lightName.str() + ".Intensity"] = { ShaderUniform(lightName.str() + ".Intensity", ShaderDataType::Float, data) };
			data.Float = pointLights[i].Radius;
			material.m_Uniforms[lightName.str() + ".Radius"] = { ShaderUniform(lightName.str() + ".Radius", ShaderDataType::Float, data) };
		}
	}

	void Renderer3D::SendGlobals(Material& material)
	{
		for (auto& uniform : s_Data.GlobalUniforms)
			material.m_Uniforms[uniform.Name] = uniform;
	}

	void Renderer3D::AddBatch(const UUID& id)
	{
		DBT_PROFILE_FUNCTION();

		if (s_Data.Batches.size() == s_Data.MaxBatches)
		{
			Log.CoreFatal("Renderer3D batch amount exceeded {0}", s_Data.MaxBatches);
			return;
		}

		RenderBatch3D* newBatch = new RenderBatch3D();

		// Add material
		newBatch->Material = AssetManager::Request<Material>(id);

		// Create and configure vertex array
		newBatch->VertexArray = VertexArray::Create();
		newBatch->IndexBuffer = IndexBuffer::Create();
		newBatch->VertexArray->AddIndexBuffer(newBatch->IndexBuffer);

		// Setup buffers
		newBatch->Buffers["Position"] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
		newBatch->Buffers["Position"]->SetLayout({ { ShaderDataType::Float3, "a_Position", false } });
		newBatch->VertexArray->AddVertexBuffer(newBatch->Buffers["Position"]);

		newBatch->Buffers["Colors"] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
		newBatch->Buffers["Colors"]->SetLayout({ { ShaderDataType::Float4, "a_Color", false } });
		newBatch->VertexArray->AddVertexBuffer(newBatch->Buffers["Colors"]);

		newBatch->Buffers["Normals"] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
		newBatch->Buffers["Normals"]->SetLayout({ { ShaderDataType::Float3, "a_Normal", false } });
		newBatch->VertexArray->AddVertexBuffer(newBatch->Buffers["Normals"]);

		newBatch->Buffers["Tangents"] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
		newBatch->Buffers["Tangents"]->SetLayout({ { ShaderDataType::Float3, "a_Tangent", false } });
		newBatch->VertexArray->AddVertexBuffer(newBatch->Buffers["Tangents"]);

		newBatch->Buffers["Bitangents"] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
		newBatch->Buffers["Bitangents"]->SetLayout({ { ShaderDataType::Float3, "a_Bitangent", false } });
		newBatch->VertexArray->AddVertexBuffer(newBatch->Buffers["Bitangents"]);

		newBatch->Buffers["TexCoords0"] = VertexBuffer::Create(s_Data.StartupBufferSize * sizeof(float), s_Data.MaxVerticesPerBatch * sizeof(float));
		newBatch->Buffers["TexCoords0"]->SetLayout({ { ShaderDataType::Float2, "a_TexCoords0", false } });
		newBatch->VertexArray->AddVertexBuffer(newBatch->Buffers["TexCoords0"]);
		
		// Add batch
		s_Data.Batches[newBatch->Material->GetID()] = newBatch;
	}
}