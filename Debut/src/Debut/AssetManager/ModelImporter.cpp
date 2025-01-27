#include <Debut/dbtpch.h>
#include <cstdio>

#include <Debut/AssetManager/ModelImporter.h>
#include <Debut/AssetManager/AssetManager.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <Debut/Rendering/Renderer/Renderer3D.h>
#include <Debut/ImGui/ProgressPanel.h>
#include <Debut/Utils/CppUtils.h>

/*
	TODO
		- Save model import settings in .model.meta file
*/

namespace Debut
{
	static inline glm::mat4 AiMatrixoGlm(const aiMatrix4x4* from)
	{
		glm::mat4 to;


		to[0][0] = from->a1; to[0][1] = from->b1;  to[0][2] = from->c1; to[0][3] = from->d1;
		to[1][0] = from->a2; to[1][1] = from->b2;  to[1][2] = from->c2; to[1][3] = from->d2;
		to[2][0] = from->a3; to[2][1] = from->b3;  to[2][2] = from->c3; to[2][3] = from->d3;
		to[3][0] = from->a4; to[3][1] = from->b4;  to[3][2] = from->c4; to[3][3] = from->d4;

		return to;
	}

	Ref<Model> ModelImporter::ImportModel(const std::string& path, const ModelImportSettings& settings)
	{
		ProgressPanel::SubmitTask("modelimport", "Importing model...");
		std::filesystem::path inputFolder(path);
		inputFolder = inputFolder.parent_path();
		std::string modelPath = path.substr(0, path.find_last_of("\\") + 1);
		modelPath += settings.ImportedName;

		std::ifstream meta(modelPath + ".model.meta");
		unsigned int pFlags = aiProcess_RemoveRedundantMaterials;

		if (meta.good())
		{
			meta.close();
			Ref<Model> model = AssetManager::Request<Model>(modelPath + ".model");
			std::vector<UUID> associationsToDelete;
			RemoveNodes(model, associationsToDelete);
			AssetManager::DeleteAssociations(associationsToDelete);
		}
		
		Assimp::Importer importer;

		if (settings.ImproveRenderingSpeed)
			pFlags |= aiProcess_ImproveCacheLocality;
		if (settings.JoinVertices)
			pFlags |= aiProcess_JoinIdenticalVertices;
		if (settings.Triangulate)
			pFlags |= aiProcess_Triangulate;
		if (settings.Normals)
			pFlags |= aiProcess_GenNormals;
		if (settings.TangentSpace)
			pFlags |= aiProcess_CalcTangentSpace;
		if (settings.OptimizeMeshes)
			pFlags |= aiProcess_OptimizeMeshes;
		if (settings.OptimizeScene)
			pFlags |= aiProcess_OptimizeGraph;

		const aiScene* scene = importer.ReadFile(path, pFlags);

		if (scene != nullptr)
		{
			// Import the model
			aiNode* rootNode = scene->mRootNode;
			Ref<Model> ret = ImportNodes(rootNode, scene, path.substr(0, path.find_last_of("\\")), settings.ImportedName, inputFolder.string());
			ret->SetPath(modelPath + ".model");

			ProgressPanel::CompleteTask("modelimport");
			meta.close();

			return ret;
		}
		else
		{
			Log.CoreError("Error while importing model {0}: {1}", path, importer.GetErrorString());
			return nullptr;
		}

		return nullptr;
	}

