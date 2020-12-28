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
			GameObject* obj = App->modLinkingContext->getNetworkGameObject(networkId);
			if (obj == nullptr)
			{
				WLOG("GAMEOBJECT TO DESTROY NOT FOUND!");
				break;
			}
			// check if is our spaceship
			
			if (obj->netType == NetEntityType::Crosshair)
			{
				//if (obj->networkId == App->modNetClient->GetNetworkID())
				//{
				//	//WLOG("KICKED: you are killed");
				//	//NetworkDisconnect();
				//	//App->modNetClient->disconnect();
				//}
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
	case NetEntityType::Crosshair:
	{

		//Crosshair* ch = dynamic_cast<Crosshair*>(obj2U->behaviour);
		//packet >> ss->hitPoints;
		
		packet >> obj2U->position.x;
		packet >> obj2U->position.y;
		//packet >> obj2U->angle;
		break;
	}
	/*case NetEntityType::Laser:
	{
		packet >> obj2U->position.x;
		packet >> obj2U->position.y;
		packet >> obj2U->angle;
		break;
	}*/
	default:
		break;
	}

}

void ReplicationManagerClient::CreateObj(const InputMemoryStream& packet, const uint32 networkId)
{
	NetEntityType type;
	packet >> type;

	switch (type)
	{
	case NetEntityType::Crosshair:
	{
		GameObject* newObj = App->modGameObject->Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(newObj, networkId);
		newObj->netType = NetEntityType::Crosshair;

		uint8 crosshairType = 0;
		packet >> crosshairType;
		packet >> newObj->position.x;
		packet >> newObj->position.y;
		//packet >> newObj->angle;
		newObj->size = { 100, 100 };

		// Create sprite
		newObj->sprite = App->modRender->addSprite(newObj);
		newObj->sprite->order = 5;
		newObj->sprite->texture = App->modResources->tex_crosshairs_ss;
		newObj->sprite->clipRect = App->modBehaviour->GetCrosshairRects(crosshairType).reticle_outside;
		

		// Create collider
		//newObj->collider = App->modCollision->addCollider(ColliderType::Player, newObj);
		//newObj->collider->isTrigger = true; // NOTE(jesus): This object will receive onCollisionTriggered events

		// Create behaviour
		PlayerCrosshair* crossHairBh = App->modBehaviour->addCrosshair(newObj);
		crossHairBh->reticle = App->modBehaviour->GetCrosshairRects(crosshairType);
		newObj->behaviour = crossHairBh;


		break;
	}
	//case NetEntityType::Laser:
	//{
	//	GameObject* laser = App->modGameObject->Instantiate();
	//	App->modLinkingContext->registerNetworkGameObjectWithNetworkId(laser, networkId);
	//	laser->netType = NetEntityType::Laser;

	//	packet >> laser->position.x;
	//	packet >> laser->position.y;
	//	packet >> laser->angle;
	//	laser->size = { 20, 60 };

	//	laser->sprite = App->modRender->addSprite(laser);
	//	laser->sprite->order = 3;
	//	laser->sprite->texture = App->modResources->laser;

	//	Laser* laserBehaviour = App->modBehaviour->addLaser(laser);
	//	laserBehaviour->isServer = false;

	//	//laser->tag = gameObject->tag;
	//	break;
	//}
	/*case NetEntityType::Explosion:
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
	}*/
	default:
	{
		break;
	}
	}
}

