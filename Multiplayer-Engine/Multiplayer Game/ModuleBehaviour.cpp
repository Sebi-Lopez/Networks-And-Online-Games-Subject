#include "Networks.h"
#include "ModuleBehaviour.h"

bool ModuleBehaviour::start()
{
	crosshairTypeRects[0].crosshairType = 0;
	crosshairTypeRects[0].reticle_outside = { 0, 0, 256, 256 };
	crosshairTypeRects[0].reticle_hit = { 256, 0, 256, 256 };

	crosshairTypeRects[1].crosshairType = 1;
	crosshairTypeRects[1].reticle_outside = { 0, 256, 256, 256 };
	crosshairTypeRects[1].reticle_hit = { 256, 256, 256, 256 };

	crosshairTypeRects[2].crosshairType = 2;
	crosshairTypeRects[2].reticle_outside = { 512, 0, 256, 256 };
	crosshairTypeRects[2].reticle_hit = { 512, 256, 256, 256 };

	crosshairTypeRects[3].crosshairType = 3;
	crosshairTypeRects[3].reticle_outside = { 768, 0, 256, 256 };
	crosshairTypeRects[3].reticle_hit = { 768, 256, 256, 256 };

	crosshairTypeRects[4].crosshairType = 4;
	crosshairTypeRects[4].reticle_outside = { 256, 512, 256, 256 };
	crosshairTypeRects[4].reticle_hit = { 512, 512, 256, 256 };

	crosshairTypeRects[5].crosshairType = 5;
	crosshairTypeRects[5].reticle_outside = { 0, 512, 256, 256 };
	crosshairTypeRects[5].reticle_hit = { 0, 768, 256, 256 };


 	return true;
}

bool ModuleBehaviour::update()
{
	/*for (Spaceship &behaviour : spaceships)
	{
		handleBehaviourLifeCycle(&behaviour);
	}
	
	for (Laser &behaviour : lasers)
	{
		handleBehaviourLifeCycle(&behaviour);
	}*/

	for (PlayerCrosshair& behaviour : players_crosshairs)
	{
		handleBehaviourLifeCycle(&behaviour);
	}

	
	handleBehaviourLifeCycle(&winManager);
	


	return true;
}

Behaviour *ModuleBehaviour::addBehaviour(BehaviourType behaviourType, GameObject *parentGameObject)
{
	switch (behaviourType)
	{
	case BehaviourType::crosshair:
		return addCrosshair(parentGameObject);
	case BehaviourType::window_manager:
		return addWinMan(parentGameObject);
	/*case BehaviourType::Laser:
		return addLaser(parentGameObject);*/
	default:
		return nullptr;
	}

	return nullptr;
}

CowboyWindowManager* ModuleBehaviour::addWinMan(GameObject* parentGo)
{
	if (winManager.gameObject == nullptr)
	{
		winManager = {};
		winManager.gameObject = parentGo;
		parentGo->behaviour = &winManager;
		return &winManager;
	}

	ASSERT(false);
	return nullptr;
}

PlayerCrosshair* ModuleBehaviour::addCrosshair(GameObject* parentGo)
{
	for (PlayerCrosshair& behaviour : players_crosshairs)
	{
		if (behaviour.gameObject == nullptr)
		{
			behaviour = {};
			behaviour.gameObject = parentGo;
			parentGo->behaviour = &behaviour;
			return &behaviour;
		}
	}

	ASSERT(false);
	return nullptr;
}

//Spaceship* ModuleBehaviour::addSpaceship(GameObject* parentGameObject)
//{
//	for (Spaceship& behaviour : spaceships)
//	{
//		if (behaviour.gameObject == nullptr)
//		{
//			behaviour = {};
//			behaviour.gameObject = parentGameObject;
//			parentGameObject->behaviour = &behaviour;
//			return &behaviour;
//		}
//	}
//
//	ASSERT(false);
//	return nullptr;
//}

//Spaceship *ModuleBehaviour::addSpaceship(GameObject *parentGameObject)
//{
//	for (Spaceship &behaviour : spaceships)
//	{
//		if (behaviour.gameObject == nullptr)
//		{
//			behaviour = {};
//			behaviour.gameObject = parentGameObject;
//			parentGameObject->behaviour = &behaviour;
//			return &behaviour;
//		}
//	}
//
//	ASSERT(false);
//	return nullptr;
//}

//Laser *ModuleBehaviour::addLaser(GameObject *parentGameObject)
//{
//	for (Laser &behaviour : lasers)
//	{
//		if (behaviour.gameObject == nullptr)
//		{
//			behaviour = {};
//			behaviour.gameObject = parentGameObject;
//			parentGameObject->behaviour = &behaviour;
//			return &behaviour;
//		}
//	}
//
//	ASSERT(false);
//	return nullptr;
//}

void ModuleBehaviour::handleBehaviourLifeCycle(Behaviour *behaviour)
{
	GameObject *gameObject = behaviour->gameObject;

	if (gameObject != nullptr)
	{
		switch (gameObject->state)
		{
		case GameObject::STARTING:
			behaviour->start();
			break;
		case GameObject::UPDATING:
			behaviour->update();
			break;
		case GameObject::DESTROYING:
			behaviour->destroy();
			gameObject->behaviour = nullptr;
			behaviour->gameObject = nullptr;
			break;
		default:;
		}
	}
}

CrosshairRects ModuleBehaviour::GetCrosshairRects(uint8 type) const
{
	if (type > MAX_CROSSHAIR_TYPES)
		return CrosshairRects();

	return crosshairTypeRects[type];
}

std::vector<PlayerCrosshair*> ModuleBehaviour::GetPlayersCrosshairs()
{
	std::vector<PlayerCrosshair*> disp;

	for (PlayerCrosshair& behaviour : players_crosshairs)
	{
		if (behaviour.gameObject == nullptr)
			continue;

		disp.push_back(&behaviour);
	}

	return disp;
}