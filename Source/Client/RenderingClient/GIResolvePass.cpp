#include "GIResolvePass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIDataLoader.h"
#include "SunShadowPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace GIResolvePass
{
	bool setupSky();
	bool setupSurfels();
	bool setupBricks();
	bool setupProbes();
	bool setupIrradianceVolume();

	bool generateSkyRadiance();
	bool litSurfels();
	bool litBricks();
	bool litProbes();
	bool generateIrradianceVolume();

	RenderPassDataComponent* m_skyRPDC;
	ShaderProgramComponent* m_skySPC;

	RenderPassDataComponent* m_skyConvRPDC;
	ShaderProgramComponent* m_skyConvSPC;

	RenderPassDataComponent* m_surfelRPDC = 0;
	ShaderProgramComponent* m_surfelSPC = 0;

	RenderPassDataComponent* m_brickRPDC = 0;
	ShaderProgramComponent* m_brickSPC = 0;

	RenderPassDataComponent* m_probeRPDC = 0;
	ShaderProgramComponent* m_probeSPC = 0;

	RenderPassDataComponent* m_irradianceVolumeRPDC = 0;
	ShaderProgramComponent* m_irradianceVolumeSPC = 0;
	SamplerDataComponent* m_irradianceVolumeSDC = 0;

	GPUBufferDataComponent* m_skyConvGBDC = 0;

	GPUBufferDataComponent* m_surfelGBDC = 0;
	GPUBufferDataComponent* m_surfelIrradianceGBDC = 0;
	GPUBufferDataComponent* m_brickGBDC = 0;
	GPUBufferDataComponent* m_brickIrradianceGBDC = 0;
	GPUBufferDataComponent* m_brickFactorGBDC = 0;
	GPUBufferDataComponent* m_probeGBDC = 0;
	TextureDataComponent* m_probeVolume = 0;
	TextureDataComponent* m_irradianceVolume = 0;

	const unsigned int m_skyCubemapSize = 32;
	Vec4 m_minProbePos;
	Vec4 m_irradianceVolumePosOffset;
	Vec4 m_irradianceVolumeRange;

	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_reloadGIData;
	bool m_needToReloadGIData = false;

	bool m_GIDataLoaded = false;
}

