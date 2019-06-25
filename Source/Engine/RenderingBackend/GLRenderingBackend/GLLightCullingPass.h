#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLLightCullingPass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();

	GLTextureDataComponent* getLightGrid();

	GLTextureDataComponent* getHeatMap();
}