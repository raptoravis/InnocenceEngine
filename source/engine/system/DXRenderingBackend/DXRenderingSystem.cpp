#include "DXRenderingSystem.h"

#include "DXGeometryRenderingPassUtilities.h"
#include "DXLightRenderingPassUtilities.h"
#include "DXFinalRenderingPassUtilities.h"

#include "../../component/DXWindowSystemComponent.h"
#include "../../component/DXRenderingSystemComponent.h"

#include "DXRenderingSystemUtilities.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXRenderingSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	bool setup(IRenderingFrontendSystem* renderingFrontend);
	bool update();
	bool terminate();

	bool initializeDefaultAssets();

	void prepareRenderingData();

	static DXWindowSystemComponent* g_DXWindowSystemComponent;
	static DXRenderingSystemComponent* g_DXRenderingSystemComponent;

	bool createPhysicalDevices();
	bool createSwapChain();
	bool createBackBuffer();
	bool createRasterizer();

	IRenderingFrontendSystem* m_renderingFrontendSystem;

	std::vector<MeshDataPack> m_meshDataPack;
}

bool DXRenderingSystemNS::createPhysicalDevices()
{
	HRESULT result;
	IDXGIFactory* m_factory;

	DXGI_ADAPTER_DESC adapterDesc;
	IDXGIAdapter* m_adapter;
	IDXGIOutput* m_adapterOutput;

	unsigned int numModes;
	unsigned long long stringLength;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_factory);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create DXGI factory!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = m_factory->EnumAdapters(0, &m_adapter);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create video card adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = m_adapter->EnumOutputs(0, &m_adapterOutput);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create monitor adapter!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't get DXGI_FORMAT_R8G8B8A8_UNORM fitted monitor!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	std::vector<DXGI_MODE_DESC> displayModeList(numModes);

	// Now fill the display mode list structures.
	result = m_adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, &displayModeList[0]);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't fill the display mode list structures!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	for (unsigned int i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == l_screenResolution.x
			&&
			displayModeList[i].Height == l_screenResolution.y
			)
		{
			g_DXRenderingSystemComponent->m_refreshRate.x = displayModeList[i].RefreshRate.Numerator;
			g_DXRenderingSystemComponent->m_refreshRate.y = displayModeList[i].RefreshRate.Denominator;
		}
	}

	// Get the adapter (video card) description.
	result = m_adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't get the video card adapter description!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	g_DXRenderingSystemComponent->m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	if (wcstombs_s(&stringLength, g_DXRenderingSystemComponent->m_videoCardDescription, 128, adapterDesc.Description, 128) != 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't convert the name of the video card to a character array!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Release the display mode list.
	// displayModeList.clear();

	// Release the adapter output.
	m_adapterOutput->Release();
	m_adapterOutput = 0;

	// Release the adapter.
	m_adapter->Release();
	m_adapter = 0;

	// Release the factory.
	m_factory->Release();
	m_factory = 0;

	return true;
}

