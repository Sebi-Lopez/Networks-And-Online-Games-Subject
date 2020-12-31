#include "ModuleNetworkingServer.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::setListenPort(int port)
{
	listenPort = port;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::onStart()
{
	if (!createSocket()) return;

	// Reuse address
	int enable = 1;
	int res = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingServer::start() - setsockopt");
		disconnect();
		return;
	}

	// Create and bind to local address
	if (!bindSocketToPort(listenPort)) {
		return;
	}

	state = ServerState::Listening;
}

void ModuleNetworkingServer::onGui()
{
	if (ImGui::CollapsingHeader("ModuleNetworkingServer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Connection checking info:");
		ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
		ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

		ImGui::Separator();

		if (state == ServerState::Listening)
		{
			int count = 0;

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				if (clientProxies[i].connected)
				{
					ImGui::Text("CLIENT %d", count++);
					ImGui::Text(" - address: %d.%d.%d.%d",
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b1,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b2,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b3,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b4);
					ImGui::Text(" - port: %d", ntohs(clientProxies[i].address.sin_port));
					ImGui::Text(" - name: %s", clientProxies[i].name.c_str());
					ImGui::Text(" - id: %d", clientProxies[i].clientId);
					if (clientProxies[i].gameObject != nullptr)
					{
						ImGui::Text(" - gameObject net id: %d", clientProxies[i].gameObject->networkId);
					}
					else
					{
						ImGui::Text(" - gameObject net id: (null)");
					}
					
					ImGui::Separator();
				}
			}

			ImGui::Checkbox("Render colliders", &App->modRender->mustRenderColliders);
		}
	}
}

void ModuleNetworkingServer::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	if (state == ServerState::Listening)
	{
		uint32 protoId;
		packet >> protoId;
		if (protoId != PROTOCOL_ID) return;

		ClientMessage message;
		packet >> message;

		ClientProxy *proxy = getClientProxy(fromAddress);

		if (message == ClientMessage::Hello)
		{
			if (proxy == nullptr)
			{
				proxy = createClientProxy();

				if (proxy != nullptr)
				{
					std::string playerName;
					uint8 crosshairType;
					packet >> playerName;
					packet >> crosshairType;

					proxy->address.sin_family = fromAddress.sin_family;
					proxy->address.sin_addr.S_un.S_addr = fromAddress.sin_addr.S_un.S_addr;
					proxy->address.sin_port = fromAddress.sin_port;
					proxy->connected = true;
					proxy->name = playerName;
					proxy->clientId = nextClientId++;

					// Create new network object
					vec2 initialPosition = 500.0f * vec2{ Random.next() - 0.5f, Random.next() - 0.5f};
					//float initialAngle = 360.0f * Random.next();
					proxy->gameObject = spawnPlayer(crosshairType, initialPosition, playerName);
				}
				else
				{
					// NOTE(jesus): Server is full...
				}
			}

			if (proxy != nullptr)
			{
				// Send welcome to the new player
				OutputMemoryStream welcomePacket;
				welcomePacket << PROTOCOL_ID;
				welcomePacket << ServerMessage::Welcome;
				welcomePacket << proxy->clientId;
				welcomePacket << proxy->gameObject->networkId;
				sendPacket(welcomePacket, fromAddress);

				// Send all network objects to the new player
				uint16 networkGameObjectsCount;
				GameObject *networkGameObjects[MAX_NETWORK_OBJECTS];
				App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
				for (uint16 i = 0; i < networkGameObjectsCount; ++i)
				{
					GameObject *gameObject = networkGameObjects[i];
					
					if (gameObject == proxy->gameObject)
						continue;

					// TODO(you): World state replication lab session ------------------------------
					proxy->replication.Create(gameObject->networkId);
				}
				OutputMemoryStream repliPacket;
				repliPacket << PROTOCOL_ID;
				repliPacket << ServerMessage::Replication;
				repliPacket << proxy->nextExpectedInputSequenceNumber;
				proxy->replication.Write(repliPacket, &proxy->deliveryManager, proxy->replication.commandsList);
				sendPacket(repliPacket, fromAddress);
				// -----------------------------------------------------------------------------

				LOG("Message received: hello - from player %s", proxy->name.c_str());
			}
			else
			{
				OutputMemoryStream unwelcomePacket;
				unwelcomePacket << PROTOCOL_ID;
				unwelcomePacket << ServerMessage::Unwelcome;
				sendPacket(unwelcomePacket, fromAddress);

				WLOG("Message received: UNWELCOMED hello - server is full");
			}
		}
		else if (message == ClientMessage::Input)
		{
			proxy->deliveryManager.ProcessAckdSequenceNumbers(packet);
			// Process the input packet and update the corresponding game object
			if (proxy != nullptr && IsValid(proxy->gameObject))
			{
				// TODO(you): Reliability on top of UDP lab session

				// Read input data
				while (packet.RemainingByteCount() > 0)
				{
					InputPacketData inputData;
					packet >> inputData.sequenceNumber;
					packet >> inputData.horizontalAxis;
					packet >> inputData.verticalAxis;
					packet >> inputData.buttonBits;
					packet >> inputData.mousex;
					packet >> inputData.mousey;
					packet >> inputData.mouseBits;

					if (inputData.sequenceNumber >= proxy->nextExpectedInputSequenceNumber)
					{
						proxy->gamepad.horizontalAxis = inputData.horizontalAxis;
						proxy->gamepad.verticalAxis = inputData.verticalAxis;
						proxy->mouse.x = inputData.mousex;
						proxy->mouse.y = inputData.mousey;
						unpackInputControllerButtons(inputData.buttonBits, proxy->gamepad);
						unpackInputMouseButtons(inputData.mouseBits, proxy->mouse);
						proxy->gameObject->behaviour->onInput(proxy->gamepad);
						proxy->gameObject->behaviour->onMouse(proxy->mouse);
						proxy->nextExpectedInputSequenceNumber = inputData.sequenceNumber + 1;
					}
				}
			}
		}
		else if (message == ClientMessage::Ping)
		{
			//WLOG("PING");
		}
		// TODO(you): UDP virtual connection lab session -------
		// we assign lastreceived packet, still dont care if is ping message or not
		if(proxy)
			proxy->timeLastRecvPacket = Time.time;
	}
}

