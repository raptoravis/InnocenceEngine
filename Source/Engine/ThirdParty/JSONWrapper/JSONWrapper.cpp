#include "JSONWrapper.h"
#include "../../Common/CommonMacro.inl"
#include "../../ComponentManager/ITransformComponentManager.h"
#include "../../ComponentManager/IVisibleComponentManager.h"
#include "../../ComponentManager/ILightComponentManager.h"
#include "../../ComponentManager/ICameraComponentManager.h"
#include "../../Core/InnoLogger.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../../Core/IOService.h"

namespace JSONWrapper
{
#define LoadComponentData(className, j, entity) \
	{ auto l_result = SpawnComponent(className, entity, ObjectSource::Asset, ObjectOwnership::Client); \
	from_json(j, *l_result); }

	template<typename T>
	inline bool saveComponentData(json& topLevel, T* rhs)
	{
		json j;
		to_json(j, *rhs);

		auto l_result = std::find_if(
			topLevel["SceneEntities"].begin(),
			topLevel["SceneEntities"].end(),
			[&](auto val) -> bool {
				return val["UUID"] == rhs->m_ParentEntity->m_UUID;
			});

		if (l_result != topLevel["SceneEntities"].end())
		{
			l_result.value()["ChildrenComponents"].emplace_back(j);
			return true;
		}
		else
		{
			InnoLogger::Log(LogLevel::Warning, "FileSystem: saveComponentData<T>: UUID ", rhs->m_ParentEntity->m_UUID, " is invalid.");
			return false;
		}
	}

	Model* processSceneJsonData(const json& j, bool AsyncUploadGPUResource = true);
	bool processAnimationJsonData(const json& j, bool AsyncUploadGPUResource = true);
	ArrayRangeInfo processMeshJsonData(const json& j, bool AsyncUploadGPUResource = true);
	SkeletonDataComponent* processSkeletonJsonData(const json& j, const char* name, bool AsyncUploadGPUResource = true);
	MaterialDataComponent* processMaterialJsonData(const json& j, const char* name, bool AsyncUploadGPUResource = true);

	bool assignComponentRuntimeData();

	ThreadSafeQueue<std::pair<TransformComponent*, ObjectName>> m_orphanTransformComponents;
}

bool JSONWrapper::loadJsonDataFromDisk(const char* fileName, json& data)
{
	std::ifstream i;

	i.open(IOService::getWorkingDirectory() + fileName);

	if (!i.is_open())
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: Can't open JSON file : ", fileName, "!");
		return false;
	}

	i >> data;
	i.close();

	return true;
}

bool JSONWrapper::saveJsonDataToDisk(const char* fileName, const json& data)
{
	std::ofstream o;
	o.open(IOService::getWorkingDirectory() + fileName, std::ios::out | std::ios::trunc);
	o << std::setw(4) << data << std::endl;
	o.close();

	InnoLogger::Log(LogLevel::Verbose, "FileSystem: JSON file : ", fileName, " has been saved.");

	return true;
}

void JSONWrapper::to_json(json& j, const InnoEntity& p)
{
	j = json
	{
		{"UUID", p.m_UUID},
		{"ObjectName", p.m_Name.c_str()},
	};
}

void JSONWrapper::to_json(json& j, const Vec4& p)
{
	j = json
	{
		{
			"X", p.x
		},
		{
			"Y", p.y
		},
		{
			"Z", p.z
		},
		{
			"W", p.w
		}
	};
}

void JSONWrapper::to_json(json& j, const Mat4& p)
{
	j["00"] = p.m00;
	j["01"] = p.m01;
	j["02"] = p.m02;
	j["03"] = p.m03;
	j["10"] = p.m10;
	j["11"] = p.m11;
	j["12"] = p.m12;
	j["13"] = p.m13;
	j["20"] = p.m20;
	j["21"] = p.m21;
	j["22"] = p.m22;
	j["23"] = p.m23;
	j["30"] = p.m30;
	j["31"] = p.m31;
	j["32"] = p.m32;
	j["33"] = p.m33;
}