bool GIResolvePass::InitializeGPUBuffers()
{
	auto l_surfels = GIDataLoader::GetSurfels();

	if (l_surfels.size())
	{
		auto l_GIResolvePassInitializeGPUBuffersTask = g_pModuleManager->getTaskSystem()->submit("GIResolvePassInitializeGPUBuffersTask", 2, nullptr,
			[&]() {
			m_surfelGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SurfelGPUBuffer/");
			m_surfelGBDC->m_CPUAccessibility = Accessibility::Immutable;
			m_surfelGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
			m_surfelGBDC->m_ElementCount = l_surfels.size();
			m_surfelGBDC->m_ElementSize = sizeof(Surfel);
			m_surfelGBDC->m_BindingPoint = 0;
			m_surfelGBDC->m_InitialData = &l_surfels[0];

			g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelGBDC);

			m_surfelIrradianceGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SurfelIrradianceGPUBuffer/");
			m_surfelIrradianceGBDC->m_CPUAccessibility = Accessibility::Immutable;
			m_surfelIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
			m_surfelIrradianceGBDC->m_ElementCount = l_surfels.size();
			m_surfelIrradianceGBDC->m_ElementSize = sizeof(Vec4);
			m_surfelIrradianceGBDC->m_BindingPoint = 1;

			g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelIrradianceGBDC);

			auto l_bricks = GIDataLoader::GetBricks();

			auto l_bricksCount = l_bricks.size();

			auto l_min = InnoMath::maxVec4<float>;
			auto l_max = InnoMath::minVec4<float>;

			for (size_t i = 0; i < l_bricksCount; i++)
			{
				l_min = InnoMath::elementWiseMin(l_min, l_bricks[i].boundBox.m_boundMin);
				l_max = InnoMath::elementWiseMax(l_max, l_bricks[i].boundBox.m_boundMax);
			}
			m_irradianceVolumeRange = l_max - l_min;
			m_irradianceVolumePosOffset = l_min;

			std::vector<unsigned int> l_brickGPUData;

			l_brickGPUData.resize(l_bricks.size() * 2);
			for (size_t i = 0; i < l_bricks.size(); i++)
			{
				l_brickGPUData[2 * i] = l_bricks[i].surfelRangeBegin;
				l_brickGPUData[2 * i + 1] = l_bricks[i].surfelRangeEnd;
			}

			m_brickGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BrickGPUBuffer/");
			m_brickGBDC->m_CPUAccessibility = Accessibility::Immutable;
			m_brickGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
			m_brickGBDC->m_ElementCount = l_bricks.size();
			m_brickGBDC->m_ElementSize = sizeof(unsigned int) * 2;
			m_brickGBDC->m_BindingPoint = 0;
			m_brickGBDC->m_InitialData = &l_brickGPUData[0];

			g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickGBDC);

			m_brickIrradianceGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BrickIrradianceGPUBuffer/");
			m_brickIrradianceGBDC->m_CPUAccessibility = Accessibility::Immutable;
			m_brickIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
			m_brickIrradianceGBDC->m_ElementCount = l_bricks.size();
			m_brickIrradianceGBDC->m_ElementSize = sizeof(Vec4);
			m_brickIrradianceGBDC->m_BindingPoint = 1;

			g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickIrradianceGBDC);

			auto l_brickFactors = GIDataLoader::GetBrickFactors();

			m_brickFactorGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("BrickFactorGPUBuffer/");
			m_brickFactorGBDC->m_CPUAccessibility = Accessibility::Immutable;
			m_brickFactorGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
			m_brickFactorGBDC->m_ElementCount = l_brickFactors.size();
			m_brickFactorGBDC->m_ElementSize = sizeof(BrickFactor);
			m_brickFactorGBDC->m_BindingPoint = 2;
			m_brickFactorGBDC->m_InitialData = &l_brickFactors[0];

			g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickFactorGBDC);

			std::vector<Probe> l_probes = GIDataLoader::GetProbes();

			TVec4<unsigned int> l_probeIndex;
			float l_minPos;

			std::sort(l_probes.begin(), l_probes.end(), [&](Probe A, Probe B)
			{
				return A.pos.x < B.pos.x;
			});

			l_minPos = l_probes[0].pos.x;
			m_minProbePos.x = l_minPos;

			for (size_t i = 0; i < l_probes.size(); i++)
			{
				auto l_probePosWS = l_probes[i].pos;

				if ((l_probePosWS.x - l_minPos) > epsilon<float, 2>)
				{
					l_minPos = l_probePosWS.x;
					l_probeIndex.x++;
				}

				l_probes[i].pos.x = (float)l_probeIndex.x;
			}

			std::sort(l_probes.begin(), l_probes.end(), [&](Probe A, Probe B)
			{
				return A.pos.y < B.pos.y;
			});

			l_minPos = l_probes[0].pos.y;
			m_minProbePos.y = l_minPos;

			for (size_t i = 0; i < l_probes.size(); i++)
			{
				auto l_probePosWS = l_probes[i].pos;

				if ((l_probePosWS.y - l_minPos) > epsilon<float, 2>)
				{
					l_minPos = l_probePosWS.y;
					l_probeIndex.y++;
				}

				l_probes[i].pos.y = (float)l_probeIndex.y;
			}

			std::sort(l_probes.begin(), l_probes.end(), [&](Probe A, Probe B)
			{
				return A.pos.z < B.pos.z;
			});

			l_minPos = l_probes[0].pos.z;
			m_minProbePos.z = l_minPos;

			for (size_t i = 0; i < l_probes.size(); i++)
			{
				auto l_probePosWS = l_probes[i].pos;

				if ((l_probePosWS.z - l_minPos) > epsilon<float, 2>)
				{
					l_minPos = l_probePosWS.z;
					l_probeIndex.z++;
				}

				l_probes[i].pos.z = (float)l_probeIndex.z;
			}

			m_minProbePos.w = 1.0f;

			m_probeGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("ProbeGPUBuffer/");
			m_probeGBDC->m_CPUAccessibility = Accessibility::Immutable;
			m_probeGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
			m_probeGBDC->m_ElementCount = l_probes.size();
			m_probeGBDC->m_ElementSize = sizeof(Probe);
			m_probeGBDC->m_BindingPoint = 3;
			m_probeGBDC->m_InitialData = &l_probes[0];

			g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_probeGBDC);

			auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

			m_probeVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("ProbeVolume/");
			m_probeVolume->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

			m_probeVolume->m_textureDataDesc.Width = (unsigned int)l_probeIndex.x + 1;
			m_probeVolume->m_textureDataDesc.Height = (unsigned int)l_probeIndex.y + 1;
			m_probeVolume->m_textureDataDesc.DepthOrArraySize = ((unsigned int)l_probeIndex.z + 1) * 6;
			m_probeVolume->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
			m_probeVolume->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler3D;
			m_probeVolume->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
			m_probeVolume->m_textureDataDesc.MinFilterMethod = TextureFilterMethod::Linear;
			m_probeVolume->m_textureDataDesc.MagFilterMethod = TextureFilterMethod::Linear;

			g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_probeVolume);

			m_irradianceVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("IrradianceVolume/");
			m_irradianceVolume->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

			m_irradianceVolume->m_textureDataDesc.Width = 64;
			m_irradianceVolume->m_textureDataDesc.Height = 32;
			m_irradianceVolume->m_textureDataDesc.DepthOrArraySize = 64 * 6;
			m_irradianceVolume->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
			m_irradianceVolume->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler3D;
			m_irradianceVolume->m_textureDataDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
			m_irradianceVolume->m_textureDataDesc.MinFilterMethod = TextureFilterMethod::Linear;
			m_irradianceVolume->m_textureDataDesc.MagFilterMethod = TextureFilterMethod::Linear;

			g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_irradianceVolume);

			m_GIDataLoaded = true;
		});

		l_GIResolvePassInitializeGPUBuffersTask->Wait();
	}

	return true;
}

