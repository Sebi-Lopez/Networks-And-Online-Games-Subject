#pragma once

#define MAX_SPAWN_WINDOWS 5
#define MAX_GAME_STATES 10
#define SECONDS_TO_RECORD_STATE 0.2f

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
	virtual void onMouse(const MouseController &mouse, const double& time_stamp = 0.0) { } // xd

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
	
	CrosshairRects reticle;

	BehaviourType type() const override { return BehaviourType::crosshair; }

	void start() override;

	void onInput(const InputController &input) override;
	void onMouse(const MouseController& mouse, const double& time_stamp) override;

	void update() override;

	void destroy() override;

	void onCollisionTriggered(Collider &c1, Collider &c2) override;

	void write(OutputMemoryStream &packet) override;

	void read(const InputMemoryStream &packet) override;
};

enum class GameState
{
	none, waiting, countdown, started, finished, max
};

enum class WindowState
{
	none, closed, open, max
};

enum class EnemyType : uint8
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

	float max_lifetime = 2.0f; // max lifetime to despawn this target
	float spawned_at = 0.0f; // store spawn time
	float lifetime = 0.0f;

	void Open();
	void Close();
	void Update();

	vec4 GetRandomEnemy(EnemyType& type);
	
};

struct Targets
{
	vec4 spawnRect = {};
	vec4 deathRect = {};
};


struct WindowTimedState
{
	CowboyWindow windows[MAX_SPAWN_WINDOWS] = {};
	float time = 0.0f;
};

struct CowboyWindowManager : public Behaviour
{

	GameState gameLoopState = GameState::none;

	uint8 max_opened_windows = 3;
	uint8 current_opened_windows = 0;
	uint8 last_window_closed_id = 0; // stores id from windows arrays to not repeat the last closed window
	float last_window_closed_time = 0.0f;
	float last_window_opened_time = 0.0f;
	float min_interval_between_spawns_close = 0.5f;
	float min_interval_between_spawns_open = 0.7f;

	uint32 average_time_between_spawns = 1000;

	uint32 countdown_duration = 5000;
	uint32 game_duration = 10000;


	CowboyWindow windows[MAX_SPAWN_WINDOWS]; // store windows gameobjects
	
	
	WindowTimedState window_timed_states[MAX_GAME_STATES] = {};
	float record_time = 0.0f;
	int next_index = 0;

	//

	Targets targetsRects[1]; // store enemie/hostage rects

	//

	BehaviourType type() const override { return BehaviourType::window_manager; }

	void start() override;


	//void onInput(const InputController& input) override;
	void onMouse(const MouseController& mouse, const double& time_stamp) override;

	void update() override;

	void destroy() override;

	//void onCollisionTriggered(Collider& c1, Collider& c2) override;

	void write(OutputMemoryStream& packet) override;

	void read(const InputMemoryStream& packet) override;

	// ---------------------- 

	void GameLoopUpdate();
	void SpawnLogic();
	void UpdateActiveWindows();

	void StoreWindowsInformation(); 

	void CloseAllWindows();
	void OpenWindow(uint8 n);

};


//struct Laser : public Behaviour
//{
//	float secondsSinceCreation = 0.0f;
//
//	BehaviourType type() const override { return BehaviourType::Laser; }
//
//	void start() override;
//
//	void update() override;
//};


//struct Spaceship : public Behaviour
//{
//	static const uint8 MAX_HIT_POINTS = 5;
//	uint8 hitPoints = MAX_HIT_POINTS;
//	uint8 spaceShipType = 0;
//
//	GameObject *lifebar = nullptr;
//
//	BehaviourType type() const override { return BehaviourType::Spaceship; }
//
//	void start() override;
//
//	void onInput(const InputController &input) override;
//
//	void update() override;
//
//	void destroy() override;
//
//	void onCollisionTriggered(Collider &c1, Collider &c2) override;
//
//	void write(OutputMemoryStream &packet) override;
//
//	void read(const InputMemoryStream &packet) override;
//};