void JSONWrapper::from_json(const json& j, Mat4& p)
{
	p.m00 = j["00"];
	p.m01 = j["01"];
	p.m02 = j["02"];
	p.m03 = j["03"];
	p.m10 = j["10"];
	p.m11 = j["11"];
	p.m12 = j["12"];
	p.m13 = j["13"];
	p.m20 = j["20"];
	p.m21 = j["21"];
	p.m22 = j["22"];
	p.m23 = j["23"];
	p.m30 = j["30"];
	p.m31 = j["31"];
	p.m32 = j["32"];
	p.m33 = j["33"];
}

void JSONWrapper::to_json(json& j, const TransformVector& p)
{
	j = json
	{
		{
			"Position",
			{
				{
					"X", p.m_pos.x
				},
				{
					"Y", p.m_pos.y
				},
				{
					"Z", p.m_pos.z
				}
			}
		},
		{
			"Rotation",
			{
				{
					"X", p.m_rot.x
				},
				{
					"Y", p.m_rot.y
				},
				{
					"Z", p.m_rot.z
				},
				{
					"W", p.m_rot.w
				}
			}
		},
		{
			"Scale",
			{
				{
					"X", p.m_scale.x
				},
				{
					"Y", p.m_scale.y
				},
				{
					"Z", p.m_scale.z
				},
			}
		}
	};
}

void JSONWrapper::to_json(json& j, const TransformComponent& p)
{
	json localTransformVector;

	to_json(localTransformVector, p.m_localTransformVector);

	auto parentTransformComponentEntityName = p.m_parentTransformComponent->m_ParentEntity->m_Name;

	j = json
	{
		{"ComponentType", p.m_ComponentType},
		{"ParentTransformComponentEntityName", parentTransformComponentEntityName.c_str()},
		{"LocalTransformVector", localTransformVector },
	};
}

void JSONWrapper::to_json(json& j, const VisibleComponent& p)
{
	j = json
	{
		{"ComponentType", p.m_ComponentType},
		{"Visibility", p.m_visibility},
		{"MeshPrimitiveTopology", p.m_meshPrimitiveTopology},
		{"TextureWrapMethod", p.m_textureWrapMethod},
		{"MeshUsage", p.m_meshUsage},
		{"MeshSource", p.m_meshSource},
		{"ProceduralMeshShape", p.m_proceduralMeshShape},
		{"ModelFileName", p.m_modelFileName},
		{"SimulatePhysics", p.m_simulatePhysics},
	};
}

void JSONWrapper::to_json(json& j, const LightComponent& p)
{
	json color;
	to_json(color, p.m_RGBColor);

	json shape;
	to_json(shape, p.m_Shape);

	j = json
	{
		{"ComponentType", p.m_ComponentType},
		{"RGBColor", color},
		{"Shape", shape},
		{"LightType", p.m_LightType},
		{"ColorTemperature", p.m_ColorTemperature},
		{"LuminousFlux", p.m_LuminousFlux},
		{"UseColorTemperature", p.m_UseColorTemperature},
	};
}

void JSONWrapper::to_json(json& j, const CameraComponent& p)
{
	j = json
	{
		{"ComponentType", p.m_ComponentType},
		{"FOVX", p.m_FOVX},
		{"WidthScale", p.m_widthScale},
		{"HeightScale", p.m_heightScale},
		{"zNear", p.m_zNear},
		{"zFar", p.m_zFar},
	};
}

void JSONWrapper::from_json(const json& j, Vec4& p)
{
	p.x = j["X"];
	p.y = j["Y"];
	p.z = j["Z"];
	p.w = j["W"];
}

