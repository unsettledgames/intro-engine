#include "Debut/dbtpch.h"
#include "SceneSerializer.h"
#include "Components.h"
#include "Debut/Utils/CppUtils.h"
#include <Debut/Utils/YamlUtils.h>
#include <Debut/Scene/Scene.h>
#include <Debut/AssetManager/AssetManager.h>
#include <Debut/Rendering/Resources/Skybox.h>
#include <yaml-cpp/yaml.h>

namespace Debut
{
	static std::string Rb2DTypeToString(Rigidbody2DComponent::BodyType type)
	{
		switch (type)
		{
		case Rigidbody2DComponent::BodyType::Static: return "Static";
		case Rigidbody2DComponent::BodyType::Dynamic: return "Dynamic";
		case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		default:
			DBT_CORE_ASSERT(false, "Unknown body type {0}", (int)type);
			return "";
		}
	}

	static std::string Rb3DTypeToString(Rigidbody3DComponent::BodyType type)
	{
		switch (type)
		{
		case Rigidbody3DComponent::BodyType::Static: return "Static";
		case Rigidbody3DComponent::BodyType::Dynamic: return "Dynamic";
		case Rigidbody3DComponent::BodyType::Kinematic: return "Kinematic";
		default:
			DBT_CORE_ASSERT(false, "Unknown body type {0}", (int)type);
			return "";
		}
	}
	

	template <typename T>
	static void SerializeComponent(Entity e, const std::string& name, YAML::Emitter& out)
	{
		if (!e.HasComponent<T>())
			return;
		T& component = e.GetComponent<T>();

		out << YAML::Key << name << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Owner" << YAML::Value << component.Owner;
		SerializeComponent(component, out);
		out << YAML::EndMap;
	}

	static void SerializeComponent(const IDComponent& i, YAML::Emitter& out)
	{
		out << YAML::Key << "ID" << YAML::Value << i.ID;
	}

	static void SerializeComponent(const TagComponent& t, YAML::Emitter& out)
	{
		out << YAML::Key << "Name" << YAML::Value << t.Name;
		out << YAML::Key << "Tag" << YAML::Value << t.Tag;
	}

	static void SerializeComponent(TransformComponent& t, YAML::Emitter& out)
	{
		out << YAML::Key << "Translation" << YAML::Value << t.Translation;
		out << YAML::Key << "Rotation" << YAML::Value << t.Rotation;
		out << YAML::Key << "Scale" << YAML::Value << t.Scale;
		out << YAML::Key << "Parent" << YAML::Value << (!t.Parent ? 0 : (uint64_t)t.Parent.GetComponent<IDComponent>().ID);
		out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
		
		for (uint64_t child : t.Children)
			out << child;

		out << YAML::EndSeq;
	}

	static void SerializeComponent(const CameraComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "FixedAspectRatio" << YAML::Value << c.FixedAspectRatio;
		out << YAML::Key << "Primary" << YAML::Value << c.Primary;
		out << YAML::Key << "PostProcessingStack" << YAML::Value << c.PostProcessing;

		out << YAML::Key << "CameraData" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "ProjectionType" << YAML::Value << (int)c.Camera.GetProjectionType();
		out << YAML::Key << "OrthoNear" << YAML::Value << c.Camera.GetNearPlane();
		out << YAML::Key << "OrthoFar" << YAML::Value << c.Camera.GetFarPlane();
		out << YAML::Key << "PerspNear" << YAML::Value << c.Camera.GetNearPlane();
		out << YAML::Key << "PerspFar" << YAML::Value << c.Camera.GetFarPlane();
		out << YAML::Key << "OrthoSize" << YAML::Value << c.Camera.GetOrthoSize();
		out << YAML::Key << "PerspFOV" << YAML::Value << c.Camera.GetFOV();

		out << YAML::EndMap;
	}

	static void SerializeComponent(const SpriteRendererComponent& s, YAML::Emitter& out)
	{
		out << YAML::Key << "Color" << YAML::Value << s.Color;
		out << YAML::Key << "Texture" << YAML::Value << s.Texture;
		out << YAML::Key << "TilingFactor" << YAML::Value << s.TilingFactor;
	}

	static void SerializeComponent(const MeshRendererComponent& s, YAML::Emitter& out)
	{
		out << YAML::Key << "Mesh" << YAML::Value << s.Mesh;
		out << YAML::Key << "Material" << YAML::Value << s.Material;
		out << YAML::Key << "Instanced" << YAML::Value << s.Instanced;
	}

