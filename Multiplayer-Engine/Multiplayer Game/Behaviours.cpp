#include "Networks.h"
#include "Behaviours.h"



void CowboyWindowManager::start()
{
	collisionRect = { 40, 38, 75, 90 }; // x and y are the offsets respect 0,0 from "opened window" size

	// create necessary rects for window and different enemies/hostages
	vec4 windowsPositions[MAX_SPAWN_WINDOWS];
	windowsPositions[0] = { 354, 0 };
	windowsPositions[1] = { 662, 0 };
	windowsPositions[2] = { 946, 0 };
	windowsPositions[3] = { 380, 308 };
	windowsPositions[4] = { 946, 308 };

	for (int i = 0; i < MAX_SPAWN_WINDOWS; ++i)
	{
		if(isServer)
			windows[i].window = NetworkInstantiate();
		else
			windows[i].window = Instantiate();

		//windows[i].window->position = { windowsPositions[i].x - 1280*0.5f, windowsPositions[i].y - 720*0.5f };
		windows[i].window->position = { windowsPositions[i].x, windowsPositions[i].y };
		
		windows[i].window->sprite = App->modRender->addSprite(windows[i].window);
		windows[i].window->sprite->texture = App->modResources->tex_cowboy_window;
		windows[i].window->sprite->clipRect = { 358, 358, 154, 154};
		windows[i].window->sprite->pivot = { 0,0 };
		windows[i].window->size = { 154,154 };
		windows[i].window->sprite->order = 3;
		
		windows[i].winMan = this;
		windows[i].window_id = (uint8)i;
		
		windows[i].window->netType = NetEntityType::CowboyWindow;
	}

	// TODO: fill with all rects from enemies/hostages

	// targets
	targetsRects[(int)EnemyType::bad1].spawnRect = { 0,0, 113, 129 };
	targetsRects[(int)EnemyType::bad3].spawnRect = { 0, 129, 113, 129 };
	targetsRects[(int)EnemyType::bad2].spawnRect = { 0, 258, 113, 129 };

	// Hostages
	targetsRects[(int)EnemyType::hostage1].spawnRect = { 261, 0, 113, 129 };
	targetsRects[(int)EnemyType::hostage2].spawnRect = { 256, 129, 113, 129 };


	// enemy score points
	enemyScores[(int)EnemyType::bad1] = 30;
	enemyScores[(int)EnemyType::bad2] = 60;
	enemyScores[(int)EnemyType::bad3] = 120;
	enemyScores[(int)EnemyType::hostage1] = -150;
	enemyScores[(int)EnemyType::hostage2] = -300;


	CloseAllWindows();
}

void CowboyWindowManager::CloseAllWindows()
{
	for (int i = 0; i < MAX_SPAWN_WINDOWS; ++i)
		windows[i].Close();
	
}

void CowboyWindowManager::OpenWindow(uint8 n)
{
	windows[n].Open();
}

void CowboyWindowManager::onMouse(const MouseController& mouse)
{

}

void CowboyWindowManager::update()
{


	if (!isServer)
		return;

	GameLoopUpdate();

}

bool CowboyWindowManager::CheckMouseClickCollision(vec2 clickPos, int& winIdx) const
{
	// iterate all windows and check
	for (int i = 0; i < MAX_SPAWN_WINDOWS; ++i)
	{
		// get rectified area with collision rect
		vec2 windowPos = windows[i].window->position;
		windowPos += { collisionRect.x, collisionRect.y};
		vec2 windowSize = {collisionRect.z, collisionRect.w};

		// now check the position
		if (clickPos.x > windowPos.x && clickPos.x < windowPos.x + windowSize.x &&
			clickPos.y > windowPos.y && clickPos.y < windowPos.y + windowSize.y) 
		{
			LOG("CLICKED ON WINDOW %i", i);
			winIdx = i;
			return true;
		}
		
	}

	return false;
}

CowboyWindow* CowboyWindowManager::GetCowboyWindowWithNetworkId(uint32 networkId)
{
	for (int i = 0; i < MAX_SPAWN_WINDOWS; ++i)
	{
		if (windows[i].window->networkId == networkId)
		{
			return &windows[i];
		}
	}

	return nullptr;
}

GameObject* CowboyWindowManager::GetNextCowWindow()
{
	if (wIdx > MAX_SPAWN_WINDOWS)
		wIdx = 0;
	return windows[wIdx++].window;
}

