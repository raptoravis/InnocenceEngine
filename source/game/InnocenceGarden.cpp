#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}

InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::setup()
{
	rootActor.addChildActor(&m_playCharacter);

	rootActor.addChildActor(&skyboxActor);

	rootActor.addChildActor(&directionalLightActor);

	rootActor.addChildActor(&landscapeActor);
	rootActor.addChildActor(&pawnActor1);
	rootActor.addChildActor(&pawnActor2);

	m_playCharacter.getTransform()->setPos(vec3(0.0, 2.0, 5.0));
	m_cameraComponents.emplace_back(&m_playCharacter.getCameraComponent());
	m_inputComponents.emplace_back(&m_playCharacter.getInputComponent());

	skyboxComponent.m_visiblilityType = visiblilityType::SKYBOX;
	skyboxComponent.m_meshType = meshType::CUBE;
	skyboxComponent.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	skyboxComponent.m_textureFileNameMap.emplace(textureFileNamePair(textureType::EQUIRETANGULAR, "ibl/Brooklyn_Bridge_Planks_2k.hdr"));
	skyboxActor.addChildComponent(&skyboxComponent);
	m_visibleComponents.emplace_back(&skyboxComponent);

	directionalLightComponent.setColor(vec3(1.0, 1.0, 1.0));
	directionalLightComponent.setlightType(lightType::DIRECTIONAL);
	directionalLightComponent.setDirection(vec3(0.0, 0.0, -0.85));
	directionalLightActor.addChildComponent(&directionalLightComponent);
	m_lightComponents.emplace_back(&directionalLightComponent);

	landscapeStaticMeshComponent.m_visiblilityType = visiblilityType::STATIC_MESH;
	landscapeStaticMeshComponent.m_meshType = meshType::CUBE;
	landscapeActor.addChildComponent(&landscapeStaticMeshComponent);
	landscapeActor.getTransform()->setScale(vec3(20.0, 20.0, 0.1f));
	landscapeActor.getTransform()->rotate(vec3(1.0, 0.0, 0.0), 90.0);
	landscapeActor.getTransform()->setPos(vec3(0.0, 0.0, 0.0));
	m_visibleComponents.emplace_back(&landscapeStaticMeshComponent);

	pawnMeshComponent1.m_visiblilityType = visiblilityType::STATIC_MESH;
	pawnMeshComponent1.m_meshType = meshType::CUSTOM;
	pawnActor1.addChildComponent(&pawnMeshComponent1);
	pawnActor1.getTransform()->setScale(vec3(0.02, 0.02, 0.02));
	pawnActor1.getTransform()->setPos(vec3(0.0, 0.2, -1.5));
	m_visibleComponents.emplace_back(&pawnMeshComponent1);

	pawnMeshComponent2.m_visiblilityType = visiblilityType::STATIC_MESH;
	pawnMeshComponent2.m_meshType = meshType::CUSTOM;
	pawnMeshComponent2.m_modelFileName = "lantern/lantern.obj";
	pawnMeshComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "lantern/lantern_Normal_OpenGL.jpg"));
	pawnMeshComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "lantern/lantern_Base_Color.jpg"));
	pawnMeshComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "lantern/lantern_Metallic.jpg"));
	pawnMeshComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "lantern/lantern_Roughness.jpg"));
	pawnMeshComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "lantern/lantern_Mixed_AO.jpg"));
	pawnActor2.addChildComponent(&pawnMeshComponent2);
	pawnActor2.getTransform()->setScale(vec3(0.02, 0.02, 0.02));
	pawnActor2.getTransform()->setPos(vec3(0.0, 0.2, 3.5));
	m_visibleComponents.emplace_back(&pawnMeshComponent2);

	setupLights();
	setupSpheres();

	rootActor.setup();
}

void InnocenceGarden::initialize()
{	
	rootActor.initialize();
}

void InnocenceGarden::update()
{
	temp += 0.02f;
	updateLights(temp);
	updateSpheres(temp);
	rootActor.update();
}

void InnocenceGarden::shutdown()
{	
	rootActor.shutdown();
}

const objectStatus & InnocenceGarden::getStatus() const
{
	return m_objectStatus;
}


std::string InnocenceGarden::getGameName() const
{
	return std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
}

void InnocenceGarden::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}

std::vector<CameraComponent*>& InnocenceGarden::getCameraComponents()
{
	return m_cameraComponents;
}

std::vector<InputComponent*>& InnocenceGarden::getInputComponents()
{
	return m_inputComponents;
}

std::vector<LightComponent*>& InnocenceGarden::getLightComponents()
{
	return m_lightComponents;
}

std::vector<VisibleComponent*>& InnocenceGarden::getVisibleComponents()
{
	return m_visibleComponents;
}