	static void SerializeComponent(const Rigidbody2DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Type" << YAML::Value << Rb2DTypeToString(c.Type);
		out << YAML::Key << "FixedRotation" << YAML::Value << c.FixedRotation;
	}

	static void SerializeComponent(const Rigidbody3DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Type" << YAML::Value << Rb3DTypeToString(c.Type);
		out << YAML::Key << "GravityFactor" << YAML::Value << c.GravityFactor;
		out << YAML::Key << "Mass" << YAML::Value << c.Mass;
	}

	static void SerializeComponent(const BoxCollider2DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Size" << YAML::Value << c.Size;
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;

		out << YAML::Key << "Material" << YAML::Value << c.Material;
	}

	static void SerializeComponent(const CircleCollider2DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Radius" << YAML::Value << c.Radius;
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;

		out << YAML::Key << "Material" << YAML::Value << c.Material;
	}

	static void SerializeComponent(const PolygonCollider2DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;
		out << YAML::Key << "Points" << YAML::Value << YAML::BeginSeq;

		for (auto& point : c.Points)
			out << point;

		out << YAML::EndSeq;
		out << YAML::Key << "Material" << YAML::Value << c.Material;
	}

	static void SerializeComponent(const BoxCollider3DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Size" << YAML::Value << c.Size;
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;
		out << YAML::Key << "Material" << YAML::Value << c.Material;
	}

	static void SerializeComponent(const MeshCollider3DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;
		out << YAML::Key << "Mesh" << YAML::Value << c.Mesh;
		out << YAML::Key << "Material" << YAML::Value << c.Material;
	}

	static void SerializeComponent(const SphereCollider3DComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Radius" << YAML::Value << c.Radius;
		out << YAML::Key << "Offset" << YAML::Value << c.Offset;
		out << YAML::Key << "Material" << YAML::Value << c.Material;
	}

	static void SerializeComponent(const DirectionalLightComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Direction" << YAML::Value << c.Direction;
		out << YAML::Key << "EnableShadows" << YAML::Value << c.CastShadows;
		out << YAML::Key << "Color" << YAML::Value << c.Color;
		out << YAML::Key << "Intensity" << YAML::Value << c.Intensity;
	}

	static void SerializeComponent(const PointLightComponent& c, YAML::Emitter& out)
	{
		out << YAML::Key << "Color" << YAML::Value << c.Color;
		out << YAML::Key << "Intensity" << YAML::Value << c.Intensity;
		out << YAML::Key << "Radius" << YAML::Value << c.Radius;
	}

	template <typename T>
	static void DeserializeComponent(Entity e, YAML::Node& in, Ref<Scene> scene = nullptr)
	{
		DeserializeComponent<T>(e, in, scene);
	}

