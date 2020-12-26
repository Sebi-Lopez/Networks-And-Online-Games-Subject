#pragma once

// TODO(you): World state replication lab session
//#include <map>
//#define MAX_ACTIONS 256

enum class ReplicationAction
{
	None, Create, Update, Destroy
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
	void Destroy(uint32 networkId);

	void Write(OutputMemoryStream& packet);

	//
	ReplicationCommand actions[MAX_NETWORK_OBJECTS]; // waste, maybe only need the current objects to modify
	//std::map<uint32, ReplicationCommand> actions;
	//ReplicationCommand actions[MAX_ACTIONS];
	//uint8 nextAction;

};