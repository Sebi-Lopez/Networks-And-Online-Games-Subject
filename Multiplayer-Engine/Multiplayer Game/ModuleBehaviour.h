#pragma once

#include "Behaviours.h"

#define MAX_CROSSHAIR_TYPES 6


class ModuleBehaviour : public Module
{
public:

	bool start() override;
	bool update() override;

	Behaviour * addBehaviour(BehaviourType behaviourType, GameObject *parentGameObject);

	PlayerCrosshair* addCrosshair(GameObject* parentGo);
	CowboyWindowManager* addWinMan(GameObject* parentGo);
	/*Spaceship * addSpaceship(GameObject *parentGameObject);
	Laser     * addLaser(GameObject *parentGameObject);*/

	CrosshairRects GetCrosshairRects(uint8 type) const;
	std::vector<PlayerCrosshair*> GetPlayersCrosshairs();


private:

	void handleBehaviourLifeCycle(Behaviour * behaviour);


	PlayerCrosshair players_crosshairs[MAX_CLIENTS];
	CowboyWindowManager winManager; // each client create its manager on screengame start, but only server side send orders to all clients
	
	/*Spaceship spaceships[MAX_CLIENTS];
	Laser lasers[MAX_GAME_OBJECTS];*/

	// store disponible cross hair type rects and type
	CrosshairRects crosshairTypeRects[MAX_CROSSHAIR_TYPES];

};

