#include "ModuleNetworkingClient.h"


//////////////////////////////////////////////////////////////////////
// ModuleNetworkingClient public methods
//////////////////////////////////////////////////////////////////////


void ModuleNetworkingClient::setServerAddress(const char * pServerAddress, uint16 pServerPort)
{
	serverAddressStr = pServerAddress;
	serverPort = pServerPort;
}

void ModuleNetworkingClient::setPlayerInfo(const char * pPlayerName, uint8 pSpaceshipType)
{
	playerName = pPlayerName;
	spaceshipType = pSpaceshipType;
}

const PlayerInfo ModuleNetworkingClient::GetPlayerInfo() const
{
	PlayerInfo info;
	info.playerName = playerName;
	info.spaceShipType = spaceshipType;
	return info;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingClient::onStart()
{
	if (!createSocket()) return;

	if (!bindSocketToPort(0)) {
		disconnect();
		return;
	}

	// Create remote address
	serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverAddressStr.c_str(), &serverAddress.sin_addr);
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingClient::startClient() - inet_pton");
		disconnect();
		return;
	}

	state = ClientState::Connecting;

	inputDataFront = 0;
	inputDataBack = 0;

	secondsSinceLastHello = 9999.0f;
	secondsSinceLastInputDelivery = 0.0f;
}

void ModuleNetworkingClient::onGui()
{
	if (state == ClientState::Stopped) return;

	if (ImGui::CollapsingHeader("ModuleNetworkingClient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (state == ClientState::Connecting)
		{
			ImGui::Text("Connecting to server...");
		}
		else if (state == ClientState::Connected)
		{
			ImGui::Text("Connected to server");

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Id: %u", playerId);
			ImGui::Text(" - Name: %s", playerName.c_str());

			ImGui::Separator();

			ImGui::Text("Spaceship info:");
			ImGui::Text(" - Type: %u", spaceshipType);
			ImGui::Text(" - Network id: %u", networkId);

			vec2 playerPosition = {};
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
			}
			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);

			ImGui::Separator();

			ImGui::Text("Connection checking info:");
			ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
			ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

			ImGui::Separator();

			ImGui::Text("Input:");
			ImGui::InputFloat("Delivery interval (s)", &inputDeliveryIntervalSeconds, 0.01f, 0.1f, 4);
		}
	}
}

void ModuleNetworkingClient::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	// TODO(you): UDP virtual connection lab session
	timeLastRecvPacket = Time.time;

	uint32 protoId;
	packet >> protoId;
	if (protoId != PROTOCOL_ID) return;

	ServerMessage message;
	packet >> message;

	if (message == ServerMessage::Ping) 
	{
		//WLOG("PING from server");
		return;
	}

	if (state == ClientState::Connecting)
	{
		if (message == ServerMessage::Welcome)
		{
			packet >> playerId;
			packet >> networkId;

			LOG("ModuleNetworkingClient::onPacketReceived() - Welcome from server");
			state = ClientState::Connected;
		}
		else if (message == ServerMessage::Unwelcome)
		{
			WLOG("ModuleNetworkingClient::onPacketReceived() - Unwelcome from server :-(");
			disconnect();
		}
	}
	else if (state == ClientState::Connected)
	{
		// TODO(you): World state replication lab session
		if(message == ServerMessage::Replication)
			repMan.Read(packet);

		// TODO(you): Reliability on top of UDP lab session
	}


}

void ModuleNetworkingClient::onUpdate()
{
	if (state == ClientState::Stopped) return;


	// TODO(you): UDP virtual connection lab session -----------
	// disconnect the client if exceeds timeout
	if (Time.time - timeLastRecvPacket > DISCONNECT_TIMEOUT_SECONDS 
		&& 
		state != ClientState::Connecting)
	{
		WLOG("Disconnected due to a server inactivity");
		disconnect();
		state = ClientState::Stopped;
		return;
	}
	// ----------------------------------------------------------

	if (state == ClientState::Connecting)
	{
		secondsSinceLastHello += Time.deltaTime;

		if (secondsSinceLastHello > 1.0f) // TODO: if too short, and disconnect/reconnect fast (client), causes to get
			// the reply from hello many times(about 2 times normally), and linkingContext wants to "rewrite" info from old
			// 
		{
			secondsSinceLastHello = 0.0f;

			OutputMemoryStream packet;
			packet << PROTOCOL_ID;
			packet << ClientMessage::Hello;
			packet << playerName;
			packet << spaceshipType;

			sendPacket(packet, serverAddress);
		}
	}
	else if (state == ClientState::Connected)
	{
		// TODO(you): UDP virtual connection lab session ------
		// send periodical ping packet
		if (Time.time - timeLastSentPing > PING_INTERVAL_SECONDS)
		{
			OutputMemoryStream packet;
			packet << PROTOCOL_ID;
			packet << ClientMessage::Ping;
			sendPacket(packet, serverAddress);
			timeLastSentPing = Time.time;
		}
		// ----------------------------------------------------

		// Process more inputs if there's space
		if (inputDataBack - inputDataFront < ArrayCount(inputData))
		{
			// Pack current input
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.sequenceNumber = currentInputData;
			inputPacketData.horizontalAxis = Input.horizontalAxis;
			inputPacketData.verticalAxis = Input.verticalAxis;
			inputPacketData.buttonBits = packInputControllerButtons(Input);
			inputPacketData.mousex = Mouse.x;
			inputPacketData.mousey = Mouse.y;
			inputPacketData.mouseBits = packInputMouseButtons(Mouse);

		}

		secondsSinceLastInputDelivery += Time.deltaTime;

		// Input delivery interval timed out: create a new input packet
		if (secondsSinceLastInputDelivery > inputDeliveryIntervalSeconds)
		{
			secondsSinceLastInputDelivery = 0.0f;

			OutputMemoryStream packet;
			packet << PROTOCOL_ID;
			packet << ClientMessage::Input;

			// TODO(you): Reliability on top of UDP lab session

			for (uint32 i = inputDataFront; i < inputDataBack; ++i)
			{
				InputPacketData &inputPacketData = inputData[i % ArrayCount(inputData)];
				packet << inputPacketData.sequenceNumber;
				packet << inputPacketData.horizontalAxis;
				packet << inputPacketData.verticalAxis;
				packet << inputPacketData.buttonBits;
				packet << inputPacketData.mousex;
				packet << inputPacketData.mousey;
				packet << inputPacketData.mouseBits;
			}

			// Clear the queue
			inputDataFront = inputDataBack;

			sendPacket(packet, serverAddress);
		}

		// TODO(you): Latency management lab session

		// Update camera for player
		GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
		if (playerGameObject != nullptr)
		{
			//App->modRender->cameraPosition = playerGameObject->position;
		}
		else
		{
			// This means that the player has been destroyed (e.g. killed)
		}
	}
}

void ModuleNetworkingClient::onConnectionReset(const sockaddr_in & fromAddress)
{
	disconnect();
}

void ModuleNetworkingClient::onDisconnect()
{
	state = ClientState::Stopped;

	GameObject *networkGameObjects[MAX_NETWORK_OBJECTS] = {};
	uint16 networkGameObjectsCount;
	App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
	App->modLinkingContext->clear();

	for (uint32 i = 0; i < networkGameObjectsCount; ++i)
	{
		Destroy(networkGameObjects[i]);
	}

	App->modRender->cameraPosition = {};
}