void CowboyWindowManager::SpawnLogic()
{
	if (current_opened_windows < max_opened_windows)
	{	
		// TODO: filter by random possibility + interval from last spawn
		if (last_window_closed_time + min_interval_between_spawns_close > Time.time ||
			last_window_opened_time + min_interval_between_spawns_open > Time.time)
			return;

		// collect disponible windows to spawn
		std::vector<int> dispWindows;
		for (uint8 i = 0; i < MAX_SPAWN_WINDOWS; ++i)	{
			if (windows[i].state == WindowState::closed)
				dispWindows.push_back(i);
		}

		// get one random disponible window
		int dispSize = dispWindows.size();
		int vecPos = last_window_closed_id;

		while(vecPos == last_window_closed_id)
			vecPos = (int)floor(Random.next() * dispSize);
	
		windows[dispWindows[vecPos]].Open();
	}
}

void CowboyWindowManager::GameLoopUpdate() // server side
{
	switch (gameLoopState)
	{
	case GameState::none:
	{
		// obtain all connected players
		std::vector<GameObject*> players = App->modNetServer->GetAllConnectedPlayers();

		if (players.size() < 1) // we need at least 1 player to play
			break;

		for (int i = 0; i < players.size(); ++i)
		{
			PlayerCrosshair* pc = dynamic_cast<PlayerCrosshair*>(players[i]->behaviour);
			if (!pc->ready)
				return;
		}

		// iterate another time the connected players to force false the ready for next round
		for (int i = 0; i < players.size(); ++i)
		{
			PlayerCrosshair* pc = dynamic_cast<PlayerCrosshair*>(players[i]->behaviour);
			pc->ready = false;
			pc->score = 0;
			NetworkUpdate(pc->gameObject);
		}

		gameLoopState = GameState::started;
		game_started_at = Time.time;

		break;
	}
	case GameState::started:
	{
		SpawnLogic();
		UpdateActiveWindows();

		if (Time.time > game_started_at + game_duration)
		{
			gameLoopState = GameState::none;
			CloseAllWindows();

		}

		break;
	}
	default:
		break;
	}
}

void CowboyWindowManager::UpdateActiveWindows()
{
	for (uint8 i = 0; i < MAX_SPAWN_WINDOWS; ++i)
	{
		if (windows[i].state == WindowState::open)
			windows[i].Update();
	}
}

void CowboyWindowManager::destroy()
{

}

void CowboyWindowManager::write(OutputMemoryStream& packet)
{
	// WIP
}

void CowboyWindowManager::read(const InputMemoryStream& packet)
{
	// WIP
}



//void Laser::start()
//{
//	gameObject->networkInterpolationEnabled = false;
//
//	App->modSound->playAudioClip(App->modResources->audioClipLaser);
//}
//
//void Laser::update()
//{
//	secondsSinceCreation += Time.deltaTime;
//
//	const float pixelsPerSecond = 1000.0f;
//	gameObject->position += vec2FromDegrees(gameObject->angle) * pixelsPerSecond * Time.deltaTime;
//
//	if (isServer)
//	{
//		const float neutralTimeSeconds = 0.1f;
//		if (secondsSinceCreation > neutralTimeSeconds && gameObject->collider == nullptr) {
//			gameObject->collider = App->modCollision->addCollider(ColliderType::Laser, gameObject);
//		}
//
//		const float lifetimeSeconds = 2.0f;
//		if (secondsSinceCreation >= lifetimeSeconds) {
//			NetworkDestroy(gameObject);
//		}
//	}
//}





void PlayerCrosshair::start()
{
	gameObject->tag = (uint32)(Random.next() * UINT_MAX);

	/*lifebar = Instantiate();
	lifebar->sprite = App->modRender->addSprite(lifebar);
	lifebar->sprite->pivot = vec2{ 0.0f, 0.5f };
	lifebar->sprite->order = 5;*/
}

