#include "PreTAAPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "LightPass.h"
#include "SkyPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace PreTAAPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool PreTAAPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("PreTAAPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "preTAAPass.frag/";

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("PreTAAPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(3);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("PreTAAPass/");

	return true;
}

bool PreTAAPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool PreTAAPass::PrepareCommandList()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 2, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, LightPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 0, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, SkyPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 1, 1);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Square);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, LightPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 0, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, SkyPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 1, 1);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool PreTAAPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* PreTAAPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* PreTAAPass::GetSPC()
{
	return m_SPC;
}