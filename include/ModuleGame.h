#pragma once

#include "Globals.h"
#include "Module.h"
#include "GameState.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>

class PhysBody;
class PhysicEntity;
class b2RevoluteJoint;

enum CollisionType
{
	COLLISION_WALL,
	COLLISION_FLIPPER,
	COLLISION_BUMPER,
	COLLISION_TARGET,
	COLLISION_COMBO_LETTER
};

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	// Core module functions
	bool Start();
	update_status Update();
	bool CleanUp();
	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

	// Collision detection helpers
	float CalculateImpactForce(PhysBody* body);
	CollisionType IdentifyCollision(PhysBody* bodyA, PhysBody* bodyB);

	// Audio settings management
	void DrawAudioSettings();
	void UpdateAudioSettings();
	void SaveAudioSettings();
	void LoadAudioSettings();

	// State-specific rendering functions
	void RenderMenuState();
	void RenderPlayingState();
	void RenderPausedState();
	void RenderGameOverState();

	// State-specific update functions
	void UpdateMenuState();
	void UpdatePlayingState();
	void UpdatePausedState();
	void UpdateGameOverState();

	// Game logic functions
	void LaunchBall();
	void LoseBall();
	void AddComboLetter(char letter);
	void SaveHighScore();
	void LoadHighScore();

	// TMX Map loading
	bool LoadTMXMap(const char* filepath);
	void CreateMapCollision();

public:
	// Game state system
	GameData gameData;

	// Physics bodies
	PhysBody* ball = nullptr;
	PhysBody* leftFlipper = nullptr;
	PhysBody* rightFlipper = nullptr;
	PhysBody* kicker = nullptr;
	
	// Flipper joints
	b2RevoluteJoint* leftFlipperJoint = nullptr;
	b2RevoluteJoint* rightFlipperJoint = nullptr;
	
	// Bumpers
	std::vector<PhysBody*> bumpers;

	// Textures
	Texture2D ballTexture;
	Texture2D backgroundTexture;
	Texture2D flipperTexture;
	Texture2D flipperBaseTexture;
	Texture2D bumper1Texture;
	Texture2D bumper2Texture;
	Texture2D bumper3Texture;
	Texture2D piece1Texture;
	Texture2D piece2Texture;

	// UI elements
	Font font;
	Font titleFont;

	// Audio settings UI
	bool showAudioSettings = false;
	bool settingsSavedMessage = false;
	float settingsSavedTimer = 0.0f;

	// Ball launch control
	bool ballLaunched = false;
	float kickerForce = 0.0f;
	float kickerChargeTime = 0.0f;
	const float MAX_KICKER_FORCE = 20.0f;
	const float KICKER_CHARGE_SPEED = 10.0f;

	// TMX Map data
	std::vector<int> mapCollisionPoints;
	PhysBody* mapBoundary = nullptr;
};