bool GIResolvePass::DeleteGPUBuffers()
{
	auto l_GIResolvePassDeleteGPUBuffersTask = g_pModuleManager->getTaskSystem()->submit("GIResolvePassDeleteGPUBuffersTask", 2, nullptr,
		[&]() {
		if (m_surfelGBDC)
		{
			g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelGBDC);
		}
		if (m_surfelIrradianceGBDC)
		{
			g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelIrradianceGBDC);
		}
		if (m_brickGBDC)
		{
			g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickGBDC);
		}
		if (m_brickIrradianceGBDC)
		{
			g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickIrradianceGBDC);
		}
		if (m_brickFactorGBDC)
		{
			g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickFactorGBDC);
		}
		if (m_probeGBDC)
		{
			g_pModuleManager->getRenderingServer()->DeleteGPUBufferDataComponent(m_probeGBDC);
		}
		if (m_probeVolume)
		{
			g_pModuleManager->getRenderingServer()->DeleteTextureDataComponent(m_probeVolume);
		}

		m_GIDataLoaded = false;
	});

	l_GIResolvePassDeleteGPUBuffersTask->Wait();

	return true;
}

bool GIResolvePass::Setup()
{
	f_reloadGIData = [&]() { m_needToReloadGIData = true; };
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_reloadGIData });

	setupSky();
	setupSurfels();
	setupBricks();
	setupProbes();
	setupIrradianceVolume();

	f_sceneLoadingFinishCallback = []() { InitializeGPUBuffers(); };
	f_sceneLoadingStartCallback = []() { DeleteGPUBuffers(); };

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	return true;
}

bool GIResolvePass::Initialize()
{
	return true;
}