	Ref<Model> ModelImporter::ImportNodes(aiNode* parent, const aiScene* scene, const std::string& saveFolder, const std::string& modelName, const std::string& inputFolder)
	{
		DBT_PROFILE_FUNCTION();
		// Don't import empty models
		if (parent->mNumMeshes == 0 && parent->mNumChildren == 0)
			return nullptr;

		// Save the root model in the folder of the asset, save the generated assets in Lib
		std::string submodelsFolder = saveFolder;
		std::string assetsFolder = AssetManager::s_AssetsDir;
		if (parent->mParent != nullptr)
			submodelsFolder = AssetManager::s_AssetsDir;

		std::vector<UUID> models;
		std::vector<UUID> meshes;
		std::vector<UUID> materials;

		meshes.resize(parent->mNumMeshes);
		materials.resize(parent->mNumMeshes);
		models.resize(parent->mNumChildren);

		// Load dependencies
		for (int i = 0; i < parent->mNumChildren; i++)
		{
			ProgressPanel::ProgressTask("modelimport", i / parent->mNumChildren);
			Ref<Model> currModel = ImportNodes(parent->mChildren[i], scene, submodelsFolder, modelName, inputFolder);
			if (currModel != nullptr)
				models[i] = currModel->GetID();
			else
				models[i] = 0;
		}
		models.erase(std::remove(models.begin(), models.end(), 0), models.end());

		// Load meshes and materials
		for (int i = 0; i < parent->mNumMeshes; i++)
		{
			// Import and submit the mesh
			aiMesh* assimpMesh = scene->mMeshes[parent->mMeshes[i]];

			{
				DBT_PROFILE_SCOPE("ModelImporter::ImportMesh");
				Ref<Mesh> mesh = ModelImporter::ImportMesh(assimpMesh, "Mesh" + i, assetsFolder, AiMatrixoGlm(&(parent->mTransformation)));
				AssetManager::Submit(mesh);
				if (mesh != nullptr)
					meshes[i] = mesh->GetID();
			}
			
			{
				DBT_PROFILE_SCOPE("ModelImporter::ImportMaterial");
				// Import and submit the material
				aiMaterial* assimpMaterial = scene->mMaterials[assimpMesh->mMaterialIndex];
				Ref<Material> material = ModelImporter::ImportMaterial(assimpMaterial, "Material" + i, assetsFolder, inputFolder);
				AssetManager::Submit(material);
				if (material != nullptr)
					materials[i] = material->GetID();
			}

			
		}
		meshes.erase(std::remove(meshes.begin(), meshes.end(), 0), meshes.end());
		materials.erase(std::remove(materials.begin(), materials.end(), 0), materials.end());

		Ref<Model> ret = CreateRef<Model>(meshes, materials, models);
		std::stringstream ss;
		std::string name;
		if (parent->mParent == nullptr)
			name = modelName;
		else
			name = CppUtils::FileSystem::CorrectFileName(parent->mName.C_Str());
		ret->SetPath(submodelsFolder + "\\" + name + ".model");
		ret->SaveSettings();
		AssetManager::Submit(ret);

		return ret;
	}

