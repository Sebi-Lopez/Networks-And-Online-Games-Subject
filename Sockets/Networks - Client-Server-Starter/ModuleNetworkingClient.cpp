#include "ModuleNetworkingClient.h"

#include <WinSock2.h>

bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff

	// - Create the socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (client_socket == INVALID_SOCKET)
	{
		reportError("Creating Client Socket");
		return true;
	}

	// - Create the remote address object
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverAddressStr, &serverAddress.sin_addr);

	// - Connect to the remote address
	if (connect(client_socket, (const sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		reportError("Connecting client socket to server socket");
		return true;
	}

	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(client_socket);

	// If everything was ok... change the state
	state = ClientState::Start;

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO(jesus): Send the player name to the server
		int res = send(client_socket, (playerName + '\0').c_str(), playerName.length() + 1, 0);
		if (res == SOCKET_ERROR) {
			reportError("sending name msg");
		}
		else {
			state = ClientState::Logging;
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, byte * data)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