void ModuleNetworkingServer::onUpdate()
{
	if (state == ServerState::Listening)
	{
		// Handle networked game object destructions
		for (DelayedDestroyEntry &destroyEntry : netGameObjectsToDestroyWithDelay)
		{
			if (destroyEntry.object != nullptr)
			{
				destroyEntry.delaySeconds -= Time.deltaTime;
				if (destroyEntry.delaySeconds <= 0.0f)
				{
					destroyNetworkObject(destroyEntry.object);
					destroyEntry.object = nullptr;
				}
			}
		}

		bool sendPing = Time.time - timeLastGeneralPingSent > PING_INTERVAL_SECONDS;

		for (ClientProxy &clientProxy : clientProxies)
		{
			if (clientProxy.connected)
			{
				// TODO(you): UDP virtual connection lab session ------------------------------------
				// disconnect timeout
				if (Time.time - clientProxy.timeLastRecvPacket > DISCONNECT_TIMEOUT_SECONDS)
				{
					WLOG("Inactivity from %s client", clientProxy.name.c_str());
					destroyClientProxy(&clientProxy); // TODO: Lag/Stutter to rest of clients when this happens
					//continue;
					break;
				}
				// send ping
				if (sendPing)
				{
					OutputMemoryStream packet;
					packet << PROTOCOL_ID;
					packet << ClientMessage::Ping;

					sendPacket(packet, clientProxy.address);
				}
				// -----------------------------------------------------------------------------------

				// Don't let the client proxy point to a destroyed game object
				if (!IsValid(clientProxy.gameObject))
				{
					clientProxy.gameObject = nullptr;
				}

				// TODO(you): World state replication lab session
				clientProxy.replication.lastReplicationSent += Time.deltaTime;
				if (clientProxy.replication.lastReplicationSent >= REPLICATION_INTERVAL)
				{
					clientProxy.replication.lastReplicationSent = 0.0f;

					OutputMemoryStream commandsPacket;
					commandsPacket << PROTOCOL_ID;
					commandsPacket << ClientMessage::Replication;

					// Sneakily Input notification
					commandsPacket << clientProxy.nextExpectedInputSequenceNumber; 
					//LOG("Server: Next expected Input Sequence Number: %i", clientProxy.nextExpectedInputSequenceNumber);

					// Actual replication
					clientProxy.replication.Write(commandsPacket, &clientProxy.deliveryManager, clientProxy.replication.commandsList);

					sendPacket(commandsPacket, clientProxy.address);
				}

				// TODO(you): Reliability on top of UDP lab session
				if (clientProxy.deliveryManager.ProcessTimedOutPackets(&clientProxy.replication))
				{

					OutputMemoryStream commandsPacket;
					commandsPacket << PROTOCOL_ID;
					commandsPacket << ClientMessage::Replication;

					// Sneakily Input notification
					commandsPacket << clientProxy.nextExpectedInputSequenceNumber;
					//LOG("Server: Next expected Input Sequence Number: %i", clientProxy.nextExpectedInputSequenceNumber);

					LOG("Resending Must-Send Commands - %i", clientProxy.replication.mustReSendList.size());
					
					for (std::list<ReplicationCommand>::iterator iter = clientProxy.replication.mustReSendList.begin(); iter != clientProxy.replication.mustReSendList.end(); ++iter)
					{
						switch ((*iter).action)
						{
						case ReplicationAction::Create:
							LOG("Create - %i", (*iter).networkId);
							break;
						case ReplicationAction::Destroy:
							LOG("Destroy - %i", (*iter).networkId);
							break;
						case ReplicationAction::Update:
							LOG("Update - %i", (*iter).networkId);
							break;
						}
					}
					
					// Actual replication
					clientProxy.replication.Write(commandsPacket, &clientProxy.deliveryManager, clientProxy.replication.mustReSendList);

					sendPacket(commandsPacket, clientProxy.address);
				}
			}
		}
		if (sendPing)
			timeLastGeneralPingSent = Time.time;
	}
}

