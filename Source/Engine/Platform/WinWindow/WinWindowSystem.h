#pragma once
#include "../../Core/IWindowSystem.h"

class WinWindowSystem : public IWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(WinWindowSystem);

	bool setup(void* hInstance, void* hwnd) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	IWindowSurface* getWindowSurface() override;
	const std::vector<ButtonState>& getButtonState() override;

	bool sendEvent(unsigned int umsg, unsigned int WParam, int LParam) override;
	bool addEventCallback(WindowEventCallbackFunctor* functor) override;
};