bool GIResolvePass::setupSky()
{
	m_skySPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIResolveSkyPass/");

	m_skySPC->m_ShaderFilePaths.m_VSPath = "GIResolveSkyPass.vert/";
	m_skySPC->m_ShaderFilePaths.m_GSPath = "GIResolveSkyPass.geom/";
	m_skySPC->m_ShaderFilePaths.m_PSPath = "GIResolveSkyPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_skySPC);

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_IsOffScreen = true;

	m_skyRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIResolveSkyPass/");

	m_skyRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_skyRPDC->m_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::SamplerCubemap;
	m_skyRPDC->m_RenderPassDesc.m_RenderTargetDesc.Width = m_skyCubemapSize;
	m_skyRPDC->m_RenderPassDesc.m_RenderTargetDesc.Height = m_skyCubemapSize;

	m_skyRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	m_skyRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	m_skyRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::Always;
	m_skyRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = m_skyCubemapSize;
	m_skyRPDC->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = m_skyCubemapSize;

	m_skyRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_skyRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_skyRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_skyRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 10;

	m_skyRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_skyRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_skyRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 3;

	m_skyRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_skyRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_skyRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 7;

	m_skyRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_skyRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_skyRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 11;

	m_skyRPDC->m_ShaderProgram = m_skySPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_skyRPDC);

	////
	m_skyConvGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("SkyConvGPUBuffer/");
	m_skyConvGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_skyConvGBDC->m_ElementCount = 6;
	m_skyConvGBDC->m_ElementSize = sizeof(Vec4);
	m_skyConvGBDC->m_BindingPoint = 4;

	return true;
}

bool GIResolvePass::setupSurfels()
{
	m_surfelSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIResolveSurfelPass/");
	m_surfelSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSurfelPass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_surfelSPC);

	m_surfelRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIResolveSurfelPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_surfelRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_surfelRPDC->m_ResourceBinderLayoutDescs.resize(8);
	m_surfelRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 3;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 8;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 6;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 0;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[6].m_IsRanged = true;

	m_surfelRPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[7].m_GlobalSlot = 7;
	m_surfelRPDC->m_ResourceBinderLayoutDescs[7].m_LocalSlot = 11;

	m_surfelRPDC->m_ShaderProgram = m_surfelSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_surfelRPDC);

	return true;
}

bool GIResolvePass::setupBricks()
{
	m_brickSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIResolveBrickPass/");
	m_brickSPC->m_ShaderFilePaths.m_CSPath = "GIResolveBrickPass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_brickSPC);

	m_brickRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIResolveBrickPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_brickRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_brickRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_brickRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_brickRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 8;

	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_brickRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;

	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_brickRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 1;

	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_brickRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 2;

	m_brickRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_brickRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_brickRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 11;

	m_brickRPDC->m_ShaderProgram = m_brickSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_brickRPDC);

	return true;
}

bool GIResolvePass::setupProbes()
{
	m_probeSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIResolveProbePass/");
	m_probeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveProbePass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_probeSPC);

	m_probeRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIResolveProbePass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_probeRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_probeRPDC->m_ResourceBinderLayoutDescs.resize(7);
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_probeRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 8;

	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_probeRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 7;

	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_probeRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 0;

	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_probeRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 1;

	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_probeRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 2;

	m_probeRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_probeRPDC->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_probeRPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 3;
	m_probeRPDC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_probeRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_probeRPDC->m_ResourceBinderLayoutDescs[6].m_GlobalSlot = 6;
	m_probeRPDC->m_ResourceBinderLayoutDescs[6].m_LocalSlot = 11;

	m_probeRPDC->m_ShaderProgram = m_probeSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_probeRPDC);

	return true;
}

bool GIResolvePass::setupIrradianceVolume()
{
	m_irradianceVolumeSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveIrradianceVolumePass.comp/";
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_irradianceVolumeSPC);

	m_irradianceVolumeRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("GIResolveIrradianceVolumePass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irradianceVolumeRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs.resize(6);
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 8;

	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 7;

	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 11;

	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadOnly;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 0;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 0;
	m_irradianceVolumeRPDC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_irradianceVolumeRPDC->m_ShaderProgram = m_irradianceVolumeSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_irradianceVolumeRPDC);

	m_irradianceVolumeSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_irradianceVolumeSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;
	m_irradianceVolumeSDC->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Border;
	m_irradianceVolumeSDC->m_SamplerDesc.m_BorderColor[3] = 1.0f;

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_irradianceVolumeSDC);

	return true;
}

bool GIResolvePass::generateSkyRadiance()
{
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);
	auto l_GICameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GICamera);
	auto l_GISkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GISky);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_skyRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_skyRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_skyRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_skyRPDC, ShaderStage::Geometry, l_GICameraGBDC->m_ResourceBinder, 0, 10, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_skyRPDC, ShaderStage::Pixel, l_SunGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_skyRPDC, ShaderStage::Pixel, l_SkyGBDC->m_ResourceBinder, 2, 7, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_skyRPDC, ShaderStage::Pixel, l_GISkyGBDC->m_ResourceBinder, 3, 11, Accessibility::ReadOnly);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_skyRPDC, l_mesh);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_skyRPDC);

	return true;
}