void ModuleNetworkingServer::onConnectionReset(const sockaddr_in & fromAddress)
{
	// Find the client proxy
	ClientProxy *proxy = getClientProxy(fromAddress);

	if (proxy)
	{
		// Clear the client proxy
		destroyClientProxy(proxy);
	}
}

void ModuleNetworkingServer::onDisconnect()
{
	uint16 netGameObjectsCount;
	GameObject *netGameObjects[MAX_NETWORK_OBJECTS];

	for (ClientProxy& clientProxy : clientProxies)
	{
		destroyClientProxy(&clientProxy);
	}

	App->modLinkingContext->getNetworkGameObjects(netGameObjects, &netGameObjectsCount);
	for (uint32 i = 0; i < netGameObjectsCount; ++i)
	{
		NetworkDestroy(netGameObjects[i]);
	}
	
	nextClientId = 0;

	state = ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Client proxies
//////////////////////////////////////////////////////////////////////

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::createClientProxy()
{
	// If it does not exist, pick an empty entry
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clientProxies[i].connected)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxy(const sockaddr_in &clientAddress)
{
	// Try to find the client
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].address.sin_addr.S_un.S_addr == clientAddress.sin_addr.S_un.S_addr &&
			clientProxies[i].address.sin_port == clientAddress.sin_port)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

void ModuleNetworkingServer::destroyClientProxy(ClientProxy *clientProxy)
{
	// Destroy the object from all clients
	if (IsValid(clientProxy->gameObject))
	{
		destroyNetworkObject(clientProxy->gameObject);
	}

    *clientProxy = {};
}


//////////////////////////////////////////////////////////////////////
// Spawning
//////////////////////////////////////////////////////////////////////

GameObject * ModuleNetworkingServer::spawnPlayer(uint8 crosshairType, vec2 initialPosition, std::string playerName)
{
	// Create a new game object with the player properties
	GameObject *gameObject = NetworkInstantiate();
	gameObject->netType = NetEntityType::Crosshair;
	gameObject->position = initialPosition;
	gameObject->size = { 100, 100 };
	//gameObject->angle = initialAngle;

	// Create collider
	//gameObject->collider = App->modCollision->addCollider(ColliderType::Player, gameObject);
	//gameObject->collider->isTrigger = true; // NOTE(jesus): This object will receive onCollisionTriggered events

	// TODO TODO: create on another place a function to contain these lines, to call from server or client from same place
	// Create behaviour
	PlayerCrosshair* crossHairBh = App->modBehaviour->addCrosshair(gameObject);
	crossHairBh->reticle = App->modBehaviour->GetCrosshairRects(crosshairType);
	crossHairBh->playerName = playerName;
	gameObject->behaviour = crossHairBh;
	gameObject->behaviour->isServer = true;

	// Create sprite
	gameObject->sprite = App->modRender->addSprite(gameObject);
	gameObject->sprite->order = 5;
	gameObject->sprite->texture = App->modResources->tex_crosshairs_ss;
	gameObject->sprite->clipRect = crossHairBh->reticle.reticle_outside;

	return gameObject;
}