void JSONWrapper::from_json(const json& j, TransformComponent& p)
{
	from_json(j["LocalTransformVector"], p.m_localTransformVector);
	auto l_parentTransformComponentEntityName = j["ParentTransformComponentEntityName"];
	if (l_parentTransformComponentEntityName == "RootTransform")
	{
		auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());
		p.m_parentTransformComponent = l_rootTranformComponent;
	}
	else
	{
		// JSON is an order-irrelevant format, so the parent transform component would always be instanciated in random point, then it's necessary to assign it later
		std::string l_parentName = l_parentTransformComponentEntityName;
		l_parentName += "/";
		m_orphanTransformComponents.push({ &p, ObjectName(l_parentName.c_str()) });
	}
}

void JSONWrapper::from_json(const json& j, TransformVector& p)
{
	p.m_pos.x = j["Position"]["X"];
	p.m_pos.y = j["Position"]["Y"];
	p.m_pos.z = j["Position"]["Z"];
	p.m_pos.w = 1.0f;

	p.m_rot.x = j["Rotation"]["X"];
	p.m_rot.y = j["Rotation"]["Y"];
	p.m_rot.z = j["Rotation"]["Z"];
	p.m_rot.w = j["Rotation"]["W"];

	p.m_scale.x = j["Scale"]["X"];
	p.m_scale.y = j["Scale"]["Y"];
	p.m_scale.z = j["Scale"]["Z"];
	p.m_scale.w = 1.0f;
}

void JSONWrapper::from_json(const json& j, VisibleComponent& p)
{
	p.m_visibility = j["Visibility"];
	p.m_meshPrimitiveTopology = j["MeshPrimitiveTopology"];
	p.m_textureWrapMethod = j["TextureWrapMethod"];
	p.m_meshUsage = j["MeshUsage"];
	p.m_meshSource = j["MeshSource"];
	p.m_proceduralMeshShape = j["ProceduralMeshShape"];
	p.m_modelFileName = j["ModelFileName"];
	p.m_simulatePhysics = j["SimulatePhysics"];
}

void JSONWrapper::from_json(const json& j, LightComponent& p)
{
	from_json(j["RGBColor"], p.m_RGBColor);
	from_json(j["Shape"], p.m_Shape);
	p.m_LightType = j["LightType"];
	p.m_ColorTemperature = j["ColorTemperature"];
	p.m_LuminousFlux = j["LuminousFlux"];
	p.m_UseColorTemperature = j["UseColorTemperature"];
}

void JSONWrapper::from_json(const json& j, CameraComponent& p)
{
	p.m_FOVX = j["FOVX"];
	p.m_widthScale = j["WidthScale"];
	p.m_heightScale = j["HeightScale"];
	p.m_zNear = j["zNear"];
	p.m_zFar = j["zFar"];
}

Model* JSONWrapper::loadModelFromDisk(const char* fileName, bool AsyncUploadGPUResource)
{
	json j;

	loadJsonDataFromDisk(fileName, j);

	return processSceneJsonData(j, AsyncUploadGPUResource);
}

Model* JSONWrapper::processSceneJsonData(const json& j, bool AsyncUploadGPUResource)
{
	Model* l_result;

	if (j.find("Meshes") != j.end())
	{
		l_result = g_pModuleManager->getAssetSystem()->addModel();
		l_result->meshMaterialPairs = processMeshJsonData(j["Meshes"], AsyncUploadGPUResource);
	}

	if (j.find("Animations") != j.end())
	{
		processAnimationJsonData(j["Animations"], AsyncUploadGPUResource);
	}

	return l_result;
}

