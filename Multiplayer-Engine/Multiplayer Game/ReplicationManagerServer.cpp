#include "Networks.h"

// TODO(you): World state replication lab session
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::Write(OutputMemoryStream& packet, DeliveryManager* deliveryManager, std::list<ReplicationCommand>& commands, std::list<WindowInfo>& windowsInfo, bool isResending)
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
				//Crosshair* ch = dynamic_cast<Crosshair*>(obj->behaviour);
				//packet << ss->hitPoints;
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

			uint8  window_id;
			WindowState state;
			uint32 hitByNetworkId;
			EnemyType currentEnemyType;

			// The data will be filled either with the live info or with the saved info from a dropped packet
			if (!isResending)
			{
				window_id = cbw->window_id;
				state = cbw->state;
				hitByNetworkId = cbw->hitByNetworkId;
				currentEnemyType = cbw->currentEnemyType;
			}
			else
			{
				if (windowsInfo.empty())
				{
					ELOG("Window info that cointained lost data from an old package is not found");
					break;
				}

				// Pop the first value of the list 
				WindowInfo windowInfo = windowsInfo.front(); 
				windowsInfo.erase(windowsInfo.begin());
				LOG("Sending Window Info from the past!");
				window_id = windowInfo.window_id;
				state = windowInfo.state;
				hitByNetworkId = windowInfo.hitByNetworkId;
				currentEnemyType = windowInfo.currentEnemyType;
			}

			// Send the filled data 

			packet << window_id;
			packet << state;
			packet << hitByNetworkId;
			packet << cbwm->enemyScores[(int)currentEnemyType];		

			if (hitByNetworkId != 0)
				LOG("");

			newDelivery->deliveryDelegate = new DeliveryMustSend(newDelivery);
			newDelivery->mustSendCommands.push_back(nextCommand);
			
			// Fill the info of the safety delivery about the window
			WindowInfo windowInfo;
			windowInfo.window_id = window_id;
			windowInfo.state = state;
			windowInfo.hitByNetworkId = hitByNetworkId;
			windowInfo.currentEnemyType = currentEnemyType;

			newDelivery->windowsInfo.push_back(windowInfo);

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