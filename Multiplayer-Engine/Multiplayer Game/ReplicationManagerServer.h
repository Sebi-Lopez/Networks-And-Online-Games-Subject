#pragma once

// TODO(you): World state replication lab session

class ReplicationManagerServer 
{
public: 

	void CreateObject(uint32 networkID);
	void UpdateObject(uint32 networkID);
	void DestroyObject(uint32 networkID);

	void WriteReplication(OutputMemoryStream& packet);

public: 

	std::vector<ReplicationCommand> replication_list; 
	
	// ToPeter: Should every proxy has its own timer like this or should we send them all at once ??
	float lastReplicationSent = 0.0f; 

};