void PlayerCrosshair::onMouse(const MouseController& mouse)
{
	gameObject->position = { (float)mouse.x, (float)mouse.y};
	
	if (mouse.buttons[0] == ButtonState::Press)
	{
		//LOG("MOUSE LEFT CLICK: %i,%i", mouse.x, mouse.y);

		if (isServer)
		{
			GameObject* particleShot = NetworkInstantiate();
			particleShot->netType = NetEntityType::Shoot;
			particleShot->position = gameObject->position;
			//particleShot->angle = gameObject->angle;
			particleShot->size = { 50, 50 };

			particleShot->sprite = App->modRender->addSprite(particleShot);
			particleShot->sprite->texture = App->modResources->explosion1;
			particleShot->sprite->order = 100;

			particleShot->animation = App->modRender->addAnimation(particleShot);
			particleShot->animation->clip = App->modResources->explosionClip;

			NetworkDestroy(particleShot, 2.0f);

			// TODO: PLAY SOUND ON ALL CLIENTS
		}

		if (isServer) // check collision from this player
		{
			// TODO: wip
			int winIdx = -1;
			CowboyWindowManager* winManager = dynamic_cast<CowboyWindowManager*>(App->modScreen->screenGame->windowManager->behaviour);
			if (winManager->CheckMouseClickCollision({ (float)mouse.x, (float)mouse.y }, winIdx))
			{
				if (winManager->windows[winIdx].state == WindowState::open)
				{
					//LOG("Window %i, OPEN");
					dynamic_cast<PlayerCrosshair*>(gameObject->behaviour)->score += winManager->enemyScores[(int)winManager->windows[winIdx].currentEnemyType];

					winManager->windows[winIdx].hitByNetworkId = gameObject->networkId;
					winManager->windows[winIdx].Close();
					//NetWorkUpdateTarget(winManager->windows[winIdx].window);

					//GameObject* blood_splash = NetworkInstantiate();
					//blood_splash->netType = NetEntityType::Blood;
					//blood_splash->position = gameObject->position;
					////particleShot->angle = gameObject->angle;
					//blood_splash->size = { 125, 125 };

					//blood_splash->sprite = App->modRender->addSprite(blood_splash);
					//blood_splash->sprite->texture = App->modResources->blood;
					//blood_splash->sprite->order = 150;

					//blood_splash->animation = App->modRender->addAnimation(blood_splash);
					//blood_splash->animation->clip = App->modResources->bloodSplash;
					//
					//NetworkDestroy(blood_splash, 5.0f);
				}
				else
				{
					// LOG("Window %i, CLOSED");
					// DO nothing
				}
			}
		}
	}

	if (isServer)
	{
		NetworkUpdate(gameObject, false);
	}
	
}

void PlayerCrosshair::onInput(const InputController &input)
{
	/*if (input.horizontalAxis != 0.0f)
	{
		const float rotateSpeed = 180.0f;
		gameObject->angle += input.horizontalAxis * rotateSpeed * Time.deltaTime;

		if (isServer)
		{
			NetworkUpdate(gameObject);
		}
	}*/

	/*if (input.actionDown == ButtonState::Pressed)
	{
		const float advanceSpeed = 200.0f;
		gameObject->position += vec2FromDegrees(gameObject->angle) * advanceSpeed * Time.deltaTime;

		if (isServer)
		{
			NetworkUpdate(gameObject);
		}
	}*/


	/*if (input.actionLeft == ButtonState::Press)
	{
		if (isServer)
		{
			GameObject *laser = NetworkInstantiate();

			laser->netType = NetEntityType::Laser;
			laser->position = gameObject->position;
			laser->angle = gameObject->angle;
			laser->size = { 20, 60 };

			laser->sprite = App->modRender->addSprite(laser);
			laser->sprite->order = 3;
			laser->sprite->texture = App->modResources->laser;

			Laser *laserBehaviour = App->modBehaviour->addLaser(laser);
			laserBehaviour->isServer = isServer;

			laser->tag = gameObject->tag;
		}
	}*/
}

void PlayerCrosshair::update()
{
	/*static const vec4 colorAlive = vec4{ 0.2f, 1.0f, 0.1f, 0.5f };
	static const vec4 colorDead = vec4{ 1.0f, 0.2f, 0.1f, 0.5f };
	const float lifeRatio = max(0.01f, (float)(hitPoints) / (MAX_HIT_POINTS));
	lifebar->position = gameObject->position + vec2{ -50.0f, -50.0f };
	lifebar->size = vec2{ lifeRatio * 80.0f, 5.0f };
	lifebar->sprite->color = lerp(colorDead, colorAlive, lifeRatio);*/

	if (isServer)
		return;

	uint32 id = App->modNetClient->GetNetworkID();

	if (gameObject->networkId == id) {
		vec2 vp = App->modRender->GetViewportSize();
		gameObject->position = { (float)Mouse.x, (float)Mouse.y};

		// NOTE: first try to instantiate all clients from server to check how it feels if not:
		// TODO: instantiate particle for this client on local

	}
}

void PlayerCrosshair::destroy()
{
	//Destroy(lifebar);
}

