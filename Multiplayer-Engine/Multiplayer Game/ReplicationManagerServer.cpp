#include "Networks.h"

// TODO(you): World state replication lab session
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::Write(OutputMemoryStream& packet)
{
	//TODO(sebi): delivery inside the replication manager to have access to the "important" information

	for (int i = 0; i < MAX_NETWORK_OBJECTS; ++i) // TODO: is really needed all objects?
	{
		ReplicationCommand nextCommand = actions[i];

		switch (nextCommand.action)
		{
		case ReplicationAction::Create:
		{
			// TODO: make templatized creation class
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(nextCommand.networkId);
			if (obj == nullptr) {
				ELOG("Replication create server: Object not found");
				return;
			}

			packet << obj->networkId;
			packet << actions[i].action;
			NetEntityType type = obj->netType;
			packet << type;
			if (type == NetEntityType::Spaceship)
			{
				Spaceship* ss = dynamic_cast<Spaceship*>(obj->behaviour);
				packet << ss->spaceShipType;
			}
			else if (type == NetEntityType::Explosion)
			{
				packet << obj->size.x;
				packet << obj->size.y;
			}

			packet << obj->position.x;
			packet << obj->position.y;
			packet << obj->angle;

			break;
		}
		case ReplicationAction::Update:
		{
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(nextCommand.networkId);

			packet << obj->networkId;
			packet << actions[i].action;
			NetEntityType type = obj->netType;
			packet << type;
			if (type == NetEntityType::Spaceship)
			{
				Spaceship* ss = dynamic_cast<Spaceship*>(obj->behaviour);
				packet << ss->hitPoints;
			}

			packet << obj->position.x;
			packet << obj->position.y;
			packet << obj->angle;
			break;
		}
		case ReplicationAction::Destroy:
		{
			packet << nextCommand.networkId;
			packet << actions[i].action;

			break;
		}
		default:
			break;
		} 

		actions[i] = {};
	}
}

// TODO: needs to copy paste but only changes the action?
void ReplicationManagerServer::Create(uint32 networkId)
{
	uint16 arrayIndex = networkId & 0xffff;
	ReplicationCommand com;
	com.action = ReplicationAction::Create;
	com.networkId = networkId;
	actions[arrayIndex] = com;
}

void ReplicationManagerServer::Update(uint32 networkId)
{
	uint16 arrayIndex = networkId & 0xffff;
	ReplicationCommand com;
	com.action = ReplicationAction::Update;
	com.networkId = networkId;
	actions[arrayIndex] = com;
}

void ReplicationManagerServer::Destroy(uint32 networkId)
{
	uint16 arrayIndex = networkId & 0xffff;
	ReplicationCommand com;
	com.action = ReplicationAction::Destroy;
	com.networkId = networkId;
	actions[arrayIndex] = com;
}