void InnocenceGarden::setupSpheres()
{
	unsigned int sphereMatrixDim = 8;
	double sphereBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < sphereMatrixDim * sphereMatrixDim; i++)
	{
		sphereComponents.emplace_back();
		sphereActors.emplace_back();
	}
	for (auto i = (unsigned int)0; i < sphereComponents.size(); i++)
	{
		sphereComponents[i].m_visiblilityType = visiblilityType::STATIC_MESH;
		sphereComponents[i].m_meshType = meshType::SPHERE;
		sphereComponents[i].m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
		sphereComponents[i].m_useTexture = true;
		rootActor.addChildActor(&sphereActors[i]);
		sphereActors[i].addChildComponent(&sphereComponents[i]);
		m_visibleComponents.emplace_back(&sphereComponents[i]);
	}
	for (auto i = (unsigned int)0; i < sphereComponents.size(); i += 4)
	{
		////Copper
		sphereComponents[i].m_albedo = vec3(0.95, 0.64, 0.54);
		sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/rustediron2_normal.png"));
		sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/rustediron2_basecolor.png"));
		sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/rustediron2_metallic.png"));
		sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/rustediron2_roughness.png"));
		////Gold
		sphereComponents[i + 1].m_albedo = vec3(1.00, 0.71, 0.29);
		sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/bamboo-wood-semigloss-normal.png"));
		sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/bamboo-wood-semigloss-albedo.png"));
		sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/bamboo-wood-semigloss-metal.png"));
		sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "PBS/bamboo-wood-semigloss-ao.png"));
		////Iron
		sphereComponents[i + 2].m_albedo = vec3(0.56, 0.57, 0.58);
		sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/greasy-metal-pan1-normal.png"));
		sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/greasy-metal-pan1-albedo.png"));
		sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/greasy-metal-pan1-metal.png"));
		sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/greasy-metal-pan1-roughness.png"));
		////Silver
		sphereComponents[i + 3].m_albedo = vec3(0.95, 0.93, 0.88);
		sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/roughrock1-normal.png"));
		sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/roughrock1-albedo.png"));
		sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/roughrock1-metalness.png"));
		sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/roughrock1-roughness.png"));
		sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "PBS/roughrock1-ao.png"));
	}
	for (auto i = (unsigned int)0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			sphereActors[i * sphereMatrixDim + j].getTransform()->setPos(vec3((-(sphereMatrixDim - 1.0) * sphereBreadthInterval / 2.0) + (i * sphereBreadthInterval), 2.0, (j * sphereBreadthInterval) - 2.0 * (sphereMatrixDim - 1)));
			sphereComponents[i * sphereMatrixDim + j].m_MRA = vec3((double)(i) / (double)(sphereMatrixDim), (double)(j) / (double)(sphereMatrixDim), 1.0);
		}
	}	
}

void InnocenceGarden::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	double pointLightBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < pointLightMatrixDim * pointLightMatrixDim; i++)
	{
		pointLightComponents.emplace_back();
		pointLightActors.emplace_back();
	}
	for (auto i = (unsigned int)0; i < pointLightComponents.size(); i++)
	{
		rootActor.addChildActor(&pointLightActors[i]);
		pointLightActors[i].addChildComponent(&pointLightComponents[i]);
		m_lightComponents.emplace_back(&pointLightComponents[i]);
	}
	for (auto i = (unsigned int)0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			pointLightActors[i * pointLightMatrixDim + j].getTransform()->setPos(vec3((-(pointLightMatrixDim - 1.0) * pointLightBreadthInterval / 2.0) + (i * pointLightBreadthInterval), 2.0 + (j * pointLightBreadthInterval), 4.0));
		}
	}
}

void InnocenceGarden::updateLights(double seed)
{
	directionalLightComponent.setDirection(vec3(0.0, cos(seed), (sin(seed) + 1.0) / 2.0));
	for (auto i = (unsigned int)0; i < pointLightComponents.size(); i+=4)
	{
		pointLightComponents[i].setColor(vec3((sin(seed + i) + 1.0) * 10.0 / 2.0, 0.2f * 10.0, 0.4f * 10.0));
		pointLightComponents[i + 1].setColor(vec3(0.2f * 10.0, (sin(seed + i) + 1.0) * 10.0 / 2.0, 0.4f * 10.0));
		pointLightComponents[i + 2].setColor(vec3(0.2f * 10.0, 0.4f * 10.0, (sin(seed + i) + 1.0) * 10.0 / 2.0));
		pointLightComponents[i + 3].setColor(vec3((sin(seed + i * 2.0 ) + 1.0) * 10.0 / 2.0, (sin(seed + i* 3.0) + 1.0) * 10.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 10.0 / 2.0));
	}
}

void InnocenceGarden::updateSpheres(double seed)
{
	pawnActor2.getTransform()->rotate(vec3(0.0, 1.0, 0.0), 0.05);
	for (auto i = (unsigned int)0; i < sphereActors.size(); i++)
	{
		sphereActors[i].getTransform()->rotate(vec3(0.0, 1.0, 0.0), 0.01 * i);
		sphereActors[i].getTransform()->setPos(sphereActors[i].getTransform()->getPos() + vec3(cos(seed) * 0.01, 0.0, 0.0));
	}
}
