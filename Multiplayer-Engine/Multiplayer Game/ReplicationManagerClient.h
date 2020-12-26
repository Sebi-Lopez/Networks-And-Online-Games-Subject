#pragma once

// TODO(you): World state replication lab session

class ReplicationManagerClient
{
public:
	void Read(const InputMemoryStream& packet);
	void CreateObj(const InputMemoryStream& packet, const uint32 networkId);
	void UpdateObj(const InputMemoryStream& packet, const uint32 networkId);
};