bool JSONWrapper::processAnimationJsonData(const json& j, bool AsyncUploadGPUResource)
{
	for (auto i : j)
	{
		std::string l_animationFileName = i["File"];

		std::ifstream l_animationFile(IOService::getWorkingDirectory() + l_animationFileName, std::ios::binary);

		if (!l_animationFile.is_open())
		{
			InnoLogger::Log(LogLevel::Error, "FileSystem: std::ifstream: can't open file ", l_animationFileName.c_str(), "!");
			return false;
		}

		auto l_ADC = g_pModuleManager->getRenderingFrontend()->addAnimationDataComponent();
		l_ADC->m_Name = (l_animationFileName + "//").c_str();

		std::streamoff l_offset = 0;

		IOService::deserialize(l_animationFile, l_offset, &l_ADC->m_Duration);
		l_offset += sizeof(l_ADC->m_Duration);
		IOService::deserialize(l_animationFile, l_offset, &l_ADC->m_NumChannels);
		l_offset += sizeof(l_ADC->m_NumChannels);
		IOService::deserialize(l_animationFile, l_offset, &l_ADC->m_NumTicks);
		l_offset += sizeof(l_ADC->m_NumTicks);

		auto l_keyDataSize = IOService::getFileSize(l_animationFile) - l_offset;
		l_ADC->m_KeyData.resize(l_keyDataSize / sizeof(KeyData));
		IOService::deserializeVector(l_animationFile, l_offset, l_keyDataSize, l_ADC->m_KeyData);

		g_pModuleManager->getAssetSystem()->recordLoadedAnimation(l_animationFileName.c_str(), l_ADC);
		g_pModuleManager->getRenderingFrontend()->registerAnimationDataComponent(l_ADC, AsyncUploadGPUResource);
	}

	return true;
}

ArrayRangeInfo JSONWrapper::processMeshJsonData(const json& j, bool AsyncUploadGPUResource)
{
	auto l_result = g_pModuleManager->getAssetSystem()->addMeshMaterialPairs(j.size());

	uint64_t l_currentIndex = 0;

	for (auto i : j)
	{
		auto l_currentMeshMaterialPair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(l_result.m_startOffset + l_currentIndex);

		// Load material data
		if (i.find("Material") != i.end())
		{
			std::string l_materialName = i["Name"];
			l_materialName += "_Material";
			l_currentMeshMaterialPair->material = processMaterialJsonData(i["Material"], l_materialName.c_str(), AsyncUploadGPUResource);
		}
		else
		{
			l_currentMeshMaterialPair->material = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
			l_currentMeshMaterialPair->material->m_ObjectStatus = ObjectStatus::Created;
			g_pModuleManager->getRenderingFrontend()->registerMaterialDataComponent(l_currentMeshMaterialPair->material, AsyncUploadGPUResource);
		}

		MeshSource l_meshSource = MeshSource(i["MeshSource"].get<int32_t>());

		// Load custom mesh data
		if (l_meshSource == MeshSource::Customized)
		{
			auto l_meshFileName = i["File"].get<std::string>();

			MeshMaterialPair* l_loadedMeshMaterialPair;

			// check if this file has already been loaded once
			if (g_pModuleManager->getAssetSystem()->findLoadedMeshMaterialPair(l_meshFileName.c_str(), l_loadedMeshMaterialPair))
			{
				l_currentMeshMaterialPair = l_loadedMeshMaterialPair;
			}
			else
			{
				std::ifstream l_meshFile(IOService::getWorkingDirectory() + l_meshFileName, std::ios::binary);

				if (!l_meshFile.is_open())
				{
					InnoLogger::Log(LogLevel::Error, "FileSystem: std::ifstream: can't open file ", l_meshFileName.c_str(), "!");
				}

				auto l_mesh = g_pModuleManager->getRenderingFrontend()->addMeshDataComponent();
				l_mesh->m_Name = (l_meshFileName + "//").c_str();

				size_t l_verticesNumber = i["VerticesNumber"];
				size_t l_indicesNumber = i["IndicesNumber"];

				l_mesh->m_vertices.reserve(l_verticesNumber);
				l_mesh->m_vertices.fulfill();
				l_mesh->m_indices.reserve(l_indicesNumber);
				l_mesh->m_indices.fulfill();

				IOService::deserializeVector(l_meshFile, 0, l_verticesNumber * sizeof(Vertex), l_mesh->m_vertices);
				IOService::deserializeVector(l_meshFile, l_verticesNumber * sizeof(Vertex), l_indicesNumber * sizeof(Index), l_mesh->m_indices);

				l_meshFile.close();

				l_mesh->m_indicesSize = l_mesh->m_indices.size();

				l_currentMeshMaterialPair->mesh = l_mesh;

				// Load bones data
				if (i.find("Bones") != i.end())
				{
					std::string l_skeletonName = i["Name"];
					l_skeletonName += "_Skeleton";
					l_currentMeshMaterialPair->mesh->m_SDC = processSkeletonJsonData(i, l_skeletonName.c_str(), AsyncUploadGPUResource);
				}

				l_currentMeshMaterialPair->mesh->m_meshSource = MeshSource::Customized;
				l_currentMeshMaterialPair->mesh->m_ObjectStatus = ObjectStatus::Created;

				g_pModuleManager->getRenderingFrontend()->registerMeshDataComponent(l_mesh, AsyncUploadGPUResource);

				g_pModuleManager->getAssetSystem()->recordLoadedMeshMaterialPair(l_meshFileName.c_str(), l_currentMeshMaterialPair);
			}
		}
		else
		{
			ProceduralMeshShape l_proceduralMeshShape = ProceduralMeshShape(i["ProceduralMeshShape"].get<int32_t>());

			l_currentMeshMaterialPair->mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(l_proceduralMeshShape);
		}

		l_currentIndex++;
	}

	return l_result;
}