bool GIResolvePass::litSurfels()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_GISkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GISky);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = std::ceil((double)m_surfelGBDC->m_ElementCount / (double)(l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide));
	auto l_averangeThreadGroupsCountPerSide = (unsigned int)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (unsigned int)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsGPUData l_surfelLitWorkload;
	l_surfelLitWorkload.numThreadGroups = TVec4<unsigned int>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_surfelLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_surfelLitWorkload, 2, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_surfelRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_surfelRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_surfelRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_SunGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 2, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_CSMGBDC->m_ResourceBinder, 5, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, SunShadowPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 6, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, l_GISkyGBDC->m_ResourceBinder, 7, 11, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_surfelRPDC, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 4, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_surfelRPDC, ShaderStage::Compute, SunShadowPass::GetRPDC()->m_RenderTargetsResourceBinders[0], 6, 0);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_surfelRPDC);

	return true;
}

bool GIResolvePass::litBricks()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);
	auto l_GISkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GISky);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_brickGBDC->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (unsigned int)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (unsigned int)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (unsigned int)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (unsigned int)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsGPUData l_brickLitWorkload;
	l_brickLitWorkload.numThreadGroups = TVec4<unsigned int>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_brickLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_brickLitWorkload, 3, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_brickRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_brickRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_brickRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 3, 2, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_brickRPDC, ShaderStage::Compute, l_GISkyGBDC->m_ResourceBinder, 4, 11, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_brickRPDC, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_brickRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 3, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_brickRPDC);

	return true;
}

bool GIResolvePass::litProbes()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);
	auto l_GISkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GISky);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_probeGBDC->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (unsigned int)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (unsigned int)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (unsigned int)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (unsigned int)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsGPUData l_probeLitWorkload;
	l_probeLitWorkload.numThreadGroups = TVec4<unsigned int>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_probeLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_probeLitWorkload, 4, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_probeRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_probeRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_probeRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, l_SkyGBDC->m_ResourceBinder, 1, 7, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_probeGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickFactorGBDC->m_ResourceBinder, 3, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 4, 2, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_probeVolume->m_ResourceBinder, 5, 3, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_probeRPDC, ShaderStage::Compute, l_GISkyGBDC->m_ResourceBinder, 6, 11, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_probeRPDC, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_probeGBDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickFactorGBDC->m_ResourceBinder, 3, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_brickIrradianceGBDC->m_ResourceBinder, 4, 2, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_probeRPDC, ShaderStage::Compute, m_probeVolume->m_ResourceBinder, 5, 3, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_probeRPDC);

	return true;
}

bool GIResolvePass::generateIrradianceVolume()
{
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);
	auto l_GISkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GISky);

	auto l_numThreadsX = 64;
	auto l_numThreadsY = 32;
	auto l_numThreadsZ = 64;

	DispatchParamsGPUData l_irradianceVolumeLitWorkload;
	l_irradianceVolumeLitWorkload.numThreadGroups = TVec4<unsigned int>(8, 4, 8, 0);
	l_irradianceVolumeLitWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irradianceVolumeLitWorkload, 5, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_irradianceVolumeRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_irradianceVolumeRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_irradianceVolumeRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, m_irradianceVolumeSDC->m_ResourceBinder, 5, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, l_SkyGBDC->m_ResourceBinder, 1, 7, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, l_GISkyGBDC->m_ResourceBinder, 2, 11, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, m_probeVolume->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, m_irradianceVolume->m_ResourceBinder, 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_irradianceVolumeRPDC, 8, 4, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, m_probeVolume->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irradianceVolumeRPDC, ShaderStage::Compute, m_irradianceVolume->m_ResourceBinder, 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_irradianceVolumeRPDC);

	return true;
}

