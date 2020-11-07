#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	// - Create the remote address object
	// - Connect to the remote address
	// - Add the created socket to the managed list of sockets using addSocket()

	int res = 0;

	// create the socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == INVALID_SOCKET)
	{
		reportError("creating socket");
		return false;
	}

	// create the remote address object
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverAddressStr, &serverAddress.sin_addr);
	
	// connect to remote address
	res = connect(client_socket, (const struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (res == SOCKET_ERROR)
	{
		reportError("Connecting to remote addr socket");
		return false;
	}

	// add to managed list
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
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (sendPacket(packet, client_socket))
		{
			state = ClientState::Logging;
		}
		else
		{
			disconnect();
			state = ClientState::Stopped;
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

		if (ImGui::Button("Log Out"))
		{
			disconnect();
			state = ClientState::Stopped;
			ClearChat();
		}
		ImGui::SameLine();
		ImGui::Text("Logged in as: %s", playerName.c_str());
		


		for (std::list<ChatEntry>::iterator iter = chatLog.begin(); iter != chatLog.end(); iter++)
		{
			PrintChatEntry((*iter));
		}

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 150);

		char user_message[120] = { "\0" };

		if (ImGui::InputText("Chat", user_message, 120, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			SendChatMessage(user_message);
		}
		
		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	ServerMessage serverMessage;
	packet >> serverMessage; 

	if (serverMessage == ServerMessage::Welcome)
	{
		std::string from; 
		packet >> from;
		std::string message;
		packet >> message;
		chatLog.push_back(ChatEntry(from, message, 0.0f, 1.0f, 1.0f));
	}
	else if (serverMessage == ServerMessage::NameNotAvailable)
	{
		// Show User that name is not available
		LOG("Name is already taken. Pleas chose another one.");
		ClearChat();
		disconnect();
		state = ClientState::Stopped;
	}
	else if (serverMessage == ServerMessage::Notification)
	{
		std::string from;
		packet >> from;
		std::string message;
		packet >> message;
		chatLog.push_back(ChatEntry(from, message, 1.0f, 1.0f, 0.0f));
	}

	else if (serverMessage == ServerMessage::ChatDistribution)
	{
		std::string from; 
		packet >> from;
		std::string message;
		packet >> message; 
		chatLog.push_back(ChatEntry(from, message));
	}
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::PrintChatEntry(ChatEntry entry)
{

	if (entry.from.empty() || entry.from == "Server")
	{
		// Messages from server are not printed who sends them
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.r, entry.g,entry.b, 255));
		ImGui::Text("%s", entry.text.c_str());
		ImGui::PopStyleColor();
	}
	else
	{
		// Print message SENDER with its color
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.r, entry.g, entry.b, 255));
		ImGui::Text("%s:", entry.from.c_str());
		ImGui::PopStyleColor();

		// Print the actual message
		ImGui::SameLine();
		ImGui::Text("%s", entry.text.c_str());
	}
}

void ModuleNetworkingClient::ClearChat()
{
	chatLog.clear();
}

void ModuleNetworkingClient::SendChatMessage(std::string message)
{
	OutputMemoryStream messagePackage; 
	messagePackage << ClientMessage::ChatEntry;
	messagePackage << playerName;
	messagePackage << message;
	sendPacket(messagePackage, client_socket);
}

