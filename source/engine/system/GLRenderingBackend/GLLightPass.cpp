#include "GLLightPass.h"
#include "GLOpaquePass.h"
#include "GLSSAOBlurPass.h"
#include "GLEnvironmentRenderPass.h"
#include "GLShadowRenderPass.h"

#include "../../component/GLRenderingSystemComponent.h"

#include "GLRenderingSystemUtilities.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

using namespace GLRenderingSystemNS;

INNO_PRIVATE_SCOPE GLLightPass
{
	void initializeLightPassShaders();
	void bindLightPassUniformLocations(GLShaderProgramComponent* rhs);

	EntityID m_entityID;

	GLRenderPassComponent* m_GLRPC;

	GLShaderProgramComponent* m_GLSPC;

#ifdef CookTorrance
	ShaderFilePaths m_shaderFilePaths = { "GL//lightPassCookTorranceVertex.sf" , "", "GL//lightPassCookTorranceFragment.sf" };
#elif BlinnPhong
	ShaderFilePaths m_shaderFilePaths = { "GL//lightPassBlinnPhongVertex.sf" , "", "GL//lightPassBlinnPhongFragment.sf" };
#endif
	std::vector<std::string> m_textureUniformNames =
	{
		"uni_opaquePassRT0",
		"uni_opaquePassRT1",
		"uni_opaquePassRT2",
		"uni_SSAOBlurPassRT0",
		"uni_directionalLightShadowMap",
		"uni_brdfLUT",
		"uni_brdfMSLUT",
		"uni_irradianceMap",
		"uni_preFiltedMap"
	};

	std::vector<GLuint> m_uni_shadowSplitAreas;
	std::vector<GLuint> m_uni_dirLightProjs;
	std::vector<GLuint> m_uni_dirLightViews;

	GLuint m_uni_viewPos;
	GLuint m_uni_dirLight_direction;
	GLuint m_uni_dirLight_luminance;
	GLuint m_uni_dirLight_rot;

	std::vector<GLuint> m_uni_pointLights_position;
	std::vector<GLuint> m_uni_pointLights_attenuationRadius;
	std::vector<GLuint> m_uni_pointLights_luminance;

	std::vector<GLuint> m_uni_sphereLights_position;
	std::vector<GLuint> m_uni_sphereLights_sphereRadius;
	std::vector<GLuint> m_uni_sphereLights_luminance;

	GLuint m_uni_isEmissive;
}

void GLLightPass::initialize()
{
	m_entityID = InnoMath::createEntityID();

	m_GLRPC = addGLRenderPassComponent(1, GLRenderingSystemComponent::get().deferredPassFBDesc, GLRenderingSystemComponent::get().deferredPassTextureDesc);

	initializeLightPassShaders();
}

void GLLightPass::initializeLightPassShaders()
{
	// shader programs and shaders
	auto l_GLSPC = addGLShaderProgramComponent(m_entityID);

	initializeGLShaderProgramComponent(l_GLSPC, m_shaderFilePaths);

	bindLightPassUniformLocations(l_GLSPC);

	m_GLSPC = l_GLSPC;
}

void GLLightPass::bindLightPassUniformLocations(GLShaderProgramComponent* rhs)
{
	updateTextureUniformLocations(rhs->m_program, m_textureUniformNames);

	m_uni_shadowSplitAreas.reserve(4);
	m_uni_dirLightProjs.reserve(4);
	m_uni_dirLightViews.reserve(4);

	for (size_t i = 0; i < 4; i++)
	{
		m_uni_shadowSplitAreas.emplace_back(
			getUniformLocation(rhs->m_program, "uni_shadowSplitAreas[" + std::to_string(i) + "]")
		);
		m_uni_dirLightProjs.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightProjs[" + std::to_string(i) + "]")
		);
		m_uni_dirLightViews.emplace_back(
			getUniformLocation(rhs->m_program, "uni_dirLightViews[" + std::to_string(i) + "]")
		);
	}

	m_uni_viewPos = getUniformLocation(
		rhs->m_program,
		"uni_viewPos");
	m_uni_dirLight_direction = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.direction");
	m_uni_dirLight_luminance = getUniformLocation(
		rhs->m_program,
		"uni_dirLight.luminance");

	m_uni_pointLights_position.reserve(GLRenderingSystemComponent::get().m_maxPointLights);
	m_uni_pointLights_attenuationRadius.reserve(GLRenderingSystemComponent::get().m_maxPointLights);
	m_uni_pointLights_luminance.reserve(GLRenderingSystemComponent::get().m_maxPointLights);

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_maxPointLights; i++)
	{
		m_uni_pointLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].position")
		);
		m_uni_pointLights_attenuationRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].attenuationRadius")
		);
		m_uni_pointLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_pointLights[" + std::to_string(i) + "].luminance")
		);
	}

	m_uni_sphereLights_position.reserve(GLRenderingSystemComponent::get().m_maxSphereLights);
	m_uni_sphereLights_sphereRadius.reserve(GLRenderingSystemComponent::get().m_maxSphereLights);
	m_uni_sphereLights_luminance.reserve(GLRenderingSystemComponent::get().m_maxSphereLights);

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_maxSphereLights; i++)
	{
		m_uni_sphereLights_position.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].position")
		);
		m_uni_sphereLights_sphereRadius.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].sphereRadius")
		);
		m_uni_sphereLights_luminance.emplace_back(
			getUniformLocation(rhs->m_program, "uni_sphereLights[" + std::to_string(i) + "].luminance")
		);
	}

	m_uni_isEmissive = getUniformLocation(
		rhs->m_program,
		"uni_isEmissive");
}