	Ref<Mesh> ModelImporter::ImportMesh(aiMesh* assimpMesh, const std::string& name, const std::string& saveFolder, glm::mat4& transform)
	{
		ProgressPanel::SubmitTask("meshimport", "Importing mesh...");

		// If the mesh already exists, don't import it
		Ref<Mesh> mesh = CreateRef<Mesh>();
		if (mesh->IsValid())
			return mesh;

		// Otherwise import it as usual
		std::vector<float> positions, normals, tangents, bitangents, colors;
		std::vector<std::vector<float>> texcoords;
		std::vector<int> indices;

		mesh->m_Transform = transform;
		positions.resize(assimpMesh->mNumVertices * 3);
		mesh->SetName(assimpMesh->mName.C_Str());

		{
			DBT_PROFILE_SCOPE("ImportMesh::Vertices");
			// Create mesh, start with the positions of the vertices
			for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
				for (uint32_t j = 0; j < 3; j++)
				{
					uint32_t index = i * 3 + j;
					positions[index] = assimpMesh->mVertices[i][j];
				}
			ProgressPanel::ProgressTask("meshimport", 0.17);
		}

		{
			DBT_PROFILE_SCOPE("ImportMesh::Colors");
			// Vertex colors
			if (assimpMesh->GetNumColorChannels() > 0)
			{
				colors.resize(assimpMesh->mNumVertices * 4);
				for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
				{
					for (uint32_t j = 0; j < 4; j++)
					{
						uint32_t index = i * 4 + j;
						colors[index] = assimpMesh->mColors[0][i][j];
					}
				}
			}
		}
		
		{
			DBT_PROFILE_SCOPE("ImportMesh::Normals");
			// Load normals
			if (assimpMesh->HasNormals())
			{
				normals.resize(assimpMesh->mNumVertices * 3);
				for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
				{
					for (uint32_t j = 0; j < 3; j++)
					{
						uint32_t index = i * 3 + j;
						normals[index] = assimpMesh->mNormals[i][j];
					}
				}
			}
			ProgressPanel::ProgressTask("meshimport", 0.17);
		}

		{
			DBT_PROFILE_SCOPE("ImportMesh::TangentSpace");
			// Load tangents / bitangents
			if (assimpMesh->HasTangentsAndBitangents())
			{
				tangents.resize(assimpMesh->mNumVertices * 3);
				bitangents.resize(assimpMesh->mNumVertices * 3);

				for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
				{
					for (uint32_t j = 0; j < 3; j++)
					{
						uint32_t index = i * 3 + j;
						tangents[index] = assimpMesh->mTangents[i][j];
					}
				}
				ProgressPanel::ProgressTask("meshimport", 0.17);

				for (uint32_t i = 0; i < assimpMesh->mNumVertices; i++)
				{
					for (uint32_t j = 0; j < 3; j++)
					{
						uint32_t index = i * 3 + j;
						bitangents[index] = assimpMesh->mBitangents[i][j];
					}
				}
				ProgressPanel::ProgressTask("meshimport", 0.17);
			}
		}

		{
			DBT_PROFILE_SCOPE("ImportMesh::TexCoords");
			// Load texture coordinates
			texcoords.resize(assimpMesh->GetNumUVChannels());
			for (uint32_t i = 0; i < assimpMesh->GetNumUVChannels(); i++)
			{
				texcoords[i].resize(assimpMesh->mNumVertices * 3);

				for (uint32_t j = 0; j < assimpMesh->mNumVertices; j++)
					for (uint32_t k = 0; k < 2; k++)
					{
						uint32_t index = j * 2 + k;
						texcoords[i][index] = assimpMesh->mTextureCoords[i][j][k];
					}
			}
			ProgressPanel::ProgressTask("meshimport", 0.17);
		}

		{
			DBT_PROFILE_SCOPE("ImportMesh::Indices");
			// Load indices
			indices.resize(assimpMesh->mNumFaces * 3);
			uint32_t indexIndex = 0;
			for (uint32_t i = 0; i < assimpMesh->mNumFaces; i++)
			{
				for (uint32_t j = 0; j < assimpMesh->mFaces[i].mNumIndices; j++)
				{
					indices[indexIndex] = assimpMesh->mFaces[i].mIndices[j];
					indexIndex++;
				}
			}
			ProgressPanel::ProgressTask("meshimport", 0.17);
		}

		{
			DBT_PROFILE_SCOPE("ImportMesh::SaveFiles");
			// Save the mesh on disk + meta file
			std::stringstream ss;
			ss << saveFolder << mesh->GetID();
			mesh->SetPath(ss.str());
			mesh->SetName(assimpMesh->mName.C_Str());
			mesh->SetPositions(positions);
			mesh->SetIndices(indices);
			mesh->SaveSettings(positions, colors, normals, tangents, bitangents, texcoords, indices);
			mesh->Load(ss.str());

			ProgressPanel::CompleteTask("meshimport");
		}

		return mesh;
	}

