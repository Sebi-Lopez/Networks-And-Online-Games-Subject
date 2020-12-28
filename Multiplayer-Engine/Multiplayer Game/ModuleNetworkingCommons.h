#pragma once

#define PROTOCOL_ID                           (uint32)'GAME'

// Packet header ///////////////////////////////////////////////////////

struct PacketHeader
{
	uint32 protocolId = PROTOCOL_ID;
	uint8 messageId;
};


// Input ///////////////////////////////////////////////////////////////

struct InputPacketData // TODO: remove unnecessary inputs
{
	uint32 sequenceNumber = 0;
	real32 horizontalAxis = 0.0f;
	real32 verticalAxis = 0.0f;
	uint16 buttonBits = 0;
	int16 mousex = 0;
	int16 mousey = 0;
	//uint16 mouseBits = 0;
	
};

uint16 packInputControllerButtons(const InputController &input);

void unpackInputControllerButtons(uint16 buttonBits, InputController &input);

InputController inputControllerFromInputPacketData(const InputPacketData &inputPacketData, const InputController &previousGamepad);
