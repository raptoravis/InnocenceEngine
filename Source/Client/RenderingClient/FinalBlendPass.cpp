#include "FinalBlendPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "BillboardPass.h"
#include "DebugPass.h"
#include "LuminanceHistogramPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace FinalBlendPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool FinalBlendPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("FinalBlendPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "finalBlendPass.frag/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("FinalBlendPass/");

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("FinalBlendPass/");

	return true;
}

bool FinalBlendPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);
	g_pModuleManager->getRenderingServer()->SetUserPipelineOutput(m_RPDC);

	return true;
}

bool FinalBlendPass::Render(IResourceBinder* input)
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 4, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, input, 0, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BillboardPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 1, 1);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, DebugPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 2, 2);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, LuminanceHistogramPass::GetAverageLuminance()->m_ResourceBinder, 3, 3, Accessibility::ReadOnly);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Square);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, input, 0, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BillboardPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 1, 1);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, DebugPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 2, 2);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, LuminanceHistogramPass::GetAverageLuminance()->m_ResourceBinder, 3, 3, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool FinalBlendPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* FinalBlendPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* FinalBlendPass::getSPC()
{
	return m_SPC;
}