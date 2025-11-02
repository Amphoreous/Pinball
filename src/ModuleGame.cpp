#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"
#include "PhysBody.h"

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	
}

ModuleGame::~ModuleGame()
{}

// Load assets
bool ModuleGame::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;

	// Load default font
	font = GetFontDefault();
	
	// Create pinball table boundaries
	// Left wall
	PhysBody* leftWall = App->physics->CreateRectangle(10, SCREEN_HEIGHT / 2, 20, SCREEN_HEIGHT, b2_staticBody);
	
	// Right wall
	PhysBody* rightWall = App->physics->CreateRectangle(SCREEN_WIDTH - 10, SCREEN_HEIGHT / 2, 20, SCREEN_HEIGHT, b2_staticBody);
	
	// Bottom wall
	PhysBody* bottomWall = App->physics->CreateRectangle(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 20, b2_staticBody);
	
	// Top wall
	PhysBody* topWall = App->physics->CreateRectangle(SCREEN_WIDTH / 2, 10, SCREEN_WIDTH, 20, b2_staticBody);
	
	// Create ball
	ball = App->physics->CreateCircle(SCREEN_WIDTH / 2, 100, 15, b2_dynamicBody);
	ball->listener = this;
	
	// TODO: Load ball texture from assets/balls folder
	// ballTexture = LoadTexture("assets/balls/ball1.png");
	
	return ret;
}

// Load assets
bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");
	
	// TODO: Unload textures
	// UnloadTexture(ballTexture);

	return true;
}

// Update: draw background
update_status ModuleGame::Update()
{
	// Handle input
	if(IsKeyPressed(KEY_SPACE))
	{
		// Launch ball or start game
		if(!roundActive)
		{
			roundActive = true;
			gameStarted = true;
		}
	}
	
	// Draw score
	DrawText(TextFormat("Score: %d", currentScore), 20, 20, 20, BLACK);
	DrawText(TextFormat("Previous: %d", previousScore), 20, 50, 20, DARKGRAY);
	DrawText(TextFormat("High Score: %d", highestScore), 20, 80, 20, DARKGREEN);
	DrawText(TextFormat("Balls: %d", ballsRemaining), 20, 110, 20, RED);
	
	// Draw ball
	if(ball)
	{
		int x, y;
		ball->GetPosition(x, y);
		DrawCircle(x, y, 15, BLUE);
	}
	
	return UPDATE_CONTINUE;
}

float ModuleGame::CalculateImpactForce(PhysBody* body)
{
	b2Vec2 vel = body->body->GetLinearVelocity();
	float speed = vel.Length();

	float maxSpeed = 20.0f;
	float force = speed / maxSpeed;

	if (force > 1.0f) force = 1.0f;
	if (force < 0.0f) force = 0.0f;

	return force;
}

CollisionType ModuleGame::IdentifyCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// Check if hit a flipper
	if (bodyB == leftFlipper || bodyB == rightFlipper)
		return COLLISION_FLIPPER;

	// Default: treat everything else as walls for now
	return COLLISION_WALL;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// Handle collisions
	LOG("Collision detected!");
	
	float impactForce = CalculateImpactForce(bodyA);

	CollisionType type = IdentifyCollision(bodyA, bodyB);

	switch (type)
	{
	case COLLISION_FLIPPER:
		App->audio->PlayFlipperHit(impactForce);
		currentScore += 10;
		break;

	case COLLISION_BUMPER:
		App->audio->PlayBumperHit(impactForce);
		currentScore += 50;
		break;
	case COLLISION_WALL:
	default:
		// Wall collision - softer sound
		App->audio->PlayBumperHit(impactForce * 0.5f);
		currentScore += 10;
		break;
	}

	// TODO: Play sound effect
	// App->audio->PlayFx(collisionFx);
}
