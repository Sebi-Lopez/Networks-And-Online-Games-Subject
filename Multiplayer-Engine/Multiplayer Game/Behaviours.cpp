#include "Networks.h"
#include "Behaviours.h"



void CowboyWindowManager::start()
{
	// create necessary rects for window and different enemies/hostages
	vec4 windowsPositions[MAX_SPAWN_WINDOWS];
	windowsPositions[0] = { 354, 0 };
	windowsPositions[1] = { 662, 0 };
	windowsPositions[2] = { 946, 0 };
	windowsPositions[3] = { 380, 308 };
	windowsPositions[4] = { 946, 308 };

	for (int i = 0; i < MAX_SPAWN_WINDOWS; ++i)
	{
		windows[i].window = Instantiate();
		windows[i].window->position = { windowsPositions[i].x - 1280*0.5f, windowsPositions[i].y - 720*0.5f };

		windows[i].window->sprite = App->modRender->addSprite(windows[i].window);
		windows[i].window->sprite->texture = App->modResources->tex_cowboy_window;
		windows[i].window->sprite->clipRect = { 358, 358, 154, 154};
		windows[i].window->sprite->pivot = { 0,0 };
		windows[i].window->size = { 154,154 };
	}

	CloseAllWindows();

	/*OpenWindow(3);
	OpenWindow(2);*/

	// targets
	targetsRects[0].spawnRect = { 0,0, 113, 129};
}

void CowboyWindowManager::CloseAllWindows()
{
	for (int i = 0; i < MAX_SPAWN_WINDOWS; ++i)
	{
		windows[i].state = WindowState::closed;
		windows[i].window->sprite->color = { 1.0f, 1.0f, 1.0f, 0.0f };
	}
}

void CowboyWindowManager::OpenWindow(uint8 n)
{
	windows[n].state = WindowState::open;
	windows[n].window->sprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void CowboyWindowManager::onMouse(const MouseController& mouse)
{

}

void CowboyWindowManager::update()
{


	if (!isServer)
		return;



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
	vec2 vp = App->modRender->GetViewportSize();
	gameObject->position = { (float)mouse.x - vp.x * 0.5f, (float)mouse.y - vp.y * 0.5f };
	//WLOG("%i", mouse.x);
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
		gameObject->position = { (float)Mouse.x - vp.x * 0.5f, (float)Mouse.y - vp.y * 0.5f };
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