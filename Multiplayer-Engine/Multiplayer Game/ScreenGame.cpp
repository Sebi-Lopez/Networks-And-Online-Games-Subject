
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

	// set camera position to match world coords 0,0 on top left of the screen
	// based on viewportsize
	App->modRender->cameraPosition = App->modRender->GetViewportSize() * 0.5f;

	windowManager = Instantiate();
	App->modBehaviour->addBehaviour(BehaviourType::window_manager, windowManager);
	if (isServer)
		windowManager->behaviour->isServer = true;

}

void ScreenGame::update()
{
	if (!(App->modNetServer->isConnected() || App->modNetClient->isConnected()))
	{
		App->modScreen->swapScreensWithTransition(this, App->modScreen->screenMainMenu);
	}
}

bool comparePlayerCrosshairScore(PlayerCrosshair* a, PlayerCrosshair* b) { return (a->score > b->score); }

void ScreenGame::gui()
{

	std::vector<PlayerCrosshair*> players = App->modBehaviour->GetPlayersCrosshairs();
	ImGui::Begin("Players info");
	ImGui::Spacing();

	// search myself on the behaviours
	PlayerCrosshair* myself = nullptr;
	for (int i = 0; i < players.size(); ++i)
	{
		if (players[i]->gameObject->networkId == App->modNetClient->GetNetworkID())
			myself = players[i];
	}

	if(myself != nullptr)
		ImGui::Checkbox("im ready!", &myself->ready);
	
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Text("Score Board:");
	ImGui::Separator();
	ImGui::Spacing();

	std::sort(players.begin(), players.end(), comparePlayerCrosshairScore);

	for (int i = 0; i < players.size(); ++i)
	{
		std::string readyStr = "- ready";
		ImGui::Text("%s : %i %s", (PlayerCrosshair*)players[i]->playerName.c_str(), (PlayerCrosshair*)players[i]->score, 
			(PlayerCrosshair*)players[i]->ready ? readyStr.c_str() : "");
		ImGui::Separator();
		ImGui::Spacing();
	}

	ImGui::End();

	// TIME COUNTER WINDOW
	float current_game_time = 0.0f;
	float duration = 999.0f;

	CowboyWindowManager* winMan = nullptr;

	if(windowManager != nullptr)
		winMan = dynamic_cast<CowboyWindowManager*>(windowManager->behaviour);
	if (winMan != nullptr)
	{
		if (winMan->gameLoopState == GameState::started)
		{
			current_game_time = Time.time - winMan->game_started_at;
			if(myself)
				myself->ready = false;
		}

		duration = winMan->game_duration;

		// TODO: find a better way
		if (!isServer)
		{
			if (current_game_time > duration)
			{
				winMan->gameLoopState = GameState::none;
			}
		}
	}

	ImGui::Begin("ROUND TIME");
	ImGui::Text(" %.2f - of %.2f sec", current_game_time, duration);

	ImGui::End();
}

void ScreenGame::disable()
{
	Destroy(background01);
	Destroy(windowManager);
}