SkeletonDataComponent* JSONWrapper::processSkeletonJsonData(const json& j, const char* name, bool AsyncUploadGPUResource)
{
	SkeletonDataComponent* l_SDC;

	// check if this file has already been loaded once
	if (g_pModuleManager->getAssetSystem()->findLoadedSkeleton(name, l_SDC))
	{
		return l_SDC;
	}
	else
	{
		l_SDC = g_pModuleManager->getRenderingFrontend()->addSkeletonDataComponent();
		l_SDC->m_Name = (std::string(name) + ("//")).c_str();

		auto l_size = j["Bones"].size();
		l_SDC->m_BoneData.reserve(l_size);
		l_SDC->m_BoneData.fulfill();

		for (auto i : j["Bones"])
		{
			BoneData l_boneData;
			from_json(i["Transformation"], l_boneData.m_L2B);
			l_SDC->m_BoneData[i["ID"]] = l_boneData;
		}

		g_pModuleManager->getAssetSystem()->recordLoadedSkeleton(name, l_SDC);
		g_pModuleManager->getRenderingFrontend()->registerSkeletonDataComponent(l_SDC, AsyncUploadGPUResource);

		return l_SDC;
	}
}

MaterialDataComponent* JSONWrapper::processMaterialJsonData(const json& j, const char* name, bool AsyncUploadGPUResource)
{
	auto l_MDC = g_pModuleManager->getRenderingFrontend()->addMaterialDataComponent();
	l_MDC->m_Name = (std::string(name) + ("//")).c_str();
	auto l_defaultMaterial = g_pModuleManager->getRenderingFrontend()->getDefaultMaterialDataComponent();

	if (j.find("Textures") != j.end())
	{
		for (auto i : j["Textures"])
		{
			std::string l_textureFile = i["File"];
			auto l_textureSlotIndex = i["TextureSlotIndex"];

			auto l_TDC = g_pModuleManager->getAssetSystem()->loadTexture(l_textureFile.c_str());
			if (l_TDC)
			{
				l_TDC->m_TextureDesc.Sampler = TextureSampler(i["Sampler"]);
				l_TDC->m_TextureDesc.Usage = TextureUsage(i["Usage"]);
				l_TDC->m_TextureDesc.IsSRGB = i["IsSRGB"];

				l_MDC->m_TextureSlots[l_textureSlotIndex].m_Texture = l_TDC;
			}
			else
			{
				l_MDC->m_TextureSlots[l_textureSlotIndex] = l_defaultMaterial->m_TextureSlots[l_textureSlotIndex];
			}
		}
	}

	l_MDC->m_materialAttributes.AlbedoR = j["Albedo"]["R"];
	l_MDC->m_materialAttributes.AlbedoG = j["Albedo"]["G"];
	l_MDC->m_materialAttributes.AlbedoB = j["Albedo"]["B"];
	l_MDC->m_materialAttributes.Alpha = j["Albedo"]["A"];
	l_MDC->m_materialAttributes.Metallic = j["Metallic"];
	l_MDC->m_materialAttributes.Roughness = j["Roughness"];
	l_MDC->m_materialAttributes.AO = j["AO"];
	l_MDC->m_materialAttributes.Thickness = j["Thickness"];
	l_MDC->m_ObjectStatus = ObjectStatus::Created;

	g_pModuleManager->getRenderingFrontend()->registerMaterialDataComponent(l_MDC, AsyncUploadGPUResource);

	return l_MDC;
}

