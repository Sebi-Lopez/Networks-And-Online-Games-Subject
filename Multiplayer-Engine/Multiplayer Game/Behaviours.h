#pragma once

#define MAX_SPAWN_WINDOWS 5

enum class BehaviourType : uint8;
struct CowboyWindowManager;

struct Behaviour
{
	GameObject *gameObject = nullptr;
	bool isServer = false;
	bool isLocalPlayer = false;

	virtual BehaviourType type() const = 0;

	virtual void start() { }

	virtual void onInput(const InputController &input) { }
	virtual void onMouse(const MouseController &mouse) { } // xd

	virtual void update() { }

	virtual void destroy() { }

	virtual void onCollisionTriggered(Collider &c1, Collider &c2) { }

	virtual void write(OutputMemoryStream &packet) { }

	virtual void read(const InputMemoryStream &packet) { }
};


enum class BehaviourType : uint8
{
	None,
	crosshair,
	window_manager,
	cowboy_window,
	max
};

struct CrosshairRects
{
	uint8 crosshairType = 0;
	vec4 reticle_outside = {};
	vec4 reticle_hit = {};
};


struct PlayerCrosshair : public Behaviour
{
	std::string playerName;
	CrosshairRects reticle;
	int score = 0;
	bool ready = false; // determine if the player is ready to countdown

	BehaviourType type() const override { return BehaviourType::crosshair; }

	void start() override;

	void onInput(const InputController &input) override;
	void onMouse(const MouseController& mouse) override;

	void update() override;

	void destroy() override;

	/*void onCollisionTriggered(Collider &c1, Collider &c2) override;

	void write(OutputMemoryStream &packet) override;

	void read(const InputMemoryStream &packet) override;*/
};

enum class GameState
{
	none, waiting, countdown, started, finished, max
};

enum class WindowState
{
	none, closed, open, max
};

enum class EnemyType
{
	none, bad1, bad2, bad3, hostage1, hostage2, max
};

struct CowboyWindow
{
	uint8 window_id = 0;
	CowboyWindowManager* winMan = nullptr; 
	GameObject* window = nullptr; // window opened
	GameObject* target = nullptr; // enemy/hostage
	WindowState state;
	EnemyType currentEnemyType = EnemyType::none;
	uint32 hitByNetworkId = 0; // store if this comes from a succesfully shot or by time despawn

	float max_lifetime = 2.0f; // max lifetime to despawn this target
	float spawned_at = 0.0f; // store spawn time
	float lifetime = 0.0f;

	void Open(EnemyType type = EnemyType::none);
	void Close();
	void Update();

	vec4 GetRandomEnemy(EnemyType& type);
	
};

struct Targets
{
	vec4 spawnRect = {};
	vec4 deathRect = {};
};

struct CowboyWindowManager : public Behaviour
{

	GameState gameLoopState = GameState::none;
	int enemyScores[(int)EnemyType::max];

	uint8 max_opened_windows = 3;
	uint8 current_opened_windows = 0;
	uint8 last_window_closed_id = 0; // stores id from windows arrays to not repeat the last closed window
	float last_window_closed_time = 0.0f;
	float last_window_opened_time = 0.0f;
	float min_interval_between_spawns_close = 0.5f;
	float min_interval_between_spawns_open = 0.7f;

	//uint32 average_time_between_spawns = 1000;

	float countdown_duration = 5.0f;
	float game_duration = 60.f;
	float game_started_at = 0.0f;

	CowboyWindow windows[MAX_SPAWN_WINDOWS]; // store windows gameobjects
	uint8 wIdx = 0;

	//

	Targets targetsRects[(int)EnemyType::max]; // store enemie/hostage rects
	vec4 collisionRect = {}; // store the area to perform coords collision check

	//

	BehaviourType type() const override { return BehaviourType::window_manager; }

	void start() override;


	//void onInput(const InputController& input) override;
	void onMouse(const MouseController& mouse) override;

	void update() override;

	void destroy() override;

	//void onCollisionTriggered(Collider& c1, Collider& c2) override;

	void write(OutputMemoryStream& packet) override;

	void read(const InputMemoryStream& packet) override;

	// ---------------------- 
	bool CheckMouseClickCollision(vec2 clickPos, int& winIdx) const;

	void GameLoopUpdate();
	void SpawnLogic();
	void UpdateActiveWindows();

	void CloseAllWindows();
	void OpenWindow(uint8 n);

	// client side
	GameObject* GetNextCowWindow();

	CowboyWindow* GetCowboyWindowWithNetworkId(uint32 networkId);

};