bool GIResolvePass::PrepareCommandList()
{
	if (m_needToReloadGIData)
	{
		GIDataLoader::ReloadGIData();
		m_needToReloadGIData = false;
		DeleteGPUBuffers();
		InitializeGPUBuffers();
	}

	if (m_GIDataLoaded)
	{
		auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);
		auto l_GICameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GICamera);
		auto l_GISkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GISky);
		auto l_cameraGPUData = g_pModuleManager->getRenderingFrontend()->getCameraGPUData();

		SkyGPUData l_SkyGPUData = g_pModuleManager->getRenderingFrontend()->getSkyGPUData();

		// GI sky small cubemap
		l_SkyGPUData.viewportSize.z = (float)m_skyCubemapSize;
		l_SkyGPUData.viewportSize.w = (float)m_skyCubemapSize;
		l_SkyGPUData.posWSNormalizer = m_irradianceVolumeRange;

		GICameraGPUData l_GICameraGPUData;
		GISkyGPUData l_GISkyGPUData;

		l_GICameraGPUData.p = InnoMath::generatePerspectiveMatrix((90.0f / 180.0f) * PI<float>, 1.0f, l_cameraGPUData.zNear, l_cameraGPUData.zFar);
		l_GISkyGPUData.p_inv = l_GICameraGPUData.p.inverse();

		auto l_rPX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
		auto l_rNX = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(-1.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
		auto l_rPY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
		auto l_rNY = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f));
		auto l_rPZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, 1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));
		auto l_rNZ = InnoMath::lookAt(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(0.0f, 0.0f, -1.0f, 1.0f), Vec4(0.0f, -1.0f, 0.0f, 0.0f));

		std::vector<Mat4> l_v =
		{
			l_rPX, l_rNX, l_rPY, l_rNY, l_rPZ, l_rNZ
		};

		for (size_t i = 0; i < 6; i++)
		{
			l_GICameraGPUData.r[i] = l_v[i];
			l_GISkyGPUData.v_inv[i] = l_v[i].inverse();
		}

		l_GICameraGPUData.t = InnoMath::generateIdentityMatrix<float>();

		auto l_probeInfo = GIDataLoader::GetProbeInfo();
		l_GISkyGPUData.probeCount = l_probeInfo.probeCount;
		l_GISkyGPUData.probeInterval = l_probeInfo.probeInterval;
		l_GISkyGPUData.workload.x = (float)m_surfelGBDC->m_ElementCount;
		l_GISkyGPUData.workload.y = (float)m_brickGBDC->m_ElementCount;
		l_GISkyGPUData.workload.z = (float)m_probeGBDC->m_ElementCount;
		l_GISkyGPUData.irradianceVolumeOffset = m_irradianceVolumePosOffset;
		l_GISkyGPUData.probeCount.w = m_minProbePos.x;
		l_GISkyGPUData.probeInterval.w = m_minProbePos.y;
		l_GISkyGPUData.irradianceVolumeOffset.w = m_minProbePos.z;

		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_SkyGBDC, &l_SkyGPUData);
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_GICameraGBDC, &l_GICameraGPUData);
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_GISkyGBDC, &l_GISkyGPUData);

		generateSkyRadiance();
		litSurfels();
		litBricks();
		litProbes();
		generateIrradianceVolume();
	}

	return true;
}

bool GIResolvePass::ExecuteCommandList()
{
	if (m_GIDataLoaded)
	{
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_skyRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_skyRPDC);

		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_surfelRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_surfelRPDC);

		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_brickRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_brickRPDC);

		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_probeRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_probeRPDC);

		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_irradianceVolumeRPDC);

		g_pModuleManager->getRenderingServer()->WaitForFrame(m_irradianceVolumeRPDC);
	}

	return true;
}

bool GIResolvePass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_skyRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_surfelRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_brickRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_probeRPDC);

	return true;
}

RenderPassDataComponent * GIResolvePass::GetRPDC()
{
	return m_surfelRPDC;
}

ShaderProgramComponent * GIResolvePass::GetSPC()
{
	return m_surfelSPC;
}

IResourceBinder * GIResolvePass::GetProbeVolume()
{
	if (m_probeVolume)
	{
		return m_probeVolume->m_ResourceBinder;
	}
	else
	{
		return nullptr;
	}
}

IResourceBinder * GIResolvePass::GetIrradianceVolume()
{
	if (m_irradianceVolume)
	{
		return m_irradianceVolume->m_ResourceBinder;
	}
	else
	{
		return nullptr;
	}
}