#pragma once

#include "Behaviours.h"

#define MAX_CROSSHAIR_TYPES 3

class ModuleBehaviour : public Module
{
public:

	bool start() override;
	bool update() override;

	Behaviour * addBehaviour(BehaviourType behaviourType, GameObject *parentGameObject);

	PlayerCrosshair* addCrosshair(GameObject* parentGo);
	/*Spaceship * addSpaceship(GameObject *parentGameObject);
	Laser     * addLaser(GameObject *parentGameObject);*/

	CrosshairRects GetCrosshairRects(uint8 type) const;

private:

	void handleBehaviourLifeCycle(Behaviour * behaviour);


	PlayerCrosshair players_crosshairs[MAX_CLIENTS];
	/*Spaceship spaceships[MAX_CLIENTS];
	Laser lasers[MAX_GAME_OBJECTS];*/

	//
	CrosshairRects crosshairTypeRects[MAX_CROSSHAIR_TYPES];
};

