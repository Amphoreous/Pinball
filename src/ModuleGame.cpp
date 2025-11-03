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

	// AUDIO SETTINGS: Load saved settings
	LoadAudioSettings();
	
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

	// Toggle settings menu with F2
	if (IsKeyPressed(KEY_F2))
	{
		showAudioSettings = !showAudioSettings;
	}

	if (settingsSavedMessage)
	{
		settingsSavedTimer += GetFrameTime();
		if (settingsSavedTimer >= 2.0f) // Show for 2 seconds
		{
			settingsSavedMessage = false;
			settingsSavedTimer = 0.0f;
		}
	}

	if (showAudioSettings)
	{
		UpdateAudioSettings();
		DrawAudioSettings();
		return UPDATE_CONTINUE; // Don't update game while in menu
	}

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
	
	// AUDIO SETTINGS: Show hint
	DrawText("Press F2 for Audio Settings", 20, SCREEN_HEIGHT - 50, 16, DARKGRAY);

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

void ModuleGame::DrawAudioSettings()
{
	// Semi-transparent background
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 200 });

	// Title
	DrawText("AUDIO SETTINGS", SCREEN_WIDTH / 2 - 150, 50, 30, WHITE);
	DrawText("Press F2 to close", SCREEN_WIDTH / 2 - 100, 90, 16, GRAY);

	int startY = 150;
	int spacing = 80;

	// Master Volume
	DrawText("Master Volume:", 100, startY, 20, WHITE);
	DrawText(TextFormat("%.0f%%", App->audio->GetMasterVolume() * 100), 400, startY, 20, YELLOW);
	DrawRectangle(100, startY + 30, 400, 20, DARKGRAY);
	DrawRectangle(100, startY + 30, (int)(400 * App->audio->GetMasterVolume()), 20, GREEN);
	DrawText("[1/2] Decrease/Increase", 520, startY + 5, 16, LIGHTGRAY);

	// Music Volume
	DrawText("Music Volume:", 100, startY + spacing, 20, WHITE);
	DrawText(TextFormat("%.0f%%", App->audio->GetMusicVolume() * 100), 400, startY + spacing, 20, YELLOW);
	DrawRectangle(100, startY + spacing + 30, 400, 20, DARKGRAY);
	DrawRectangle(100, startY + spacing + 30, (int)(400 * App->audio->GetMusicVolume()), 20, BLUE);
	DrawText("[3/4] Decrease/Increase", 520, startY + spacing + 5, 16, LIGHTGRAY);

	// SFX Volume
	DrawText("SFX Volume:", 100, startY + spacing * 2, 20, WHITE);
	DrawText(TextFormat("%.0f%%", App->audio->GetSFXVolume() * 100), 400, startY + spacing * 2, 20, YELLOW);
	DrawRectangle(100, startY + spacing * 2 + 30, 400, 20, DARKGRAY);
	DrawRectangle(100, startY + spacing * 2 + 30, (int)(400 * App->audio->GetSFXVolume()), 20, RED);
	DrawText("[5/6] Decrease/Increase", 520, startY + spacing * 2 + 5, 16, LIGHTGRAY);

	// Mute toggles
	DrawText("Mute All: [M]", 100, startY + spacing * 3, 20, WHITE);
	if (App->audio->GetMasterVolume() == 0.0f)
		DrawText("MUTED", 300, startY + spacing * 3, 20, RED);
	else
		DrawText("ACTIVE", 300, startY + spacing * 3, 20, GREEN);

	// Instructions
	DrawText("Press [S] to save settings", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT - 80, 18, GREEN);

	// Show saved message
	if (settingsSavedMessage)
	{
		DrawText("Settings Saved!", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT - 50, 20, LIME);
	}
}

// AUDIO SETTINGS: Handle input for settings menu
void ModuleGame::UpdateAudioSettings()
{
	float step = 0.05f; // 5% per press

	// Master Volume controls
	if (IsKeyPressed(KEY_ONE))
	{
		App->audio->SetMasterVolume(App->audio->GetMasterVolume() - step);
	}
	if (IsKeyPressed(KEY_TWO))
	{
		App->audio->SetMasterVolume(App->audio->GetMasterVolume() + step);
	}

	// Music Volume controls
	if (IsKeyPressed(KEY_THREE))
	{
		App->audio->SetMusicVolume(App->audio->GetMusicVolume() - step);
	}
	if (IsKeyPressed(KEY_FOUR))
	{
		App->audio->SetMusicVolume(App->audio->GetMusicVolume() + step);
	}

	// SFX Volume controls
	if (IsKeyPressed(KEY_FIVE))
	{
		App->audio->SetSFXVolume(App->audio->GetSFXVolume() - step);
	}
	if (IsKeyPressed(KEY_SIX))
	{
		App->audio->SetSFXVolume(App->audio->GetSFXVolume() + step);
	}

	// Mute toggle
	if (IsKeyPressed(KEY_M))
	{
		if (App->audio->GetMasterVolume() > 0.0f)
			App->audio->SetMasterVolume(0.0f);
		else
			App->audio->SetMasterVolume(1.0f);
	}

	// Save settings
	if (IsKeyPressed(KEY_S))
	{
		SaveAudioSettings();
		settingsSavedMessage = true;
		settingsSavedTimer = 0.0f;
	}

	// Close menu
	if (IsKeyPressed(KEY_ESCAPE))
	{
		showAudioSettings = false;
	}
}

// AUDIO SETTINGS: Save to file
void ModuleGame::SaveAudioSettings()
{
	FILE* file = fopen("audio_settings.dat", "wb");
	if (file)
	{
		float master = App->audio->GetMasterVolume();
		float music = App->audio->GetMusicVolume();
		float sfx = App->audio->GetSFXVolume();

		fwrite(&master, sizeof(float), 1, file);
		fwrite(&music, sizeof(float), 1, file);
		fwrite(&sfx, sizeof(float), 1, file);

		fclose(file);
		LOG("Audio settings saved");
	}
	else
	{
		LOG("Failed to save audio settings");
	}
}

// AUDIO SETTINGS: Load from file
void ModuleGame::LoadAudioSettings()
{
	FILE* file = fopen("audio_settings.dat", "rb");
	if (file)
	{
		float master, music, sfx;

		fread(&master, sizeof(float), 1, file);
		fread(&music, sizeof(float), 1, file);
		fread(&sfx, sizeof(float), 1, file);

		App->audio->SetMasterVolume(master);
		App->audio->SetMusicVolume(music);
		App->audio->SetSFXVolume(sfx);

		fclose(file);
		LOG("Audio settings loaded successfully");
	}
	else
	{
		LOG("No saved audio settings found, using defaults");
	}
}