bool DXRenderingSystemNS::createSwapChain()
{
	HRESULT result;
	D3D_FEATURE_LEVEL featureLevel;

	// Initialize the swap chain description.
	ZeroMemory(&g_DXRenderingSystemComponent->m_swapChainDesc, sizeof(g_DXRenderingSystemComponent->m_swapChainDesc));

	// Set to a single back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferCount = 1;

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	// Set the width and height of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Width = (UINT)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Height = (UINT)l_screenResolution.y;

	// Set regular 32-bit surface for the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (g_DXRenderingSystemComponent->m_vsync_enabled)
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Numerator = g_DXRenderingSystemComponent->m_refreshRate.x;
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Denominator = g_DXRenderingSystemComponent->m_refreshRate.y;
	}
	else
	{
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	g_DXRenderingSystemComponent->m_swapChainDesc.OutputWindow = g_DXWindowSystemComponent->m_hwnd;

	// Turn multisampling off.
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Count = 1;
	g_DXRenderingSystemComponent->m_swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	// @TODO: finish this feature
	g_DXRenderingSystemComponent->m_swapChainDesc.Windowed = true;

	// Set the scan line ordering and scaling to unspecified.
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	g_DXRenderingSystemComponent->m_swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	g_DXRenderingSystemComponent->m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	g_DXRenderingSystemComponent->m_swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &g_DXRenderingSystemComponent->m_swapChainDesc, &g_DXRenderingSystemComponent->m_swapChain, &g_DXRenderingSystemComponent->m_device, NULL, &g_DXRenderingSystemComponent->m_deviceContext);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the swap chain/D3D device/D3D device context!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool DXRenderingSystemNS::createBackBuffer()
{
	HRESULT result;

	// Get the pointer to the back buffer.
	result = g_DXRenderingSystemComponent->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&g_DXRenderingSystemComponent->m_renderTargetTexture);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't get back buffer pointer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = g_DXRenderingSystemComponent->m_device->CreateRenderTargetView(g_DXRenderingSystemComponent->m_renderTargetTexture, NULL, &g_DXRenderingSystemComponent->m_renderTargetView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create render target view!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	g_DXRenderingSystemComponent->m_renderTargetTexture->Release();
	g_DXRenderingSystemComponent->m_renderTargetTexture = 0;

	// Initialize the description of the depth buffer.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthTextureDesc, sizeof(g_DXRenderingSystemComponent->m_depthTextureDesc));

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	// Set up the description of the depth buffer.
	g_DXRenderingSystemComponent->m_depthTextureDesc.Width = (UINT)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Height = (UINT)l_screenResolution.y;
	g_DXRenderingSystemComponent->m_depthTextureDesc.MipLevels = 1;
	g_DXRenderingSystemComponent->m_depthTextureDesc.ArraySize = 1;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	g_DXRenderingSystemComponent->m_depthTextureDesc.SampleDesc.Count = 1;
	g_DXRenderingSystemComponent->m_depthTextureDesc.SampleDesc.Quality = 0;
	g_DXRenderingSystemComponent->m_depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	g_DXRenderingSystemComponent->m_depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	g_DXRenderingSystemComponent->m_depthTextureDesc.CPUAccessFlags = 0;
	g_DXRenderingSystemComponent->m_depthTextureDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = g_DXRenderingSystemComponent->m_device->CreateTexture2D(&g_DXRenderingSystemComponent->m_depthTextureDesc, NULL, &g_DXRenderingSystemComponent->m_depthStencilTexture);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the texture for the depth buffer!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Initialize the description of the stencil state.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthStencilDesc, sizeof(g_DXRenderingSystemComponent->m_depthStencilDesc));

	// Set up the description of the stencil state.
	g_DXRenderingSystemComponent->m_depthStencilDesc.DepthEnable = true;
	g_DXRenderingSystemComponent->m_depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	g_DXRenderingSystemComponent->m_depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	g_DXRenderingSystemComponent->m_depthStencilDesc.StencilEnable = true;
	g_DXRenderingSystemComponent->m_depthStencilDesc.StencilReadMask = 0xFF;
	g_DXRenderingSystemComponent->m_depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	g_DXRenderingSystemComponent->m_depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = g_DXRenderingSystemComponent->m_device->CreateDepthStencilState(
		&g_DXRenderingSystemComponent->m_depthStencilDesc,
		&g_DXRenderingSystemComponent->m_depthStencilState);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the depth stencil state!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Set the depth stencil state.
	g_DXRenderingSystemComponent->m_deviceContext->OMSetDepthStencilState(
		g_DXRenderingSystemComponent->m_depthStencilState, 1);

	// Initialize the depth stencil view.
	ZeroMemory(&g_DXRenderingSystemComponent->m_depthStencilViewDesc, sizeof(
		g_DXRenderingSystemComponent->m_depthStencilViewDesc));

	// Set up the depth stencil view description.
	g_DXRenderingSystemComponent->m_depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	g_DXRenderingSystemComponent->m_depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	g_DXRenderingSystemComponent->m_depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = g_DXRenderingSystemComponent->m_device->CreateDepthStencilView(
		g_DXRenderingSystemComponent->m_depthStencilTexture,
		&g_DXRenderingSystemComponent->m_depthStencilViewDesc,
		&g_DXRenderingSystemComponent->m_depthStencilView);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the depth stencil view!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	g_DXRenderingSystemComponent->m_deviceContext->OMSetRenderTargets(
		1,
		&g_DXRenderingSystemComponent->m_renderTargetView,
		g_DXRenderingSystemComponent->m_depthStencilView);

	return true;
}

