#include "Networks.h"

// TODO(you): World state replication lab session
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::Write(OutputMemoryStream& packet, DeliveryManager* deliveryManager, std::list<ReplicationCommand>& commands)
{
	Delivery* newDelivery = deliveryManager->WriteSequenceNumber(packet);

	for (std::list<ReplicationCommand>::iterator iter = commands.begin(); iter != commands.end();)
	{
		ReplicationCommand nextCommand = (*iter);

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
			packet << nextCommand.action;
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

			newDelivery->deliveryDelegate = new DeliveryMustSend(newDelivery);
			newDelivery->mustSendCommands.push_back(nextCommand);

			break;
		}
		case ReplicationAction::Update:
		{
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(nextCommand.networkId);

			packet << obj->networkId;
			packet << nextCommand.action;
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
			packet << nextCommand.action;

			newDelivery->deliveryDelegate = new DeliveryMustSend(newDelivery);
			newDelivery->mustSendCommands.push_back(nextCommand);

			break;
		}
		default:
			break;
		} 

		iter = commands.erase(iter);
	}
}

// TODO: needs to copy paste but only changes the action?
void ReplicationManagerServer::Create(uint32 networkId)
{
	ReplicationCommand com;
	com.action = ReplicationAction::Create;
	com.networkId = networkId;

	commandsList.push_back(com);
}

void ReplicationManagerServer::Update(uint32 networkId)
{
	ReplicationCommand com;
	com.action = ReplicationAction::Update;
	com.networkId = networkId;

	commandsList.push_back(com);
}

void ReplicationManagerServer::Destroy(uint32 networkId)
{
	ReplicationCommand com;
	com.action = ReplicationAction::Destroy;
	com.networkId = networkId;
		
	commandsList.push_back(com);
}