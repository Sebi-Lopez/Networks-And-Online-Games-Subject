#include "ModuleNetworkingServer.h"


#include <random>


//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff
	// - Create the listenSocket
	// - Set address reuse
	// - Bind the socket to a local interface
	// - Enter in listen mode
	// - Add the listenSocket to the managed list of sockets using addSocket()
	int res = 0;
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		reportError("creating listen socket");
		return false;
	}

	int enable = 1;
	res = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR)
	{
		reportError("Set reuse address");
	}

	// BINDING section ------------------

	sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY; // any local ip address

	res = bind(listenSocket, (const struct sockaddr*)&bindAddr , sizeof(bindAddr));
	if (res == SOCKET_ERROR)
	{
		reportError("Binding to any local ip");
	}

	// ----------------------------------

	// Enter in listen mode -------------

	// The maximum length of the queue of pending connections. If set to SOMAXCONN, 
	// the underlying service provider responsible for socket s will set the backlog to a 
	// maximum reasonable value.
	res = listen(listenSocket, SOMAXCONN);
	if (res == SOCKET_ERROR)
	{
		reportError("Setting the socket to listen tcp inc");
	}

	// add to managed list of sockets
	addSocket(listenSocket);


	state = ServerState::Listening;

	// Random seeding
	srand(time(NULL));

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected sockets:");

		for (auto &connectedSocket : connectedSockets)
		{
			ImGui::Separator();
			ImGui::Text("Socket ID: %d", connectedSocket.socket);
			ImGui::Text("Address: %d.%d.%d.%d:%d",
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b1,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b2,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b3,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b4,
				ntohs(connectedSocket.address.sin_port));
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());
		}

		ImGui::End();
	}

	return true;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	// Set the player name of the corresponding connected socket proxy
	ClientMessage clientMessage;
	packet >> clientMessage; 

	if (clientMessage == ClientMessage::Hello)
	{
		std::string playerName;
		packet >> playerName;

		if (isNameAvailable(playerName))
		{
			for (auto& connectedSocket : connectedSockets)
			{
				if (connectedSocket.socket == socket)
				{
					connectedSocket.playerName = playerName;
					SendWelcomePackage(socket);
					NotifyAllConnectedUsers(playerName, NotificationType::NewUser);
					break;
				}
			}
		}
		else
		{
			// Send notification that the name is taken 
			OutputMemoryStream notification;
			notification << ServerMessage::NameAlreadyExists;
			sendPacket(notification, socket);

			// Delete it from the connected sockets (it will be disconnected on its own)
			for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
			{
				if ((*it).socket == socket)
				{
					connectedSockets.erase(it);
					break;
				}
			}
		}
	}

	if (clientMessage == ClientMessage::ChatEntry)
	{
		// Get Message Data
		std::string from;
		std::string message;		
		float color[3];

		packet >> from;
		packet >> message;
		for (int i = 0; i < 3; ++i)
			packet >> color[i];

		// Send message data to everyone
		OutputMemoryStream chatPackage; 
		chatPackage << ServerMessage::ChatDistribution; 
		chatPackage << from; 
		chatPackage << message;

		for (int i = 0; i < 3; ++i)
			chatPackage << color[i];

		for (auto& connectedSocket : connectedSockets)
		{
			sendPacket(chatPackage, connectedSocket.socket);
		}
	}

	if (clientMessage == ClientMessage::C_Help)
	{
		OutputMemoryStream helpPackage; 
		helpPackage << ServerMessage::CommandResponse;
		helpPackage << "Here's the list of commands that you can use: \n/help\n/list\n/whisper [to] [message]\n/clear\n/mute [user]\n/unmute [user]...";
		
		sendPacket(helpPackage, socket);
	}

	if (clientMessage == ClientMessage::C_List)
	{
		OutputMemoryStream listPackage;
		listPackage << ServerMessage::CommandResponse;

		// Make a string with the list of users
		std::string user_list = "Connected Users:\n";
		for (auto& connectedSocket : connectedSockets)
		{
			user_list += "- " + connectedSocket.playerName + "\n";
		}
		listPackage << user_list;

		sendPacket(listPackage, socket);
	}

	if (clientMessage == ClientMessage::C_Whisper)
	{
		std::string to;
		std::string msg;
		std::string from;
		packet >> from;
		packet >> to;
		packet >> msg;

		OutputMemoryStream whisperPacket;
		bool found = false; 

		// Find the whispered and send the whisper
		for (auto& connectedSocket : connectedSockets)
		{
			if (connectedSocket.playerName == to)
			{
				whisperPacket << ServerMessage::Whisper;
				whisperPacket << from;
				whisperPacket << msg;
				sendPacket(whisperPacket, connectedSocket.socket);
				found = true;
				break;
			}
		}

		// Send a response to the whisperer
		OutputMemoryStream responsePacket;
		responsePacket << ServerMessage::CommandResponse;
		std::string response;

		if (!found) 			
			response = "Couldn't find user: " + to + ".\nType /list to get the list of all connected users.";
		else					
			response = "Whisper sent correctly to: " + to + ".\nWhispered: " + msg;

		responsePacket << response;
		sendPacket(responsePacket, socket);
	}

	if(clientMessage == ClientMessage::C_Mute)
	{
		std::string to;
		packet >> to; 

		OutputMemoryStream muteResponse; 
		muteResponse << ServerMessage::MuteResponse; 

		bool found = false; 

		for (auto& connectedSocket : connectedSockets)
		{
			if (connectedSocket.playerName == to)
			{
				found = true; 
				break;
			}
		}

		muteResponse << found;
		muteResponse << to; 

		sendPacket(muteResponse, socket);
	}
}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto &connectedSocket = *it;
		if (connectedSocket.socket == socket)
		{
			NotifyAllConnectedUsers(connectedSocket.playerName, NotificationType::DisconnectedUser);
			connectedSockets.erase(it);
			break;
		}
	}
}

void ModuleNetworkingServer::NotifyAllConnectedUsers(const std::string newUser, NotificationType notificationType)
{
	OutputMemoryStream notificationPackage; 

	notificationPackage << ServerMessage::Notification; 
	notificationPackage << "Server";

	// Set the notification message depending on the type
	std::string notificationMsg;
	switch (notificationType)
	{
	case NotificationType::NewUser:
		notificationMsg = newUser + " joined the chat";
		break;

	case NotificationType::DisconnectedUser:
		notificationMsg = newUser + " left the chat";
		break;
	}

	notificationPackage << notificationMsg;

	for (auto& connectedSocket : connectedSockets)
	{
		sendPacket(notificationPackage, connectedSocket.socket);
	}
}

bool ModuleNetworkingServer::isNameAvailable(const std::string newName)
{
	for (auto& connectedSocket : connectedSockets)
	{
		if (connectedSocket.playerName == newName)
			return false;
	}
	return true;
}

void ModuleNetworkingServer::SendWelcomePackage(SOCKET socket)
{
	// Send the welcome package back 
	OutputMemoryStream welcomePackage;
	welcomePackage << ServerMessage::Welcome;
	welcomePackage << "Server";
	welcomePackage << "      --------- Welcome to the CHAT ---------\nFeel free to type /help to see the command list.";

	// Send 3 floats to set the users color - Random is set from 0.6 to 1 to try and get only bright colors. 
	for (int i = 0; i < 3; ++i)
		welcomePackage << 0.6f + ((rand() % 40) / 100.f);

	sendPacket(welcomePackage, socket);
}
