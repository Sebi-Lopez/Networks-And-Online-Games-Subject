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
	if (state != ClientState::Stopped )
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);


		if (ImGui::Button("Log Out"))
		{
			disconnect();
			ClearChat();
			state = ClientState::Stopped;
		}
		ImGui::SameLine();
		ImGui::Text("Username: %s", playerName.c_str());

		ImGui::BeginChild("Hello", ImVec2(0, ImGui::GetWindowSize().y - 250), true);

		if (state == ClientState::Failed)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.f, 0.f, 1.0f));
			ImGui::Text("**** COULDN'T LOG IN PROPERLY: NAME ALREADY EXISTS ****");
			ImGui::PopStyleColor();
			ImGui::Text("**** Please log out and try again with another name ****");

			ImGui::EndChild();
			ImGui::End();
			return true;
		}

		if (state == ClientState::Kicked)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.f, 0.f, 1.0f));
			ImGui::Text("******** SOMEONE KICKED YOU FROM THE CHAT ********");
			ImGui::PopStyleColor();
			ImGui::Text("**** You pissed someone off. Be carefull with your actions ****");

			ImGui::EndChild();
			ImGui::End();
			return true;
		}

		for (std::list<ChatEntry>::iterator iter = chatLog.begin(); iter != chatLog.end(); iter++)
		{
			PrintChatEntry((*iter));
		}


		ImGui::EndChild();

		char user_message[120] = { "\0" };

		if (ImGui::InputText("Chat", user_message, 120, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			SendChatMessage(user_message);
		}

		if(!ImGui::IsAnyItemActive())
			ImGui::SetKeyboardFocusHere();

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	ServerMessage serverMessage;
	packet >> serverMessage; 

	switch (serverMessage)
	{
		case ServerMessage::Welcome:
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
		} break;
		 
		case ServerMessage::NameAlreadyExists:
		{
			// Show User that name is not available
			state = ClientState::Failed;
		} break;

		case ServerMessage::Notification:
		{
			std::string from;
			packet >> from;
			std::string message;
			packet >> message;
			chatLog.push_back(ChatEntry(from, message, 0.5f, 0.5f, 0.5f));
		} break;

		case ServerMessage::CommandResponse:
		{
			std::string message;
			packet >> message;
			chatLog.push_back(ChatEntry(message, 0.3f, 8.0f, 0.3f));
		} break;

		case ServerMessage::ChatDistribution:
		{
			std::string from;
			packet >> from;

			// Check if the user is muted
			if (isMuted(from))
				return;

			std::string message;
			packet >> message;

			// Set User color
			float r, g, b;
			packet >> r;
			packet >> g;
			packet >> b;

			chatLog.push_back(ChatEntry(from, message, r, g, b));
		} break;

		case ServerMessage::Whisper:
		{
			std::string from;
			packet >> from;

			// Check if the user is muted
			if (isMuted(from))
				return;

			std::string message;
			packet >> message;
			std::string notification = from + " whispered to you: ";
			chatLog.push_back(ChatEntry(notification, 0.2f, 0.2f, 0.9f));
			chatLog.push_back(ChatEntry(from, message));
		} break;

		case ServerMessage::MuteResponse:
		{
			bool exists = false;
			std::string muted;
			packet >> exists;
			packet >> muted;

			std::string notification;
			if (exists)
			{
				mutedUsers.push_back(muted);
				notification = "User " + muted + " is now muted";
			}
			else
			{
				notification = "Could not find user: " + muted + ".\n Type /list to see the list of connected users";
			}
			chatLog.push_back(ChatEntry(notification, 0.5f, 0.5f, 0.5f));
		} break;

		case ServerMessage::Kick:
		{
			state = ClientState::Kicked;
		} break;

		default: {
			break;
		}
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
		ImGui::TextWrapped("%s", entry.text.c_str());
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
		ImGui::TextWrapped("%s", entry.text.c_str());
	}
}

void ModuleNetworkingClient::ClearChat()
{
	chatLog.clear();
}

