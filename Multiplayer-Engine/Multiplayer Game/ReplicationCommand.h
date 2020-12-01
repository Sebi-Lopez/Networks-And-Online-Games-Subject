#pragma once

// TODO(you): World state replication lab session

enum class ReplicationAction {
	None = 0, 
	Create_Obj,
	Update_Obj,
	Destroy_Obj
};

struct ReplicationCommand 
{
	ReplicationAction action = ReplicationAction::None;
	uint32 networkID = 0;

	ReplicationCommand(ReplicationAction action, uint32 id)
		: action(action), networkID(id) {}
};