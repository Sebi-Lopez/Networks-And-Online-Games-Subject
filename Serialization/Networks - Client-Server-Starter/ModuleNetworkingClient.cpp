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

		// Set User color
		packet >> user_color[0];
		packet >> user_color[1];
		packet >> user_color[2];

		chatLog.push_back(ChatEntry(from, message, 0.0f, 1.0f, 1.0f));
	}
	else if (serverMessage == ServerMessage::NameAlreadyExists)
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
		chatLog.push_back(ChatEntry(from, message, 0.5f, 0.5f, 0.5f));
	}
	else if (serverMessage == ServerMessage::CommandResponse)
	{
		std::string message;
		packet >> message;
		chatLog.push_back(ChatEntry(message, 0.3f, 8.0f, 0.3f));
	}
	else if (serverMessage == ServerMessage::ChatDistribution)
	{
		std::string from; 
		packet >> from;
		std::string message;
		packet >> message; 
		
		// Set User color
		float r, g, b;
		packet >> r;
		packet >> g;
		packet >> b;

		chatLog.push_back(ChatEntry(from, message, r, g, b));
	}
	else if (serverMessage == ServerMessage::Whisper)
	{
		std::string from;
		packet >> from;
		std::string message;
		packet >> message;
		std::string notification = from + " whispered to you: ";
		chatLog.push_back(ChatEntry(notification, 0.2f, 0.2f, 0.9f));
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
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.r, entry.g,entry.b, 1.0f));
		ImGui::Text("%s", entry.text.c_str());
		ImGui::PopStyleColor();
	}
	else
	{
		// Print message SENDER with its color
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(entry.r, entry.g, entry.b, 1.0f));
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

void ModuleNetworkingClient::SendChatMessage(const std::string& message)
{
	OutputMemoryStream messagePackage;

	if (message.at(0) != '/') {
		// Normal message
		messagePackage << ClientMessage::ChatEntry;
		messagePackage << playerName;
		messagePackage << message;
		for (int i = 0; i < 3; ++i)
			messagePackage << user_color[i];
	}
	else
	{
		// Find the actual command that is made from the '/' char to the first space
		size_t endCommand = message.find_first_of(" ");
		size_t sizeMessage = message.size();
		std::string command;

		if (endCommand == std::string::npos)
			command = message.substr(1); // Take only the word without '/'
		else
			command = message.substr(1, endCommand - 1); // - 1 cause endCommand points to the space pos

		if (command.compare(std::string("help")) == 0)
		{
			messagePackage << ClientMessage::C_Help;
		}
		else if (command.compare(std::string("list")) == 0)
		{
			messagePackage << ClientMessage::C_List;
		}
		else if (command.compare(std::string("whisper")) == 0)
		{
			if (endCommand == std::string::npos)	// This means that the command doesnt have any attributes
			{
				PushCommandError();
				return;
			}

			// String with the rest of the attributes of the command
			std::string attributes = message.substr(endCommand);
			attributes = attributes.substr(1); // The first character is a space, so we erase it; 

			size_t size_attributes = attributes.size();
			size_t secondSpace = attributes.find_first_of(" ");

			if (secondSpace == std::string::npos || size_attributes <= secondSpace) // This means that the command only has one attribute
			{
				PushCommandError();
				return;
			}

			// Attribute till the "first" space
			std::string to = attributes.substr(0, secondSpace);
			std::string msg_whispered = attributes.substr(secondSpace + 1);

			messagePackage << ClientMessage::C_Whisper;
			messagePackage << playerName; // From
			messagePackage << to;
			messagePackage << msg_whispered;
		}
		else
		{
			PushCommandError();
			return;
		}
	}
	

	sendPacket(messagePackage, client_socket);
}

void ModuleNetworkingClient::PushCommandError()
{
	chatLog.push_back(ChatEntry("Command doesn't exist or is incomplete. \nType /help to see the available commands.", 0.5f, 0.5f, 0.5f));

}

