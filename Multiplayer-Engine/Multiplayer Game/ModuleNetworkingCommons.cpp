#include "Networks.h"

uint16 packInputControllerButtons(const InputController & input)
{
	uint16 buttonBits = 0;
	int buttonIndex = 0;
	for (ButtonState buttonState : input.buttons)
	{
		uint16 bit = (int)(buttonState == ButtonState::Press ||
			               buttonState == ButtonState::Pressed);
		buttonBits |= (bit << buttonIndex);
		buttonIndex++;
	}

	return buttonBits;
}

uint16 packInputMouseButtons(const MouseController& input)
{
	uint16 buttonBits = 0;
	int buttonIndex = 0;
	for (ButtonState buttonState : input.buttons)
	{
		uint16 bit = (int)(buttonState == ButtonState::Press ||
			buttonState == ButtonState::Pressed);
		buttonBits |= (bit << buttonIndex);
		buttonIndex++;
	}

	return buttonBits;
}



void unpackInputControllerButtons(uint16 buttonBits, InputController & input)
{
	// NOTE(jesus): This table contains the ButtonState depending on the pressed state (true or false) of a button
	static const ButtonState transition[2][2] =      // Index 0 means released, 1 means pressed
	{
		{ButtonState::Idle,    ButtonState::Press},  // 0 0, 0 1
		{ButtonState::Release, ButtonState::Pressed} // 1 0, 1 1
	};

	int buttonIndex = 0;
	for (ButtonState &buttonState : input.buttons)
	{
		int wasPressed = (int)(buttonState == ButtonState::Press ||
			                   buttonState == ButtonState::Pressed);
		int isPressed = (int)(bool)(buttonBits & (1 << buttonIndex));
		buttonState = transition[wasPressed][isPressed];
		buttonIndex++;
	}
}

void unpackInputMouseButtons(uint16 buttonBits, MouseController& input)
{
	// NOTE(jesus): This table contains the ButtonState depending on the pressed state (true or false) of a button
	static const ButtonState transition[2][2] =      // Index 0 means released, 1 means pressed
	{
		{ButtonState::Idle,    ButtonState::Press},  // 0 0, 0 1
		{ButtonState::Release, ButtonState::Pressed} // 1 0, 1 1
	};

	int buttonIndex = 0;
	for (ButtonState& buttonState : input.buttons)
	{
		int wasPressed = (int)(buttonState == ButtonState::Press ||
			buttonState == ButtonState::Pressed);
		int isPressed = (int)(bool)(buttonBits & (1 << buttonIndex));
		buttonState = transition[wasPressed][isPressed];
		buttonIndex++;
	}
}

InputController inputControllerFromInputPacketData(const InputPacketData & inputPacketData, const InputController & previousGamepad)
{
	InputController gamepad = previousGamepad;
	gamepad.horizontalAxis = inputPacketData.horizontalAxis;
	gamepad.verticalAxis = inputPacketData.verticalAxis;
	unpackInputControllerButtons(inputPacketData.buttonBits, gamepad);
	return gamepad;
}

MouseController inputMouseFromInputPacketData(const InputPacketData& inputPacketData, const MouseController& previousMouse)
{
	MouseController mouse = previousMouse;
	mouse.x = inputPacketData.mousex;
	mouse.y = inputPacketData.mousey;
	unpackInputMouseButtons(inputPacketData.mouseBits, mouse);
	return mouse;
}