bool JSONWrapper::saveScene(const char* fileName)
{
	json topLevel;
	topLevel["SceneName"] = fileName;

	// save entities name and ID
	for (auto i : g_pModuleManager->getEntityManager()->GetEntities())
	{
		if (i->m_ObjectSource == ObjectSource::Asset)
		{
			json j;
			to_json(j, *i);
			topLevel["SceneEntities"].emplace_back(j);
		}
	}

	// save children components
	for (auto i : GetComponentManager(TransformComponent)->GetAllComponents())
	{
		if (i->m_ObjectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : GetComponentManager(VisibleComponent)->GetAllComponents())
	{
		if (i->m_ObjectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : GetComponentManager(LightComponent)->GetAllComponents())
	{
		if (i->m_ObjectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : GetComponentManager(CameraComponent)->GetAllComponents())
	{
		if (i->m_ObjectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}

	saveJsonDataToDisk(fileName, topLevel);

	InnoLogger::Log(LogLevel::Success, "FileSystem: Scene ", fileName, " has been saved.");

	return true;
}

bool JSONWrapper::loadScene(const char* fileName)
{
	json j;
	if (!loadJsonDataFromDisk(fileName, j))
	{
		return false;
	}

	auto l_sceneName = j["SceneName"];

	for (auto i : j["SceneEntities"])
	{
		std::string l_entityName = i["ObjectName"];
		l_entityName += "/";
		auto l_entity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Asset, ObjectOwnership::Client, l_entityName.c_str());

		for (auto k : i["ChildrenComponents"])
		{
			uint32_t componentTypeID = k["ComponentType"];

			if (componentTypeID == 1)
			{
				LoadComponentData(TransformComponent, k, l_entity)
			}
			else if (componentTypeID == 2)
			{
				LoadComponentData(VisibleComponent, k, l_entity)
			}
			else if (componentTypeID == 3)
			{
				LoadComponentData(LightComponent, k, l_entity)
			}
			else if (componentTypeID == 4)
			{
				LoadComponentData(CameraComponent, k, l_entity)
			}
			else
			{
				InnoLogger::Log(LogLevel::Error, "JSONWrapper: Unknown ComponentTypeID: ", componentTypeID);
			}
		}
	}

	InnoLogger::Log(LogLevel::Success, "FileSystem: Scene loading finished.");

	assignComponentRuntimeData();

	return true;
}

bool JSONWrapper::assignComponentRuntimeData()
{
	while (m_orphanTransformComponents.size() > 0)
	{
		std::pair<TransformComponent*, ObjectName> l_orphan;
		if (m_orphanTransformComponents.tryPop(l_orphan))
		{
			auto l_entity = g_pModuleManager->getEntityManager()->Find(l_orphan.second.c_str());

			if (l_entity.has_value())
			{
				l_orphan.first->m_parentTransformComponent = GetComponent(TransformComponent, *l_entity);
			}
			else
			{
				InnoLogger::Log(LogLevel::Error, "FileSystem: Can't find TransformComponent with entity name", l_orphan.second.c_str(), "!");
			}
		}
	}

	return true;
}