void ModuleNetworkingClient::SendChatMessage(const std::string& message)
{
	if (message.empty())
		return;

	OutputMemoryStream messagePackage;

	std::string command; 
	std::string first_attribute;
	std::string second_attribute; 

	BreakDownCommand(message, command, first_attribute, second_attribute);

	if (command.empty())
	{
		// Normal message
		messagePackage << ClientMessage::ChatEntry;
		messagePackage << playerName;
		messagePackage << message;
		for (int i = 0; i < 3; ++i)
			messagePackage << user_color[i];
	}
	else
	{
		if (command.compare(std::string("help")) == 0 && first_attribute.empty())
		{
			messagePackage << ClientMessage::C_Help;
		}
		else if (command.compare(std::string("list")) == 0 && first_attribute.empty())
		{
			messagePackage << ClientMessage::C_List;
		}
		else if (command.compare(std::string("clear")) == 0 && first_attribute.empty())
		{
			ClearChat();
		}
		else if (command.compare(std::string("mute")) == 0)
		{
			messagePackage << ClientMessage::C_Mute;

			// It ONLY has to have the first attribute
			if (first_attribute.empty() || !second_attribute.empty())
			{
				PushCommandError();
				return;
			}

			messagePackage << first_attribute;
		}
		else if (command.compare(std::string("unmute")) == 0)
		{
			// It ONLY has to have the first attribute
			if (first_attribute.empty() || !second_attribute.empty())
			{
				PushCommandError();
				return;
			}

			std::string user = first_attribute;

			if (user == playerName)
			{
				chatLog.push_back(ChatEntry("Whaaaat??? Now you can talk????!", 0.5f, 0.5f, 0.5f));
				return;
			}

			std::string notification;
			if (isMuted(user))
			{
				mutedUsers.remove(user);
				notification = "User: " + user + " is now unmuted";
				chatLog.push_back(ChatEntry(notification, 0.5f, 0.5f, 0.5f));
			}
			else
			{
				notification = "User: " + user + " is not muted or doesn't exist.\nFeel free to write /list to see the list of connected users.";
				chatLog.push_back(ChatEntry(notification, 0.5f, 0.5f, 0.5f));
			}
		}
		else if (command.compare(std::string("kick")) == 0)
		{
			// It ONLY has to have the first attribute
			if (first_attribute.empty() || !second_attribute.empty())
			{
				PushCommandError();
				return;
			}

			std::string user = first_attribute;

			if (user == playerName)
			{
				chatLog.push_back(ChatEntry("If you want to kick yourself, you can use the avobe button 'Log Out'. Don't hurt yourself.", 0.5f, 0.5f, 0.5f));
				return;
			}

			messagePackage << ClientMessage::C_Kick;
			messagePackage << user;
			messagePackage << playerName;
		}
		else if (command.compare(std::string("whisper")) == 0)
		{
			// It has to have both attributes
			if (first_attribute.empty() || second_attribute.empty())
			{
				PushCommandError();
				return;
			}

			std::string to = first_attribute;

			if (to == playerName)
			{
				chatLog.push_back(ChatEntry("If you want to whisper yourself, try to speak really low. :)", 0.5f, 0.5f, 0.5f));
				return;
			}

			std::string msg_whispered = second_attribute;

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

bool ModuleNetworkingClient::isMuted(const std::string& user)
{
	// Check if the user is muted
	for (std::list<std::string>::iterator it = mutedUsers.begin(); it != mutedUsers.end(); ++it)
	{
		if ((*it).compare(user) == 0)
		{
			return true;
		}
	}
	return false;
}

void ModuleNetworkingClient::BreakDownCommand(const std::string& message, std::string& command, std::string& first_attrib, std::string& second_attrib)
{
	// NO COMMAND
	if (message.at(0) != '/') // There is no command to break down
		return;


	// COMMAND WITHOUT ATTRIBUTES

	size_t first_space = message.find_first_of(" ");
	if (first_space == std::string::npos) // Case there's no space 
	{
		command = message.substr(1); // Take the whole message without the first char: '/'
		return;
	}
	else {
		command = message.substr(1, first_space - 1); // - 1 cause endCommand points to the space pos
	}


	// COMMAND WITH ATTRIBUTES

	std::string attributes = message.substr(first_space);  // Keep only from first_space till the end
	attributes = attributes.substr(1); // The first character is a space, so we erase it; 

	// Attributes right now is the whole sentance after the first space 

	size_t secondSpace = attributes.find_first_of(" ");

	if (secondSpace == std::string::npos) // This means there is no second space, therefore only one attribute
	{
		first_attrib = attributes;
		return;
	}

	// Now we are sure there are 2 words, and we know the pos of the space 
	
	// The first attribute is from the beggining to the pos of the space
	first_attrib = attributes.substr(0, secondSpace);

	// And the second one is from that pos (+1, cause otherwise you would take that space with you) till the end.
	second_attrib = attributes.substr(secondSpace + 1);
}

