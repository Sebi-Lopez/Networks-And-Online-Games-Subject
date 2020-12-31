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
		case ReplicationAction::CowboyOrder:
		{
			NetEntityType type = NetEntityType::None;
			uint8 window_idx = 0;
			WindowState wstate = WindowState::none;
			uint32 hitNetId = 0;
			int score = 0;
			EnemyType enemyType;

			packet >> type;
			packet >> window_idx;
			packet >> wstate;
			packet >> enemyType;
			packet >> hitNetId;
			packet >> score;

			CowboyWindowManager* winMan = dynamic_cast<CowboyWindowManager*>(App->modScreen->screenGame->windowManager->behaviour);
			if (wstate == WindowState::open) 
				winMan->windows[window_idx].Open(enemyType);
			else if (wstate == WindowState::closed)
				winMan->windows[window_idx].Close();

			// add score to whatever player pertains
			if (hitNetId != 0)
			{
				GameObject* pcgo = App->modLinkingContext->getNetworkGameObject(hitNetId);
				PlayerCrosshair* pc = dynamic_cast<PlayerCrosshair*>(pcgo->behaviour);
				pc->score += score;
				LOG("INCREMENT SCORE: %i", score);

				if (pcgo->networkId == App->modNetClient->GetNetworkID())
				{
					GameObject* bloodSplash = Instantiate();
					//App->modLinkingContext->registerNetworkGameObjectWithNetworkId(bloodSplash, networkId);
					bloodSplash->netType = NetEntityType::Blood;
					bloodSplash->position.x = pc->gameObject->position.x;
					bloodSplash->position.y = pc->gameObject->position.y;
					//particleShot->angle = gameObject->angle;
					bloodSplash->size = { 125, 125 };

					bloodSplash->sprite = App->modRender->addSprite(bloodSplash);
					bloodSplash->sprite->texture = App->modResources->blood;

					bloodSplash->animation = App->modRender->addAnimation(bloodSplash);
					bloodSplash->animation->clip = App->modResources->bloodSplash;

					bloodSplash->sprite->order = 150;
					Destroy(bloodSplash, 5.0f);

					// TOAUDIO: blood splash
					if(score > 0)
						App->modSound->playAudioClip(App->modResources->audioClipManHit);
					else if(score == -150)
						App->modSound->playAudioClip(App->modResources->audioClipWomanHit);
					else 
						App->modSound->playAudioClip(App->modResources->audioClipMexicanHit);
				}
			}

			break;
		}
		case ReplicationAction::Destroy:
		{
			// check if its our spaceship
			if (networkId == App->modNetClient->GetNetworkID())
			{
				/*ELOG("Your ship should be destroyed.");
				WLOG("KICKED: you are killed");
				NetworkDisconnect();*/
				//App->modNetClient->disconnect();
			}

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

	// We need to get the data out, so the reading of the next 
	// replication isnt corrupted in case there is no object

	vec2 position;
	float angle;
	uint8 hitPoints;
	int score = 0;
	bool ready = false;
	GameState gamestate = GameState::none;

	switch (type)
	{
	case NetEntityType::None:
		break;
	case NetEntityType::Crosshair:
	{
		packet >> score;
		packet >> ready;
		packet >> gamestate;

		packet >> position.x;
		packet >> position.y;
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
	case NetEntityType::Crosshair:
	{

		//Crosshair* ch = dynamic_cast<Crosshair*>(obj2U->behaviour);
		//packet >> ss->hitPoints;
		
		obj2U->position = position;
		PlayerCrosshair* pc = dynamic_cast<PlayerCrosshair*>(obj2U->behaviour);
		pc->score = score;
		pc->ready = ready;

		// get winman and adjust the gamestate and started time
		GameObject* winMan = App->modScreen->screenGame->windowManager;
		if (winMan != nullptr)
		{
			CowboyWindowManager* cbwm = dynamic_cast<CowboyWindowManager*>(winMan->behaviour);

			if (cbwm != nullptr)
			{
				// if our state is other tan started, start if we receive started
				GameState thisClientState = cbwm->gameLoopState;

				if (gamestate == GameState::started && thisClientState != GameState::started)
				{
					cbwm->gameLoopState = GameState::started;
					cbwm->game_started_at = Time.time; // NOTE: Little delay here
				}

			}
		}

		

		//packet >> obj2U->angle;

		if (using_entity_interpolation) {
			obj2U->UpdateInterpolationValues(position, 0.f);
		}
		else
		{
			obj2U->position = position;
		}

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
	case NetEntityType::Crosshair:
	{
		GameObject* newObj = App->modGameObject->Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(newObj, networkId);
		newObj->netType = NetEntityType::Crosshair;

		std::string playerName;
		uint8 crosshairType = 0;
		packet >> crosshairType;
		packet >> playerName;
		packet >> newObj->position.x;
		packet >> newObj->position.y;
		
		newObj->initial_pos = newObj->position;
		newObj->final_pos = newObj->position;

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
		crossHairBh->playerName = playerName;
		newObj->behaviour = crossHairBh;


		break;
	}
	case NetEntityType::CowboyWindow:
	{
		// we assume sequentally the windows to fill
		GameObject* targetWindow = dynamic_cast<CowboyWindowManager*>(App->modScreen->screenGame->windowManager->behaviour)->GetNextCowWindow();
		  
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(targetWindow, networkId);
		targetWindow->netType = NetEntityType::CowboyWindow;

		packet >> targetWindow->position.x; // TODO: REMOVE position, not needed, is fixed on all clients now
		packet >> targetWindow->position.y;

		break;
	}
	case NetEntityType::Shoot:
	{
		GameObject* particleShot = Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(particleShot, networkId);
		particleShot->netType = NetEntityType::Shoot;
		packet >> particleShot->position.x;
		packet >> particleShot->position.y;
		//particleShot->angle = gameObject->angle;
		particleShot->size = { 50, 50 };

		particleShot->sprite = App->modRender->addSprite(particleShot);
		particleShot->sprite->texture = App->modResources->explosion1;
		particleShot->sprite->order = 100;

		particleShot->animation = App->modRender->addAnimation(particleShot);
		particleShot->animation->clip = App->modResources->explosionClip;

		App->modSound->playAudioClip(App->modResources->audioClipShot);
		break;
	}

	case NetEntityType::Blood:
	{
		GameObject* bloodSplash = Instantiate();
		App->modLinkingContext->registerNetworkGameObjectWithNetworkId(bloodSplash, networkId);
		bloodSplash->netType = NetEntityType::Blood;
		packet >> bloodSplash->position.x;
		packet >> bloodSplash->position.y;
		//particleShot->angle = gameObject->angle;
		bloodSplash->size = { 125, 125 };

		bloodSplash->sprite = App->modRender->addSprite(bloodSplash);
		bloodSplash->sprite->texture = App->modResources->blood;

		bloodSplash->animation = App->modRender->addAnimation(bloodSplash);
		bloodSplash->animation->clip = App->modResources->bloodSplash;

		bloodSplash->sprite->order = 150;
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