//////////////////////////////////////////////////////////////////////
// Update / destruction
//////////////////////////////////////////////////////////////////////

GameObject * ModuleNetworkingServer::instantiateNetworkObject()
{
	// Create an object into the server
	GameObject * gameObject = Instantiate();

	// Register the object into the linking context
	App->modLinkingContext->registerNetworkGameObject(gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(you): World state replication lab session
			clientProxies[i].replication.Create(gameObject->networkId);
		}
	}

	return gameObject;
}

void ModuleNetworkingServer::updateNetworkObject(GameObject * gameObject, bool self_inform)
{
	// Notify all client proxies' replication manager to update the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			if (clientProxies[i].gameObject->networkId == gameObject->networkId && !self_inform)
				continue;
			// TODO(you): World state replication lab session
			clientProxies[i].replication.Update(gameObject->networkId);
		}
	}
}

void ModuleNetworkingServer::NotifyCowboyWindow(GameObject* gameObject)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			clientProxies[i].replication.UpdateCowboyWindow(gameObject->networkId);
		}
	}
}

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(you): World state replication lab session
			clientProxies[i].replication.Destroy(gameObject->networkId);
		}
	}

	// Assuming the message was received, unregister the network identity
	App->modLinkingContext->unregisterNetworkGameObject(gameObject);

	// Finally, destroy the object from the server
	Destroy(gameObject);
}

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject, float delaySeconds)
{
	uint32 emptyIndex = MAX_GAME_OBJECTS;
	for (uint32 i = 0; i < MAX_GAME_OBJECTS; ++i)
	{
		if (netGameObjectsToDestroyWithDelay[i].object == gameObject)
		{
			float currentDelaySeconds = netGameObjectsToDestroyWithDelay[i].delaySeconds;
			netGameObjectsToDestroyWithDelay[i].delaySeconds = min(currentDelaySeconds, delaySeconds);
			return;
		}
		else if (netGameObjectsToDestroyWithDelay[i].object == nullptr)
		{
			if (emptyIndex == MAX_GAME_OBJECTS)
			{
				emptyIndex = i;
			}
		}
	}

	ASSERT(emptyIndex < MAX_GAME_OBJECTS);

	netGameObjectsToDestroyWithDelay[emptyIndex].object = gameObject;
	netGameObjectsToDestroyWithDelay[emptyIndex].delaySeconds = delaySeconds;
}


//////////////////////////////////////////////////////////////////////
// Global create / update / destruction of network game objects
//////////////////////////////////////////////////////////////////////

GameObject * NetworkInstantiate()
{
	ASSERT(App->modNetServer->isConnected());

	return App->modNetServer->instantiateNetworkObject();
}

void NetworkUpdate(GameObject * gameObject, bool self_inform)
{
	ASSERT(App->modNetServer->isConnected());
	ASSERT(gameObject->networkId != 0);

	App->modNetServer->updateNetworkObject(gameObject, self_inform);
}

void NetworkDestroy(GameObject * gameObject)
{
	NetworkDestroy(gameObject, 0.0f);
}

void NetworkDestroy(GameObject * gameObject, float delaySeconds)
{
	// If the server is down, dont do anything (you laser/explosion), you will be/are destroyed anyways
	if (!App->modNetServer->isConnected())
		return;

	ASSERT(App->modNetServer->isConnected());
	ASSERT(gameObject->networkId != 0);

	App->modNetServer->destroyNetworkObject(gameObject, delaySeconds);
}

void NetWorkUpdateTarget(GameObject* gameObject)
{
	ASSERT(App->modNetServer->isConnected());
	ASSERT(gameObject->networkId != 0);

	App->modNetServer->NotifyCowboyWindow(gameObject);
}
