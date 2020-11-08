#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

enum class ClientMessage
{
	Hello,
	ChatEntry,
	C_Help,
	C_List,
	C_Whisper,
	C_ChangeColor
};

enum class ServerMessage
{
	Welcome,
	ChatDistribution,
	Notification,
	CommandResponse,
	Whisper,
	NameAlreadyExists
};

enum class NotificationType
{
	NewUser,
	DisconnectedUser
};