	template <>
	static void DeserializeComponent<IDComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in) return;

		IDComponent& id = e.GetComponent<IDComponent>();

		id.ID = in["ID"].as<uint64_t>();
		id.Owner = in["Owner"] ? in["Owner"].as<uint64_t>() : id.ID;
	}

	template <>
	static void DeserializeComponent<TagComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in) return;
		TagComponent& tag = e.AddComponent<TagComponent>();
		tag.Name = in["Name"].as<std::string>();
		tag.Tag = in["Tag"].as<std::string>();
		tag.Owner = in["Owner"] ? in["Owner"].as<uint64_t>() : 0;
	}

	template <>
	static void DeserializeComponent<TransformComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		TransformComponent& transform = e.AddComponent<TransformComponent>();

		transform.Owner = e.GetComponent<IDComponent>().ID;
		transform.Translation = in["Translation"].as<glm::vec3>();
		transform.Rotation = in["Rotation"].as<glm::vec3>();
		transform.Scale = in["Scale"].as<glm::vec3>();

		if (in["Parent"] && in["Parent"].as<uint64_t>() != 0)
		{
			Entity parent = scene->GetEntityByID(in["Parent"].as<uint64_t>());
			transform.Parent = parent;
			parent.Transform().Children.push_back(transform.Owner);
		}
		else
			transform.Parent = Entity(entt::null, nullptr);
	}

	template <>
	static void DeserializeComponent<CameraComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		CameraComponent& cc = e.AddComponent<CameraComponent>();
		cc.FixedAspectRatio = in["FixedAspectRatio"].as<bool>();
		cc.Primary = in["Primary"].as<bool>();
		cc.PostProcessing = in["PostProcessingStack"] ? in["PostProcessingStack"].as<uint64_t>() : 0;

		cc.Camera.SetOrthoSize(in["CameraData"]["OrthoSize"].as<float>());
		cc.Camera.SetNearPlane(in["CameraData"]["OrthoNear"].as<float>());
		cc.Camera.SetFarPlane(in["CameraData"]["OrthoFar"].as<float>());

		cc.Camera.SetFOV(in["CameraData"]["PerspFOV"].as<float>());
		cc.Camera.SetNearPlane(in["CameraData"]["PerspNear"].as<float>());
		cc.Camera.SetFarPlane(in["CameraData"]["PerspFar"].as<float>());
		cc.Camera.SetViewportSize(scene->GetViewportSize().x, scene->GetViewportSize().y);

		cc.Camera.SetProjectionType((SceneCamera::ProjectionType)in["CameraData"]["ProjectionType"].as<int>());
	}

	template<>
	static void DeserializeComponent<SpriteRendererComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		SpriteRendererComponent& sc = e.AddComponent<SpriteRendererComponent>();
		sc.Color = in["Color"].as<glm::vec4>();
		if (in["Texture"]) sc.Texture = in["Texture"].as<uint64_t>();
		if (in["TilingFactor"])		sc.TilingFactor = in["TilingFactor"].as<float>();
	}

	template<>
	static void DeserializeComponent<MeshRendererComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		bool instanced = in["Instanced"].as<bool>();
		UUID material = in["Material"].as<uint64_t>();
		UUID mesh = in["Mesh"].as<uint64_t>();

		MeshRendererComponent& mr = e.AddComponent<MeshRendererComponent>(mesh, material, e.ID(), instanced);
	}

	template<>
	static void DeserializeComponent<Rigidbody2DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		Rigidbody2DComponent& rb2d = e.AddComponent<Rigidbody2DComponent>();
		rb2d.FixedRotation = in["FixedRotation"].as<bool>();
		rb2d.Type = Rigidbody2DComponent::StrToRigidbody2DType(in["Type"].as<std::string>());
	}

	template<>
	static void DeserializeComponent<Rigidbody3DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		Rigidbody3DComponent& rb3d = e.AddComponent<Rigidbody3DComponent>();
		rb3d.Type = Rigidbody3DComponent::StrToRigidbody3DType(in["Type"].as<std::string>());
		rb3d.GravityFactor = in["GravityFactor"] ? in["GravityFactor"].as<float>() : 1.0f;
		rb3d.Mass = in["Mass"] ? in["Mass"].as<float>() : 1.0f;
	}

	template<>
	static void DeserializeComponent<BoxCollider2DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		BoxCollider2DComponent& bc2d = e.AddComponent<BoxCollider2DComponent>();
		bc2d.Offset = in["Offset"].as<glm::vec2>();
		bc2d.Size = in["Size"].as<glm::vec2>();
		bc2d.Material = in["Material"] ? in["Material"].as<uint64_t>() : 0;
	}

	template<>
	static void DeserializeComponent<CircleCollider2DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		CircleCollider2DComponent& cc2d = e.AddComponent<CircleCollider2DComponent>();
		cc2d.Offset = in["Offset"].as<glm::vec2>();
		cc2d.Radius = in["Radius"].as<float>();
		cc2d.Material = in["Material"].as<uint64_t>();
	}

	template<>
	static void DeserializeComponent<PolygonCollider2DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		PolygonCollider2DComponent& pc2d = e.AddComponent<PolygonCollider2DComponent>();
		pc2d.Offset = in["Offset"].as<glm::vec2>();
		uint32_t i = 0;
		// Remove default points
		for (uint32_t j = 0; j < 4; j++)
			pc2d.RemovePoint(0);

		for (auto& vec : in["Points"])
		{
			pc2d.AddPoint();
			pc2d.SetPoint(i, vec.as<glm::vec2>());
			i++;
		}
		pc2d.Triangulate();
		pc2d.Material = in["Material"].as<uint64_t>();
	}

	template<>
	static void DeserializeComponent<BoxCollider3DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		BoxCollider3DComponent& bc3d = e.AddComponent<BoxCollider3DComponent>();
		bc3d.Offset = in["Offset"].as<glm::vec3>();
		bc3d.Size = in["Size"].as<glm::vec3>();
		bc3d.Material = in["Material"] ? in["Material"].as<uint64_t>() : 0;
	}

	template<>
	static void DeserializeComponent<SphereCollider3DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		SphereCollider3DComponent& bc3d = e.AddComponent<SphereCollider3DComponent>();
		bc3d.Offset = in["Offset"].as<glm::vec3>();
		bc3d.Radius = in["Radius"].as<float>();
		bc3d.Material = in["Material"] ? in["Material"].as<uint64_t>() : 0;
	}

	template<>
	static void DeserializeComponent<MeshCollider3DComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		MeshCollider3DComponent& mesh = e.AddComponent<MeshCollider3DComponent>();
		mesh.Offset = in["Offset"].as<glm::vec3>();
		mesh.Mesh = in["Mesh"].as<uint64_t>();
		mesh.Material = in["Material"].as<uint64_t>();
	}

	template<>
	static void DeserializeComponent<DirectionalLightComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		DirectionalLightComponent& dl = e.AddComponent<DirectionalLightComponent>();
		dl.Direction = in["Direction"].as<glm::vec3>();
		dl.Color = in["Color"].as<glm::vec3>();
		dl.Intensity = in["Intensity"].as<float>();
		dl.CastShadows = in["EnableShadows"] ? in["EnableShadows"].as<bool>() : false;
	}

	template<>
	static void DeserializeComponent<PointLightComponent>(Entity e, YAML::Node& in, Ref<Scene> scene)
	{
		if (!in)
			return;
		PointLightComponent& dl = e.AddComponent<PointLightComponent>();
		dl.Color = in["Color"].as<glm::vec3>();
		dl.Intensity = in["Intensity"].as<float>();
		dl.Radius = in["Radius"].as<float>();
	}

	void SceneSerializer::SerializeEntity(EntitySceneNode& node, YAML::Emitter& out)
	{
		Entity entity = node.EntityData;
		out << YAML::BeginMap << YAML::Key << "Entity: " << YAML::Value << entity.ID();
		out << YAML::Key << "HierarchyOrder" << YAML::Value << node.IndexInNode;

		SerializeComponent<IDComponent>(entity, "IDComponent", out);
		SerializeComponent<TagComponent>(entity, "TagComponent", out);
		SerializeComponent<TransformComponent>(entity, "TransformComponent", out);
		SerializeComponent<CameraComponent>(entity, "CameraComponent", out);
		SerializeComponent<SpriteRendererComponent>(entity, "SpriteRendererComponent", out);
		SerializeComponent<MeshRendererComponent>(entity, "MeshRendererComponent", out);

		SerializeComponent<Rigidbody2DComponent>(entity, "Rigidbody2DComponent", out);
		SerializeComponent<Rigidbody3DComponent>(entity, "Rigidbody3DComponent", out);

		SerializeComponent<BoxCollider2DComponent>(entity, "BoxCollider2DComponent", out);
		SerializeComponent<CircleCollider2DComponent>(entity, "CircleCollider2DComponent", out);
		SerializeComponent<PolygonCollider2DComponent>(entity, "PolygonCollider2DComponent", out);

		SerializeComponent<BoxCollider3DComponent>(entity, "BoxCollider3DComponent", out);
		SerializeComponent<SphereCollider3DComponent>(entity, "SphereCollider3DComponent", out);
		SerializeComponent<MeshCollider3DComponent>(entity, "MeshCollider3DComponent", out);

		SerializeComponent<DirectionalLightComponent>(entity, "DirectionalLightComponent", out);
		SerializeComponent<PointLightComponent>(entity, "PointLightComponent", out);

		out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
		for (uint32_t i = 0; i < node.Children.size(); i++)
			SerializeEntity(*node.Children[i], out);
		out << YAML::EndSeq;

		out << YAML::EndMap;
	}

	void SceneSerializer::SerializeText(const std::string& fileName, const EntitySceneNode& sceneGraph, 
		const YAML::Node& additionalData)
	{
		YAML::Emitter out;
		std::ofstream outFile(fileName);
		
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled scene";
		
		out << YAML::Key << "AdditionalData" << YAML::Value << additionalData;
		out << YAML::Key << "Lighting" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "AmbientLightColor" << YAML::Value << m_Scene->GetAmbientLight();
		out << YAML::Key << "AmbientLightIntensity" << YAML::Value << m_Scene->GetAmbientLightIntensity();
		Ref<Skybox> skybox = m_Scene->GetSkybox();
		out << YAML::Key << "Skybox" << YAML::Value << (skybox == nullptr ? 0 : m_Scene->GetSkybox()->GetID());
		out << YAML::EndMap;

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		for (uint32_t i = 0; i < sceneGraph.Children.size(); i++)
			SerializeEntity(*sceneGraph.Children[i], out);

		out << YAML::EndSeq;
		out << YAML::EndMap;

		Log.CoreInfo("{0}", out.c_str());

		outFile << out.c_str();
	}

	EntitySceneNode* SceneSerializer::DeserializeText(const std::string& fileName, YAML::Node& additionalData)
	{
		if (!CppUtils::String::EndsWith(fileName, ".debut"))
			return nullptr;

		std::ifstream inFile(fileName);
		std::stringstream strStream;

		strStream << inFile.rdbuf();

		YAML::Node in = YAML::Load(strStream.str());

		if (!in["Scene"])
			return nullptr;

		if (in["AdditionalData"])
		{
			additionalData = in["AdditionalData"];
			additionalData["Valid"] = true;
		}

		auto entities = in["Entities"];
		EntitySceneNode* sceneTree = new EntitySceneNode();
		sceneTree->IsRoot = true;

		if (entities)
			for (auto yamlEntity : entities)
				sceneTree->Children.push_back(DeserializeEntity(yamlEntity));

		YAML::Node lighting = in["Lighting"];
		if (lighting["AmbientLightColor"].IsDefined())
			m_Scene->SetAmbientLight(lighting["AmbientLightColor"].as<glm::vec3>());
		if (lighting["AmbientLightIntensity"].IsDefined())
			m_Scene->SetAmbientLightIntensity(lighting["AmbientLightIntensity"].as<float>());
		if (lighting["Skybox"].IsDefined() && lighting["Skybox"].as<uint64_t>() != 0)
			m_Scene->SetSkybox(lighting["Skybox"].as<uint64_t>());

		return sceneTree;
	}

	EntitySceneNode* SceneSerializer::DeserializeEntity(YAML::Node& yamlEntity)
	{
		// Create a new entity, set the tag and name
		auto idComponent = yamlEntity["IDComponent"];
		Entity entity = m_Scene->CreateEmptyEntity(idComponent["ID"].as<uint64_t>());
		EntitySceneNode* node = new EntitySceneNode(false, entity);
		node->IndexInNode = yamlEntity["HierarchyOrder"].as<uint32_t>();

		// Deserialize the other components
		DeserializeComponent<IDComponent>(entity, yamlEntity["IDComponent"]);
		DeserializeComponent<TagComponent>(entity, yamlEntity["TagComponent"]);
		DeserializeComponent<TransformComponent>(entity, yamlEntity["TransformComponent"], m_Scene);

		DeserializeComponent<CameraComponent>(entity, yamlEntity["CameraComponent"], m_Scene);
		DeserializeComponent<SpriteRendererComponent>(entity, yamlEntity["SpriteRendererComponent"]);
		DeserializeComponent<MeshRendererComponent>(entity, yamlEntity["MeshRendererComponent"]);

		DeserializeComponent<Rigidbody2DComponent>(entity, yamlEntity["Rigidbody2DComponent"]);
		DeserializeComponent<Rigidbody3DComponent>(entity, yamlEntity["Rigidbody3DComponent"]);
		
		DeserializeComponent<BoxCollider2DComponent>(entity, yamlEntity["BoxCollider2DComponent"]);
		DeserializeComponent<PolygonCollider2DComponent>(entity, yamlEntity["PolygonCollider2DComponent"]);
		DeserializeComponent<CircleCollider2DComponent>(entity, yamlEntity["CircleCollider2DComponent"]);

		DeserializeComponent<BoxCollider3DComponent>(entity, yamlEntity["BoxCollider3DComponent"]);
		DeserializeComponent<SphereCollider3DComponent>(entity, yamlEntity["SphereCollider3DComponent"]);
		DeserializeComponent<MeshCollider3DComponent>(entity, yamlEntity["MeshCollider3DComponent"]);

		DeserializeComponent<DirectionalLightComponent>(entity, yamlEntity["DirectionalLightComponent"]);
		DeserializeComponent<PointLightComponent>(entity, yamlEntity["PointLightComponent"]);

		auto children = yamlEntity["Children"];
		for (auto child : children)
			node->Children.push_back(DeserializeEntity(child));

		return node;
	}
}