#include "ModuleNetworkingServer.h"




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
	
	OutputMemoryStream welcomePackage; 
	welcomePackage << ServerMessage::Welcome;
	welcomePackage << "Server";
	welcomePackage << " --------- Welcome to the CHAT ---------";
	sendPacket(welcomePackage, socket);
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

		for (auto &connectedSocket : connectedSockets)
		{
			if (connectedSocket.socket == socket)
			{
				connectedSocket.playerName = playerName;
			}
		}
	}

	if (clientMessage == ClientMessage::ChatEntry)
	{
		std::string from;
		std::string message;
		packet >> from;
		packet >> message;

		OutputMemoryStream chatPackage; 
		chatPackage << ServerMessage::ChatDistribution; 
		chatPackage << from; 
		chatPackage << message;

		for (auto& connectedSocket : connectedSockets)
		{
			sendPacket(chatPackage, connectedSocket.socket);
		}
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
			connectedSockets.erase(it);
			break;
		}
	}
}