bool DXRenderingSystemNS::createRasterizer()
{
	HRESULT result;

	// Setup the raster description which will determine how and what polygons will be drawn.
	g_DXRenderingSystemComponent->m_rasterDescForward.AntialiasedLineEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.CullMode = D3D11_CULL_NONE;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthBias = 0;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthBiasClamp = 0.0f;
	g_DXRenderingSystemComponent->m_rasterDescForward.DepthClipEnable = true;
	g_DXRenderingSystemComponent->m_rasterDescForward.FillMode = D3D11_FILL_SOLID;
	g_DXRenderingSystemComponent->m_rasterDescForward.FrontCounterClockwise = true;
	g_DXRenderingSystemComponent->m_rasterDescForward.MultisampleEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.ScissorEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescForward.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state for forward pass
	result = g_DXRenderingSystemComponent->m_device->CreateRasterizerState(
		&g_DXRenderingSystemComponent->m_rasterDescForward,
		&g_DXRenderingSystemComponent->m_rasterStateForward);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the rasterizer state for forward pass!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	g_DXRenderingSystemComponent->m_rasterDescDeferred.AntialiasedLineEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.CullMode = D3D11_CULL_NONE;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthBias = 0;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthBiasClamp = 0.0f;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.DepthClipEnable = true;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.FillMode = D3D11_FILL_SOLID;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.FrontCounterClockwise = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.MultisampleEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.ScissorEnable = false;
	g_DXRenderingSystemComponent->m_rasterDescDeferred.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state for deferred pass
	result = g_DXRenderingSystemComponent->m_device->CreateRasterizerState(
		&g_DXRenderingSystemComponent->m_rasterDescDeferred,
		&g_DXRenderingSystemComponent->m_rasterStateDeferred);
	if (FAILED(result))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create the rasterizer state for deferred pass!");
		m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	// Setup the viewport for rendering.
	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	g_DXRenderingSystemComponent->m_viewport.Width =
		(float)l_screenResolution.x;
	g_DXRenderingSystemComponent->m_viewport.Height =
		(float)l_screenResolution.y;
	g_DXRenderingSystemComponent->m_viewport.MinDepth = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.MaxDepth = 1.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftX = 0.0f;
	g_DXRenderingSystemComponent->m_viewport.TopLeftY = 0.0f;

	return true;
}

bool DXRenderingSystemNS::setup(IRenderingFrontendSystem* renderingFrontend)
{
	m_renderingFrontendSystem = renderingFrontend;

	g_DXWindowSystemComponent = &DXWindowSystemComponent::get();
	g_DXRenderingSystemComponent = &DXRenderingSystemComponent::get();

	bool result = true;
	result = result && createPhysicalDevices();
	result = result && createSwapChain();
	result = result && createBackBuffer();
	result = result && createRasterizer();

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	// Setup the description of the deferred pass.
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureWidth = l_screenResolution.x;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.textureHeight = l_screenResolution.y;
	DXRenderingSystemComponent::get().deferredPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	DXRenderingSystemComponent::get().deferredPassRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DXRenderingSystemComponent::get().deferredPassRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	DXRenderingSystemComponent::get().deferredPassRTVDesc.Texture2D.MipSlice = 0;

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem setup finished.");
	return result;
}

