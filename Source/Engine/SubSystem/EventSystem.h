#pragma once
#include "../Interface/IEventSystem.h"

class InnoEventSystem : public IEventSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoEventSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	InputConfig getInputConfig() override;

	void addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent) override;
	void addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent) override;

	void buttonStateCallback(ButtonState buttonState) override;
	void windowSizeCallback(int32_t width, int32_t height) override;
	void mouseMovementCallback(float mouseXPos, float mouseYPos) override;
	void scrollCallback(float xoffset, float yoffset) override;

	Vec2 getMousePosition() override;

	ObjectStatus getStatus() override;
};