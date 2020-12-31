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
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(nextCommand.networkId);
			if (obj == nullptr) {
				ELOG("Replication create server: Object not found");
				break;
			}

			packet << obj->networkId;
			packet << nextCommand.action;
			NetEntityType type = obj->netType;
			packet << type;

			if (type == NetEntityType::Crosshair)
			{
				PlayerCrosshair* behaviour = dynamic_cast<PlayerCrosshair*>(obj->behaviour);
				packet << behaviour->reticle.crosshairType;
				packet << behaviour->playerName;
				
			}
			else if (type == NetEntityType::CowboyWindow)
			{
				//CowboyWindow* cbw = dynamic_cast<CowboyWindow*>(obj);
				//
				//ELOG("");
				
			}
			/*else if (type == NetEntityType::Explosion)
			{
				packet << obj->size.x;
				packet << obj->size.y;
			}*/

			packet << obj->position.x;
			packet << obj->position.y;
			//packet << obj->angle;

			newDelivery->deliveryDelegate = new DeliveryMustSend(newDelivery);
			newDelivery->mustSendCommands.push_back(nextCommand);
			break;
		}
		case ReplicationAction::Update:
		{
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(nextCommand.networkId);

			if (obj == nullptr) {
				ELOG("Replication UPDATE server: Object not found");
				break;
			}

			packet << obj->networkId;
			packet << nextCommand.action;
			NetEntityType type = obj->netType;
			packet << type;
			if (type == NetEntityType::Crosshair)
			{
				PlayerCrosshair* pc = dynamic_cast<PlayerCrosshair*>(obj->behaviour);
				packet << pc->score;
				packet << pc->ready;

				if (pc->ready)
					LOG("");

			}

			packet << obj->position.x;
			packet << obj->position.y;
			//packet << obj->angle;
			break;
		}
		case ReplicationAction::CowboyOrder:
		{
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(nextCommand.networkId);
			CowboyWindowManager* cbwm = dynamic_cast<CowboyWindowManager*>(App->modScreen->screenGame->windowManager->behaviour);
			CowboyWindow* cbw = cbwm->GetCowboyWindowWithNetworkId(obj->networkId);

			packet << obj->networkId;
			packet << nextCommand.action;
			NetEntityType type = obj->netType;
			packet << type;

			packet << cbw->window_id;
			packet << cbw->state;
			packet << cbw->currentEnemyType;
			packet << cbw->hitByNetworkId;
			packet << cbwm->enemyScores[(int)cbw->currentEnemyType];		

			if (cbw->hitByNetworkId != 0)
				LOG("");

			newDelivery->deliveryDelegate = new DeliveryMustSend(newDelivery);
			newDelivery->mustSendCommands.push_back(nextCommand);

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

void ReplicationManagerServer::UpdateCowboyWindow(uint32 networkId)
{
	ReplicationCommand com;
	com.action = ReplicationAction::CowboyOrder;
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