bool DXRenderingSystemNS::update()
{
	if (m_renderingFrontendSystem->anyUninitializedMeshDataComponent())
	{
		auto l_MDC = m_renderingFrontendSystem->acquireUninitializedMeshDataComponent();
		if (l_MDC)
		{
			auto l_result = generateDXMeshDataComponent(l_MDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create DXMeshDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}
	if (m_renderingFrontendSystem->anyUninitializedTextureDataComponent())
	{
		auto l_TDC = m_renderingFrontendSystem->acquireUninitializedTextureDataComponent();
		if (l_TDC)
		{
			auto l_result = generateDXTextureDataComponent(l_TDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create DXTextureDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}

	// Clear the buffers to begin the scene.
	prepareRenderingData();

	DXGeometryRenderingPassUtilities::update();

	DXLightRenderingPassUtilities::update();

	DXFinalRenderingPassUtilities::update();

	return true;
}

bool DXRenderingSystemNS::terminate()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->SetFullscreenState(false, NULL);
	}

	if (g_DXRenderingSystemComponent->m_rasterStateDeferred)
	{
		g_DXRenderingSystemComponent->m_rasterStateDeferred->Release();
		g_DXRenderingSystemComponent->m_rasterStateDeferred = 0;
	}

	if (g_DXRenderingSystemComponent->m_depthStencilView)
	{
		g_DXRenderingSystemComponent->m_depthStencilView->Release();
		g_DXRenderingSystemComponent->m_depthStencilView = 0;
	}

	if (g_DXRenderingSystemComponent->m_depthStencilState)
	{
		g_DXRenderingSystemComponent->m_depthStencilState->Release();
		g_DXRenderingSystemComponent->m_depthStencilState = 0;
	}

	if (g_DXRenderingSystemComponent->m_depthStencilTexture)
	{
		g_DXRenderingSystemComponent->m_depthStencilTexture->Release();
		g_DXRenderingSystemComponent->m_depthStencilTexture = 0;
	}

	if (g_DXRenderingSystemComponent->m_renderTargetView)
	{
		g_DXRenderingSystemComponent->m_renderTargetView->Release();
		g_DXRenderingSystemComponent->m_renderTargetView = 0;
	}

	if (g_DXRenderingSystemComponent->m_deviceContext)
	{
		g_DXRenderingSystemComponent->m_deviceContext->Release();
		g_DXRenderingSystemComponent->m_deviceContext = 0;
	}

	if (g_DXRenderingSystemComponent->m_device)
	{
		g_DXRenderingSystemComponent->m_device->Release();
		g_DXRenderingSystemComponent->m_device = 0;
	}

	if (g_DXRenderingSystemComponent->m_swapChain)
	{
		g_DXRenderingSystemComponent->m_swapChain->Release();
		g_DXRenderingSystemComponent->m_swapChain = 0;
	}

	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem has been terminated.");
	return true;
}

bool  DXRenderingSystemNS::initializeDefaultAssets()
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE);
	g_DXRenderingSystemComponent->m_UnitLineDXMDC = generateDXMeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	g_DXRenderingSystemComponent->m_UnitQuadDXMDC = generateDXMeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
	g_DXRenderingSystemComponent->m_UnitCubeDXMDC = generateDXMeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE);
	g_DXRenderingSystemComponent->m_UnitSphereDXMDC = generateDXMeshDataComponent(l_MDC);

	g_DXRenderingSystemComponent->m_basicNormalDXTDC = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	g_DXRenderingSystemComponent->m_basicAlbedoDXTDC = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	g_DXRenderingSystemComponent->m_basicMetallicDXTDC = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	g_DXRenderingSystemComponent->m_basicRoughnessDXTDC = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	g_DXRenderingSystemComponent->m_basicAODXTDC = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));

	g_DXRenderingSystemComponent->m_iconTemplate_OBJ = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::OBJ));
	g_DXRenderingSystemComponent->m_iconTemplate_PNG = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::PNG));
	g_DXRenderingSystemComponent->m_iconTemplate_SHADER = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::SHADER));
	g_DXRenderingSystemComponent->m_iconTemplate_UNKNOWN = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::UNKNOWN));

	g_DXRenderingSystemComponent->m_iconTemplate_DirectionalLight = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT));
	g_DXRenderingSystemComponent->m_iconTemplate_PointLight = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT));
	g_DXRenderingSystemComponent->m_iconTemplate_SphereLight = generateDXTextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT));

	return true;
}

