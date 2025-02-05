#pragma once

#include "ModuleNetworking.h"

#include <list>



class ModuleNetworkingClient : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	bool start(const char *serverAddress, int serverPort, const char *playerName);

	bool isRunning() const;



private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool update() override;

	bool gui() override;



	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	void onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet) override;

	void onSocketDisconnected(SOCKET socket) override;

	void PrintChatEntry(ChatEntry entry);

	void ClearChat();

	void SendChatMessage(const std::string& message);

	void PushCommandError(); 

	bool isMuted(const std::string& user);

	void BreakDownCommand(const std::string& message, std::string& command, std::string& first_attrib, std::string& second_attrib);

	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		Logging,
		Failed,
		Kicked
	};

	ClientState state = ClientState::Stopped;

	sockaddr_in serverAddress = {};
	SOCKET client_socket = INVALID_SOCKET;
	
	float user_color[3] = { 0.5f, 0.5f, 0.5f };
	std::string playerName;

	std::list<ChatEntry> chatLog; 

	std::list<std::string> mutedUsers; 

};

