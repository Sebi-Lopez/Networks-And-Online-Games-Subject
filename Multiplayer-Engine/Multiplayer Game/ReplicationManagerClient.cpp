#include "Networks.h"
#include "ReplicationManagerClient.h"

// TODO(you): World state replication lab session

void ReplicationManagerClient::ReadReplication(const InputMemoryStream& packet)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 networkId = 0;
		ReplicationAction action = ReplicationAction::None;

		packet >> networkId;
		packet >> action;

		switch (action)
		{
			case ReplicationAction::Update_Obj:
			{
				GameObject* object = App->modLinkingContext->getNetworkGameObject(networkId);

				if (object == nullptr)
				{
					WLOG("Couldnt find an object with id: %i", networkId);
				}
				LOG("Updated Object with ID: %i", networkId);

				packet >> object->position.x;
				packet >> object->position.y;

				packet >> object->size.x;
				packet >> object->size.y;

				packet >> object->angle;
					
			} break;

			case ReplicationAction::Create_Obj:
			{
				GameObject* newGameObject = App->modGameObject->Instantiate();
				ObjectType type = ObjectType::Other;

				App->modLinkingContext->registerNetworkGameObjectWithNetworkId(newGameObject, networkId);
				
				LOG("Created Object with ID: %i", networkId);

				packet >> newGameObject->position.x;
				packet >> newGameObject->position.y;

				packet >> newGameObject->size.x;
				packet >> newGameObject->size.y;

				packet >> newGameObject->angle;

				packet >> type;

				switch (type)
				{
					case ObjectType::Player:
					{
						newGameObject->type = ObjectType::Player;
						newGameObject->sprite = App->modRender->addSprite(newGameObject);
						newGameObject->sprite->order = 5;
						newGameObject->sprite->texture = App->modResources->spacecraft1;
					} break;

					case ObjectType::Laser:
					{
						newGameObject->type = ObjectType::Laser;
						newGameObject->sprite = App->modRender->addSprite(newGameObject);
						newGameObject->sprite->order = 3;
						newGameObject->sprite->texture = App->modResources->laser;
					} break;
					
					case ObjectType::Explosion:
					{
						
					} break;
				}
			} break;

			case ReplicationAction::Destroy_Obj:
			{
				LOG("Destroyed Object with ID: %i", networkId);
				GameObject* toDestroyObject = App->modLinkingContext->getNetworkGameObject(networkId);
				App->modLinkingContext->unregisterNetworkGameObject(toDestroyObject);
				App->modGameObject->Destroy(toDestroyObject);
			} break;

			case ReplicationAction::None:
			{
			} break;

			default:
				break;
			}
	}
}
