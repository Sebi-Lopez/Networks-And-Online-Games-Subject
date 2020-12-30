
#include "Networks.h"

GameObject* background01 = nullptr;


void ScreenGame::enable()
{
	if (isServer)
	{
		App->modNetServer->setListenPort(serverPort);
		App->modNetServer->setEnabled(true);
	}
	else
	{
		App->modNetClient->setServerAddress(serverAddress, serverPort);
		App->modNetClient->setPlayerInfo(playerName, spaceshipType);
		App->modNetClient->setEnabled(true);
	}

	background01 = Instantiate();
	background01->sprite = App->modRender->addSprite(background01);
	background01->sprite->texture = App->modResources->tex_sunset_background;
	background01->sprite->order = -1;
	background01->sprite->pivot = { 0,0 };
	//background01->sprite->clipRect = { 0,0, 256, 256 };
	//vec2 vp = App->modRender->GetViewportSize();
	//background01->size = { vp.x, vp.y };

	// set camera position to match world coords 0,0 on top left of the screen
	// based on viewportsize
	App->modRender->cameraPosition = App->modRender->GetViewportSize() * 0.5f;

	windowManager = Instantiate();
	App->modBehaviour->addBehaviour(BehaviourType::window_manager, windowManager);
	if (isServer)
		windowManager->behaviour->isServer = true;




	
	/*spaceTopRight = Instantiate();
	spaceTopRight->sprite = App->modRender->addSprite(spaceTopRight);
	spaceTopRight->sprite->texture = App->modResources->space;
	spaceTopRight->sprite->order = -1;
	spaceBottomLeft = Instantiate();
	spaceBottomLeft->sprite = App->modRender->addSprite(spaceBottomLeft);
	spaceBottomLeft->sprite->texture = App->modResources->space;
	spaceBottomLeft->sprite->order = -1;
	spaceBottomRight = Instantiate();
	spaceBottomRight->sprite = App->modRender->addSprite(spaceBottomRight);
	spaceBottomRight->sprite->texture = App->modResources->space;
	spaceBottomRight->sprite->order = -1;*/
}

void ScreenGame::update()
{
	if (!(App->modNetServer->isConnected() || App->modNetClient->isConnected()))
	{
		App->modScreen->swapScreensWithTransition(this, App->modScreen->screenMainMenu);
	}
	else
	{
		if (!isServer)
		{
			/*vec2 camPos = App->modRender->cameraPosition;
			vec2 bgSize = spaceTopLeft->sprite->texture->size;
			spaceTopLeft->position = bgSize * floor(camPos / bgSize);
			spaceTopRight->position = bgSize * (floor(camPos / bgSize) + vec2{ 1.0f, 0.0f });
			spaceBottomLeft->position = bgSize * (floor(camPos / bgSize) + vec2{ 0.0f, 1.0f });
			spaceBottomRight->position = bgSize * (floor(camPos / bgSize) + vec2{ 1.0f, 1.0f });;*/
		}
	}
}

void ScreenGame::gui()
{

	std::vector<PlayerCrosshair*> players = App->modBehaviour->GetPlayersCrosshairs();
	ImGui::Begin("Scoreboard window");
	
	for (int i = 0; i < players.size(); ++i)
	{
		ImGui::Text("%i", (PlayerCrosshair*)players[i]->score);
	}

	ImGui::End();
	
}

void ScreenGame::disable()
{
	Destroy(background01);
	Destroy(windowManager);
	/*Destroy(spaceTopLeft);
	Destroy(spaceTopRight);
	Destroy(spaceBottomLeft);
	Destroy(spaceBottomRight);*/
}
