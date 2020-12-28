#pragma once


enum class BehaviourType : uint8;

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
	void onMouse(const MouseController& mouse) override;

	void update() override;

	void destroy() override;

	void onCollisionTriggered(Collider &c1, Collider &c2) override;

	void write(OutputMemoryStream &packet) override;

	void read(const InputMemoryStream &packet) override;
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
