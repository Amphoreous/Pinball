#pragma once

#include "Globals.h"
#include "Module.h"

#include "p2Point.h"

#include "raylib.h"
#include <vector>

class PhysBody;
class PhysicEntity;

enum CollisionType
{
	COLLISION_WALL,
	COLLISION_FLIPPER,
	COLLISION_BUMPER,

};

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	float CalculateImpactForce(PhysBody* body);
	CollisionType IdentifyCollision(PhysBody* bodyA, PhysBody* bodyB);

	bool Start();
	update_status Update();
	bool CleanUp();
	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

	void DrawAudioSettings();
	void UpdateAudioSettings();
	void SaveAudioSettings();
	void LoadAudioSettings();

public:
	// Ball
	PhysBody* ball = nullptr;
	Texture2D ballTexture;
	
	// Flippers
	PhysBody* leftFlipper = nullptr;
	PhysBody* rightFlipper = nullptr;
	
	// Score tracking
	int currentScore = 0;
	int previousScore = 0;
	int highestScore = 0;
	int ballsRemaining = 3;
	
	// Game state
	bool gameStarted = false;
	bool roundActive = false;

	bool showAudioSettings = false;
	bool settingsSavedMessage = false;
	float settingsSavedTimer = 0.0f;
	
	// Fonts for UI
	Font font;
};