void GLLightPass::update()
{
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glDisable(GL_CULL_FACE);

	// bind to framebuffer
	activateRenderPass(m_GLRPC);

	// 1. opaque objects
	// copy stencil buffer of opaque objects from G-Pass
	copyStencilBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	activateShaderProgram(m_GLSPC);

#ifdef CookTorrance
	// Cook-Torrance
	// world space position + metallic
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[0],
		0);
	// world space normal + roughness
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[1],
		1);
	// albedo + ambient occlusion
	activateTexture(
		GLOpaquePass::getGLRPC()->m_GLTDCs[2],
		2);
	// SSAO
	activateTexture(
		GLSSAOBlurPass::getGLRPC()->m_GLTDCs[0],
		3);
	// shadow map
	activateTexture(
		GLShadowRenderPass::getGLRPC(0)->m_GLTDCs[0],
		4);
	// BRDF look-up table 1
	activateTexture(
		GLEnvironmentRenderPass::getBRDFSplitSumLUT(),
		5);
	// BRDF look-up table 2
	activateTexture(
		GLEnvironmentRenderPass::getBRDFMSAverageLUT(),
		6);
	// Irradiance env cubemap
	activateTexture(
		GLEnvironmentRenderPass::getConvPassGLTDC(),
		7);
	// pre-filtered specular env cubemap
	activateTexture(
		GLEnvironmentRenderPass::getPreFilterPassGLTDC(),
		8);
#endif

	updateUniform(
		m_uni_isEmissive,
		false);

	auto l_cameraDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCameraDataPack();
	auto l_sunDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getSunDataPack();
	auto l_CSMDataPack = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getCSMDataPack();

	updateUniform(
		m_uni_viewPos,
		l_cameraDataPack.globalPos.x, l_cameraDataPack.globalPos.y, l_cameraDataPack.globalPos.z);

	updateUniform(
		m_uni_dirLight_direction,
		l_sunDataPack.dir.x, l_sunDataPack.dir.y, l_sunDataPack.dir.z);
	updateUniform(
		m_uni_dirLight_luminance,
		l_sunDataPack.luminance.x, l_sunDataPack.luminance.y, l_sunDataPack.luminance.z);

	for (size_t j = 0; j < 4; j++)
	{
		updateUniform(
			m_uni_shadowSplitAreas[j],
			l_CSMDataPack[j].splitCorners.x, l_CSMDataPack[j].splitCorners.y, l_CSMDataPack[j].splitCorners.z, l_CSMDataPack[j].splitCorners.w);

		updateUniform(
			m_uni_dirLightProjs[j],
			l_CSMDataPack[j].p);
		updateUniform(
			m_uni_dirLightViews[j],
			l_CSMDataPack[j].v);
	}

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_PointLightDatas.size(); i++)
	{
		auto l_pos = GLRenderingSystemComponent::get().m_PointLightDatas[i].pos;
		auto l_luminance = GLRenderingSystemComponent::get().m_PointLightDatas[i].luminance;
		auto l_attenuationRadius = GLRenderingSystemComponent::get().m_PointLightDatas[i].attenuationRadius;

		updateUniform(
			m_uni_pointLights_position[i],
			l_pos.x, l_pos.y, l_pos.z);
		updateUniform(
			m_uni_pointLights_attenuationRadius[i],
			l_attenuationRadius);
		updateUniform(
			m_uni_pointLights_luminance[i],
			l_luminance.x, l_luminance.y, l_luminance.z);
	}

	for (size_t i = 0; i < GLRenderingSystemComponent::get().m_SphereLightDatas.size(); i++)
	{
		auto l_pos = GLRenderingSystemComponent::get().m_SphereLightDatas[i].pos;
		auto l_luminance = GLRenderingSystemComponent::get().m_SphereLightDatas[i].luminance;
		auto l_sphereRadius = GLRenderingSystemComponent::get().m_SphereLightDatas[i].sphereRadius;

		updateUniform(
			m_uni_sphereLights_position[i],
			l_pos.x, l_pos.y, l_pos.z);
		updateUniform(
			m_uni_sphereLights_sphereRadius[i],
			l_sphereRadius);
		updateUniform(
			m_uni_sphereLights_luminance[i],
			l_luminance.x, l_luminance.y, l_luminance.z);
	}

	// draw light pass rectangle
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	// 2. draw emissive objects
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x02, 0xFF);
	glStencilMask(0x00);

	glClear(GL_STENCIL_BUFFER_BIT);

	// copy stencil buffer of emmisive objects from G-Pass
	copyStencilBuffer(GLOpaquePass::getGLRPC(), m_GLRPC);

	updateUniform(
		m_uni_isEmissive,
		true);

	// draw light pass rectangle
	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	drawMesh(l_MDC);

	glDisable(GL_STENCIL_TEST);
}

bool GLLightPass::resize()
{
	resizeGLRenderPassComponent(m_GLRPC, GLRenderingSystemComponent::get().deferredPassFBDesc);

	return true;
}

bool GLLightPass::reloadShader()
{
	deleteShaderProgram(m_GLSPC);

	initializeGLShaderProgramComponent(m_GLSPC, m_shaderFilePaths);

	bindLightPassUniformLocations(m_GLSPC);

	return true;
}

GLRenderPassComponent * GLLightPass::getGLRPC()
{
	return m_GLRPC;
}