	Ref<Material> ModelImporter::ImportMaterial(aiMaterial* assimpMaterial, const std::string& name, const std::string& saveFolder, const std::string& inputFolder)
	{
		Ref<Material> material = CreateRef<Material>();
		if (material->IsValid())
			return material;

		// Otherwise import the material as usual
		material->SetShader(AssetManager::Request<Shader>("assets\\shaders\\default-3d.glsl", "assets\\shaders\\default-3d.glsl.meta"));
		material->SetName(assimpMaterial->GetName().C_Str());

		// Store / add properties
		aiMaterialProperty** properties = assimpMaterial->mProperties;

		// Colors
		aiColor3D color;
		assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material->SetVec3("u_DiffuseColor", { color.r, color.g, color.b });

		Log.CoreInfo("Diffuse color: {0},{1},{2}", color.r, color.g, color.b);

		assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
		material->SetVec4("u_AmbientColor", { color.r, color.g, color.b, 1.0 });

		Log.CoreInfo("Ambient color: {0},{1},{2}", color.r, color.g, color.b);

		// Load textures
		aiString texturePath;
		aiTextureType types[] = { aiTextureType_DIFFUSE, aiTextureType_NORMALS, aiTextureType_AMBIENT, 
			aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_DISPLACEMENT, aiTextureType_METALNESS, 
			aiTextureType_REFLECTION, aiTextureType_SPECULAR, aiTextureType_EMISSIVE};
		std::string uniformNames[] = { "u_DiffuseTexture", "u_NormalMap", "u_AmbientMap", "u_RoughnessMap", "u_DisplacementMap",
			"u_MetalnessTexture", "u_ReflectionMap", "u_SpecularMap", "u_EmissionMap" };

		for (uint32_t i = 0; i < 9; i++)
		{
			if (assimpMaterial->GetTextureCount(types[i]))
			{
				aiString path;
				std::string fullPath;
				
				assimpMaterial->Get(AI_MATKEY_TEXTURE(types[i], 0), path);
				if (!std::filesystem::exists(path.C_Str()))
					fullPath = std::filesystem::path(inputFolder).parent_path().string() + "\\textures\\" + path.C_Str();

				material->SetTexture(uniformNames[i], AssetManager::Request<Texture2D>(fullPath));
			}
		}

		// Save the material on disk + meta file
		// Save the mesh on disk + meta file
		std::stringstream ss;
		ss << saveFolder << material->GetID();
		material->SetPath(ss.str());
		ss.str("");

		ss << AssetManager::s_MetadataDir << material->GetID() << ".meta";
		material->SetMetaPath(ss.str());
		material->SaveSettings();

		return material;
	}

	void ModelImporter::RemoveNodes(Ref<Model> model, std::vector<UUID>& associations)
	{
		CppUtils::FileSystem::RemoveFile((model->GetPath() + ".meta").c_str());
		CppUtils::FileSystem::RemoveFile((model->GetPath()).c_str());
		AssetManager::Remove<Model>(model->GetID());
		associations.push_back(model->GetID());

		std::stringstream ss;
		for (uint32_t i = 0; i < model->GetMeshes().size(); i++)
		{
			ss.str("");
			ss << AssetManager::s_AssetsDir << model->GetMeshes()[i];
			CppUtils::FileSystem::RemoveFile(ss.str().c_str());
			associations.push_back(model->GetMeshes()[i]);
			AssetManager::Remove<Mesh>(model->GetMeshes()[i]);

			ss.str("");
			ss << AssetManager::s_MetadataDir << model->GetMeshes()[i] << ".meta";
			CppUtils::FileSystem::RemoveFile(ss.str().c_str());
		}

		for (uint32_t i = 0; i < model->GetMaterials().size(); i++)
		{
			ss.str("");
			ss << AssetManager::s_AssetsDir << model->GetMaterials()[i];
			CppUtils::FileSystem::RemoveFile(ss.str().c_str());
			associations.push_back(model->GetMaterials()[i]);
			AssetManager::Remove<Material>(model->GetMaterials()[i]);

			ss.str("");
			ss << AssetManager::s_MetadataDir << model->GetMaterials()[i] << ".meta";
			CppUtils::FileSystem::RemoveFile(ss.str().c_str());
		}

		for (uint32_t i = 0; i < model->GetSubmodels().size(); i++)
			RemoveNodes(AssetManager::Request<Model>(model->GetSubmodels()[i]), associations);
	}
}