void PlayerCrosshair::onCollisionTriggered(Collider &c1, Collider &c2)
{
	if (c2.type == ColliderType::Laser && c2.gameObject->tag != gameObject->tag)
	{
		if (isServer)
		{
			NetworkDestroy(c2.gameObject); // Destroy the laser
		
			/*if (hitPoints > 0)
			{
				hitPoints--;
				NetworkUpdate(gameObject);
			}*/

			float size = 30 + 50.0f * Random.next();
			vec2 position = gameObject->position + 50.0f * vec2{Random.next() - 0.5f, Random.next() - 0.5f};

			//if (hitPoints <= 0)
			//{
			//	// Centered big explosion
			//	size = 250.0f + 100.0f * Random.next();
			//	position = gameObject->position;

			//	NetworkDestroy(gameObject);
			//}

			/*GameObject *explosion = NetworkInstantiate();
			explosion->netType = NetEntityType::Explosion;
			explosion->position = position;
			explosion->size = vec2{ size, size };
			explosion->angle = 365.0f * Random.next();

			explosion->sprite = App->modRender->addSprite(explosion);
			explosion->sprite->texture = App->modResources->explosion1;
			explosion->sprite->order = 100;

			explosion->animation = App->modRender->addAnimation(explosion);
			explosion->animation->clip = App->modResources->explosionClip;

			NetworkDestroy(explosion, 2.0f);*/

			// NOTE(jesus): Only played in the server right now...
			// You need to somehow make this happen in clients
			App->modSound->playAudioClip(App->modResources->audioClipExplosion);
		}
	}
}

void PlayerCrosshair::write(OutputMemoryStream & packet)
{
	//packet << hitPoints;
}

void PlayerCrosshair::read(const InputMemoryStream & packet)
{
	//packet >> hitPoints;
}

// --------------------- WINDOW itself logic ------------------------------------

void CowboyWindow::Update()
{
	if (Time.time > spawned_at + lifetime)
		Close();
}

void CowboyWindow::Open(EnemyType type)
{
	hitByNetworkId = 0;
	state = WindowState::open;
	window->sprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };

	spawned_at = Time.time;
	float min_lifetime = 0.6f; // sec
	lifetime = max_lifetime * Random.next();

	if (lifetime < min_lifetime)
		lifetime = min_lifetime;

	// get one random enemy

	if (target == nullptr)	 // if target is still not created
	{
		target = Instantiate();
		vec2 windowPos = window->position;
		vec2 windowSize = window->size;

		target->position = { windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f };
		// TODO: get the offset from some place based on enemy type
		vec2 offset = { 13, -9 }; // valid for all bads, not the hostages
		target->position.x += offset.x;
		target->position.y += offset.y;
	}

	if (target->sprite == nullptr)
	{
		target->sprite = App->modRender->addSprite(target);
		target->sprite->texture = App->modResources->tex_cowboy_window;
		target->sprite->order = window->sprite->order + 1;
	}

	currentEnemyType = EnemyType::none;
	target->sprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	if (type != EnemyType::none) { // client calls with specific type
		target->sprite->clipRect = winMan->targetsRects[(int)type].spawnRect;
	}
	else
		target->sprite->clipRect = GetRandomEnemy(currentEnemyType); // server creates random ones

	target->size = { target->sprite->clipRect.z, target->sprite->clipRect.w };
	//target->sprite->pivot = { 0,0 };

	winMan->current_opened_windows++;
	winMan->last_window_opened_time = Time.time;

	App->modSound->playAudioClip(App->modResources->audioClipDoorOpen);

	if (winMan->isServer)
		NetWorkUpdateTarget(window);
}

void CowboyWindow::Close()
{
	window->sprite->color = { 1.0f, 1.0f, 1.0f, 0.0f };

	if (target != nullptr)
	{
		target->sprite->color = { 1.0f, 1.0f, 1.0f, 0.0f };
	}

	if(state == WindowState::open)
		winMan->current_opened_windows--;

	state = WindowState::closed;

	winMan->last_window_closed_id = window_id;
	winMan->last_window_closed_time = Time.time;

	App->modSound->playAudioClip(App->modResources->audioClipDoorClose);

	if(winMan->isServer)
		NetWorkUpdateTarget(window);
}

vec4 CowboyWindow::GetRandomEnemy(EnemyType& type)
{
	type = (EnemyType)(int)(((int)EnemyType::max) * Random.next());

	if (type == EnemyType::none) // Case random 0. - 0.9
		type = EnemyType::bad1;

	int index = (int)type;
	if (index < 0 || index >= (int)EnemyType::max) {
		ELOG("Random returned a non existant enemy");
		return vec4{ 0,0,0,0 };
	}

	vec4 ret = winMan->targetsRects[index].spawnRect;
	return ret;
}

