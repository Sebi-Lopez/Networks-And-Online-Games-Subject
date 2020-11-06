#pragma once

struct ChatEntry
{
	ChatEntry(std::string from, std::string text) : 
		from(from), text(text) {}

	ChatEntry(std::string text) :
		text(text) {}

	std::string from;
	std::string text;
	float color[3] = { 0.0f, 0.0f,0.0f };
};

class ModuleNetworking : public Module
{
private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool init() override;

	bool preUpdate() override;

	bool cleanUp() override;



	//////////////////////////////////////////////////////////////////////
	// Socket event callbacks
	//////////////////////////////////////////////////////////////////////

	virtual bool isListenSocket(SOCKET socket) const { return false; }

	virtual void onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress) { }

	virtual void onSocketReceivedData(SOCKET s, const InputMemoryStream& packet) = 0;

	virtual void onSocketDisconnected(SOCKET s) = 0;



protected:

	std::vector<SOCKET> sockets;

	void addSocket(SOCKET socket);

	void disconnect();

	static void reportError(const char *message);
	
	static bool sendPacket(const OutputMemoryStream& packet, SOCKET socket);
};