void DXRenderingSystemNS::prepareRenderingData()
{
	auto l_cameraDataPack = m_renderingFrontendSystem->getCameraDataPack();

	DXRenderingSystemComponent::get().m_GPassCameraCBufferData.m_CamProjJittered = l_cameraDataPack.p_Jittered;
	DXRenderingSystemComponent::get().m_GPassCameraCBufferData.m_CamProjOriginal = l_cameraDataPack.p_Original;
	DXRenderingSystemComponent::get().m_GPassCameraCBufferData.m_CamRot = l_cameraDataPack.r;
	DXRenderingSystemComponent::get().m_GPassCameraCBufferData.m_CamTrans = l_cameraDataPack.t;
	DXRenderingSystemComponent::get().m_GPassCameraCBufferData.m_CamRot_prev = l_cameraDataPack.r_prev;
	DXRenderingSystemComponent::get().m_GPassCameraCBufferData.m_CamTrans_prev = l_cameraDataPack.t_prev;

	auto l_sunDataPack = m_renderingFrontendSystem->getSunDataPack();

	DXRenderingSystemComponent::get().m_LPassCBufferData.viewPos = l_cameraDataPack.globalPos;
	DXRenderingSystemComponent::get().m_LPassCBufferData.lightDir = l_sunDataPack.dir;
	DXRenderingSystemComponent::get().m_LPassCBufferData.color = l_sunDataPack.luminance;

	auto l_meshDataPack = m_renderingFrontendSystem->getMeshDataPack();

	if (l_meshDataPack.has_value())
	{
		m_meshDataPack = l_meshDataPack.value();
	}

	for (auto i : m_meshDataPack)
	{
		auto l_DXMDC = getDXMeshDataComponent(i.MDC->m_parentEntity);
		if (l_DXMDC && l_DXMDC->m_objectStatus == ObjectStatus::ALIVE)
		{
			GPassMeshDataPack l_meshDataPack;

			l_meshDataPack.indiceSize = i.MDC->m_indicesSize;
			l_meshDataPack.meshPrimitiveTopology = i.MDC->m_meshPrimitiveTopology;
			l_meshDataPack.meshCBuffer.m = i.m;
			l_meshDataPack.meshCBuffer.m_prev = i.m_prev;
			l_meshDataPack.meshCBuffer.m_normalMat = i.normalMat;
			l_meshDataPack.DXMDC = l_DXMDC;

			auto l_material = i.material;
			// any normal?
			auto l_TDC = l_material->m_texturePack.m_normalTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.normalDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useNormalTexture = false;
			}
			// any albedo?
			l_TDC = l_material->m_texturePack.m_albedoTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.albedoDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useAlbedoTexture = false;
			}
			// any metallic?
			l_TDC = l_material->m_texturePack.m_metallicTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.metallicDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useMetallicTexture = false;
			}
			// any roughness?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.roughnessDXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useRoughnessTexture = false;
			}
			// any ao?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.AODXTDC = getDXTextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useAOTexture = false;
			}

			l_meshDataPack.textureCBuffer.albedo = vec4(
				l_material->m_meshCustomMaterial.albedo_r,
				l_material->m_meshCustomMaterial.albedo_g,
				l_material->m_meshCustomMaterial.albedo_b,
				1.0f
			);
			l_meshDataPack.textureCBuffer.MRA = vec4(
				l_material->m_meshCustomMaterial.metallic,
				l_material->m_meshCustomMaterial.roughness,
				l_material->m_meshCustomMaterial.ao,
				1.0f
			);

			DXRenderingSystemComponent::get().m_GPassMeshDataQueue.push(l_meshDataPack);
		}
	}
}

bool DXRenderingSystem::setup(IRenderingFrontendSystem* renderingFrontend)
{
	return DXRenderingSystemNS::setup(renderingFrontend);
}

bool DXRenderingSystem::initialize()
{
	DXRenderingSystemNS::initializeDefaultAssets();
	DXGeometryRenderingPassUtilities::initialize();
	DXLightRenderingPassUtilities::initialize();
	DXFinalRenderingPassUtilities::initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem has been initialized.");
	return true;
}

bool DXRenderingSystem::update()
{
	return DXRenderingSystemNS::update();
}

bool DXRenderingSystem::terminate()
{
	return DXRenderingSystemNS::terminate();
}

ObjectStatus DXRenderingSystem::getStatus()
{
	return DXRenderingSystemNS::m_objectStatus;
}

bool DXRenderingSystem::resize()
{
	return true;
}

bool DXRenderingSystem::reloadShader(RenderPassType renderPassType)
{
	return true;
}

bool DXRenderingSystem::bakeGI()
{
	return true;
}