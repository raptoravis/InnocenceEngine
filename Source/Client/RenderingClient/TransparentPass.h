#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace TransparentPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
};
