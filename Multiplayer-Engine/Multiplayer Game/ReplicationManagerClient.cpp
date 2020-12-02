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
						//  Type
						newGameObject->type = ObjectType::Player;

						// Texture
						newGameObject->sprite = App->modRender->addSprite(newGameObject);
						newGameObject->sprite->order = 5;
						// Should send space type
						newGameObject->sprite->texture = App->modResources->spacecraft1;

						// Create collider
						// Should we add col?
						/*newGameObject->collider = App->modCollision->addCollider(ColliderType::Player, newGameObject);
						newGameObject->collider->isTrigger = true; */

						// Create behaviour
						Spaceship* spaceshipBehaviour = App->modBehaviour->addSpaceship(newGameObject);
						newGameObject->behaviour = spaceshipBehaviour;

					} break;

					case ObjectType::Laser:
					{
						// Type
						newGameObject->type = ObjectType::Laser;

						// Texture
						newGameObject->sprite = App->modRender->addSprite(newGameObject);
						newGameObject->sprite->order = 3;
						newGameObject->sprite->texture = App->modResources->laser;

						// Create behaviour
						App->modBehaviour->addLaser(newGameObject);
					} break;
					
					case ObjectType::Explosion:
					{
						// Type
						newGameObject->type = ObjectType::Explosion;

						// Texture
						newGameObject->sprite = App->modRender->addSprite(newGameObject);
						newGameObject->sprite->order = 100;
						newGameObject->sprite->texture = App->modResources->explosion1;

						//Animation
						newGameObject->animation = App->modRender->addAnimation(newGameObject);
						newGameObject->animation->clip = App->modResources->explosionClip;
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
