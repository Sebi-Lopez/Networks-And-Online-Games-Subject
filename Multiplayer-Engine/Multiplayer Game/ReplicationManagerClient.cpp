#include "Networks.h"

// TODO(you): World state replication lab session
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::Read(const InputMemoryStream& packet)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 networkId = 0;
		ReplicationAction action;
		packet >> networkId;
		packet >> action;

		switch (action)
		{
		case ReplicationAction::None:
			break;
		case ReplicationAction::Create:
		{
			CreateObj(packet, networkId);
			break;
		}
		case ReplicationAction::Update:
		{
			UpdateObj(packet, networkId);
			break;
		}
		case ReplicationAction::Destroy:
		{
			// check if its our spaceship
			if (networkId == App->modNetClient->GetNetworkID())
			{
				ELOG("Your ship should be destroyed.");
				WLOG("KICKED: you are killed");
				NetworkDisconnect();
				//App->modNetClient->disconnect();
			}

			GameObject* obj = App->modLinkingContext->getNetworkGameObject(networkId);
			if (obj == nullptr)
			{
				WLOG("GAMEOBJECT TO DESTROY NOT FOUND!");
				break;
			}
						
			App->modLinkingContext->unregisterNetworkGameObject(obj);
			Destroy(obj);
			break;
		}
		default:
			break;
		}

	}
}

void ReplicationManagerClient::UpdateObj(const InputMemoryStream& packet, const uint32 networkId)
{
	NetEntityType type;
	packet >> type;

	// We need to get the data out, so the reading of the next 
	// replication isnt corrupted in case there is no object

	vec2 position;
	float angle;
	uint8 hitPoints;

	switch (type)
	{
	case NetEntityType::None:
		break;
	case NetEntityType::Spaceship:
	{
		packet >> hitPoints;

		packet >> position.x;
		packet >> position.y;
		packet >> angle;
		break;
	}
	case NetEntityType::Laser:
	{
		packet >> position.x;
		packet >> position.y;
		packet >> angle;
		break;
	}
	default:
		break;
	}

	// Once we got the data out, we figure out if the object is already created 
	// If there's no object, we are still waiting for the creation packet that might be dropped 

	GameObject* obj2U = App->modLinkingContext->getNetworkGameObject(networkId);
	if (obj2U == nullptr)
	{
		ELOG("Replication Client: GAMEOBJECT TO UPDATE NOT FOUND!");
		return;
	}

	switch (type)
	{
	case NetEntityType::None:
		break;
	case NetEntityType::Spaceship:
	{
		Spaceship* ss = dynamic_cast<Spaceship*>(obj2U->behaviour);
		ss->hitPoints = hitPoints;

		obj2U->position = position;
		obj2U->angle = angle;
		break;
	}
	case NetEntityType::Laser:
	{
		obj2U->position = position;
		obj2U->angle = angle;
		break;
	}
	default:
		break;
	}

}

void ReplicationManagerClient::CreateObj(const InputMemoryStream& packet, const uint32 networkId)
{
	NetEntityType type;
	packet >> type;

	// Check if the array of network game objects is full there 
	// In any case, if they sent you to create an obj, that means that the server 
	// destroyed the object that was there, and the packet was lost, so it will be destroyed later anyways 

	GameObject* obj = App->modLinkingContext->getNetworkGameObject(networkId, false);
	if (obj != nullptr)
	{
		LOG("There is an object in the position you are trying to create a new one. Deleting.");
		App->modLinkingContext->unregisterNetworkGameObject(obj);
		Destroy(obj);
	}


	switch (type)
	{
	case NetEntityType::Spaceship:
	{
		GameObject* newObj = App->modGameObject->Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(newObj, networkId);
		newObj->netType = NetEntityType::Spaceship;

		uint8 spaceshipType = 0;
		packet >> spaceshipType;
		packet >> newObj->position.x;
		packet >> newObj->position.y;
		packet >> newObj->angle;
		newObj->size = { 100, 100 };

		// Create sprite
		newObj->sprite = App->modRender->addSprite(newObj);
		newObj->sprite->order = 5;

		if (spaceshipType == 0) {
			newObj->sprite->texture = App->modResources->spacecraft1;
		}
		else if (spaceshipType == 1) {
			newObj->sprite->texture = App->modResources->spacecraft2;
		}
		else {
			newObj->sprite->texture = App->modResources->spacecraft3;
		}


		// Create collider
		//newObj->collider = App->modCollision->addCollider(ColliderType::Player, newObj);
		//newObj->collider->isTrigger = true; // NOTE(jesus): This object will receive onCollisionTriggered events

		// Create behaviour
		Spaceship* spaceshipBehaviour = App->modBehaviour->addSpaceship(newObj);
		newObj->behaviour = spaceshipBehaviour;
		newObj->behaviour->isServer = false;
		break;
	}
	case NetEntityType::Laser:
	{
		GameObject* laser = App->modGameObject->Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(laser, networkId);
		laser->netType = NetEntityType::Laser;

		packet >> laser->position.x;
		packet >> laser->position.y;
		packet >> laser->angle;
		laser->size = { 20, 60 };

		laser->sprite = App->modRender->addSprite(laser);
		laser->sprite->order = 3;
		laser->sprite->texture = App->modResources->laser;

		Laser* laserBehaviour = App->modBehaviour->addLaser(laser);
		laserBehaviour->isServer = false;

		//laser->tag = gameObject->tag;
		break;
	}
	case NetEntityType::Explosion:
	{
		GameObject* explosion = Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(explosion, networkId);
		explosion->netType = NetEntityType::Explosion;
	
		packet >> explosion->size.x;
		packet >> explosion->size.y;

		packet >> explosion->position.x;
		packet >> explosion->position.y;
		packet >> explosion->angle;

		explosion->sprite = App->modRender->addSprite(explosion);
		explosion->sprite->texture = App->modResources->explosion1;
		explosion->sprite->order = 100;

		explosion->animation = App->modRender->addAnimation(explosion);
		explosion->animation->clip = App->modResources->explosionClip;
		break;
	}
	default:
	{
		break;
	}
	}
}

