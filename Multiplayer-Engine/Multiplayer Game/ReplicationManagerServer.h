#pragma once

// TODO(you): World state replication lab session
//#include <map>
//#define MAX_ACTIONS 256

enum class ReplicationAction
{
	None, Create, Update, CowboyOrder, Destroy
};

struct ReplicationCommand
{
	ReplicationAction action;
	uint32 networkId;
};

class ReplicationManagerServer
{
public:
	void Create(uint32 networkId);
	void Update(uint32 networkdId);
	void UpdateCowboyWindow(uint32 networkId);
	void Destroy(uint32 networkId);

	void Write(OutputMemoryStream& packet, DeliveryManager* deliveryManager, std::list<ReplicationCommand>& commands, std::list<WindowInfo>& windowsInfo, bool isResending = false); // xd

	std::list<ReplicationCommand> commandsList; 

	// Delivery Manager Resending info 
	std::list<ReplicationCommand> mustReSendList; 
	std::list<WindowInfo> windowInfoResendList;

	float lastReplicationSent = 0.0f; 
};