#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"
#include "PhysBody.h"
#include "GameState.h"
#include <string.h>

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	ball = nullptr;
	leftFlipper = nullptr;
	rightFlipper = nullptr;
	kicker = nullptr;
}

ModuleGame::~ModuleGame()
{
}

bool ModuleGame::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;

	font = GetFontDefault();

	titleFont = font;

	// Skip game state system - just play directly
	// InitGameData(&gameData);
	// LoadHighScore();
	// LoadAudioSettings();

	// Load TMX map and create collision boundaries
	if (LoadTMXMap("assets/map/Pinball_Table.tmx"))
	{
		CreateMapCollision();
	}
	else
	{
		LOG("Warning: Could not load TMX map, creating basic boundaries");
		// Create basic pinball table boundaries as fallback
		PhysBody* leftWall = App->physics->CreateRectangle(10, SCREEN_HEIGHT / 2, 20, SCREEN_HEIGHT, b2_staticBody);
		PhysBody* rightWall = App->physics->CreateRectangle(SCREEN_WIDTH - 10, SCREEN_HEIGHT / 2, 20, SCREEN_HEIGHT, b2_staticBody);
		PhysBody* bottomWall = App->physics->CreateRectangle(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 20, b2_staticBody);
		PhysBody* topWall = App->physics->CreateRectangle(SCREEN_WIDTH / 2, 10, SCREEN_WIDTH, 20, b2_staticBody);
	}

	// Create ball (starts enabled for debug/testing)
	ball = App->physics->CreateCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 15, b2_dynamicBody);
	ball->listener = this;

	LOG("Game initialized in MENU state");

	return ret;
}

bool ModuleGame::CleanUp()
{
	LOG("Unloading Intro scene");

	SaveHighScore();

	// TODO: Unload textures
	// UnloadTexture(ballTexture);

	return true;
}

update_status ModuleGame::Update()
{
	// Simple direct play - no menu, no state system
	
	// Draw simple UI
	DrawText("PINBALL - F1: Toggle Debug", 10, 10, 20, WHITE);
	DrawText("Click and drag the ball!", 10, 40, 16, LIGHTGRAY);
	
	// Show mouse position for debugging
	Vector2 mousePos = GetMousePosition();
	DrawText(TextFormat("Mouse: (%.0f, %.0f)", mousePos.x, mousePos.y), 10, 70, 16, YELLOW);
	
	// Draw ball - LARGE and VISIBLE
	if(ball)
	{
		int x, y;
		ball->GetPosition(x, y);
		
		// Draw a big crosshair at ball position
		DrawLine(x - 50, y, x + 50, y, RED);
		DrawLine(x, y - 50, x, y + 50, RED);
		
		// Draw the ball itself - large and blue
		DrawCircle(x, y, 15, BLUE);
		
		// Draw text showing position
		DrawText(TextFormat("Ball: (%d, %d)", x, y), x + 20, y - 20, 16, YELLOW);
	}
	
	return UPDATE_CONTINUE;
}

// ============================================================================
// MENU STATE - Main menu / Start screen
// ============================================================================
void ModuleGame::UpdateMenuState()
{
	// Start game with SPACE
	if (IsKeyPressed(KEY_SPACE))
	{
		LOG("Starting new game from menu");
		ResetGame(&gameData);
		TransitionToState(&gameData, STATE_PLAYING);

		// Enable ball physics
		if (ball) ball->body->SetEnabled(true);
	}

	// Quit with ESC
	if (IsKeyPressed(KEY_ESCAPE))
	{
		// Game will handle quit in Application
	}
}

void ModuleGame::RenderMenuState()
{
	// Clear background
	ClearBackground(Color{ 20, 30, 50, 255 });

	// Title
	const char* title = "POKEMON PINBALL";
	int titleWidth = MeasureText(title, 60);
	DrawText(title, SCREEN_WIDTH / 2 - titleWidth / 2, 150, 60, YELLOW);

	// Instructions
	DrawText("Press SPACE to Start", SCREEN_WIDTH / 2 - 150, 300, 30, WHITE);
	DrawText("Press ESC to Quit", SCREEN_WIDTH / 2 - 120, 350, 25, LIGHTGRAY);

	// Controls
	DrawText("CONTROLS:", 100, 450, 25, SKYBLUE);
	DrawText("LEFT Arrow  - Left Flipper", 100, 490, 20, WHITE);
	DrawText("RIGHT Arrow - Right Flipper", 100, 520, 20, WHITE);
	DrawText("DOWN Arrow  - Launch Ball (hold)", 100, 550, 20, WHITE);
	DrawText("P           - Pause Game", 100, 580, 20, WHITE);
	DrawText("F1          - Debug Mode", 100, 610, 20, WHITE);

	// High score display
	DrawText(TextFormat("High Score: %d", gameData.highestScore),
		SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 100, 25, GOLD);
}

// ============================================================================
// PLAYING STATE - Active gameplay
// ============================================================================
void ModuleGame::UpdatePlayingState()
{
	// Pause with P
	if (IsKeyPressed(KEY_P))
	{
		TransitionToState(&gameData, STATE_PAUSED);
		return;
	}

	// Ball launch control with DOWN arrow
	if (IsKeyDown(KEY_DOWN) && !ballLaunched)
	{
		kickerChargeTime += GetFrameTime();
		kickerForce = MIN(kickerChargeTime * KICKER_CHARGE_SPEED, MAX_KICKER_FORCE);
	}

	if (IsKeyReleased(KEY_DOWN) && !ballLaunched)
	{
		LaunchBall();
	}

	// Flipper controls (placeholder - implement with actual flipper bodies)
	if (IsKeyPressed(KEY_LEFT))
	{
		LOG("Left flipper activated");
		// TODO: Apply impulse to left flipper
		App->audio->PlayFlipperHit(0.7f);
	}

	if (IsKeyPressed(KEY_RIGHT))
	{
		LOG("Right flipper activated");
		// TODO: Apply impulse to right flipper
		App->audio->PlayFlipperHit(0.7f);
	}

	// Check if ball went off screen (lost)
	if (ball)
	{
		int x, y;
		ball->GetPosition(x, y);

		if (y > SCREEN_HEIGHT + 50)
		{
			LoseBall();
		}
	}
}

void ModuleGame::RenderPlayingState()
{
	// Background
	ClearBackground(Color{ 15, 25, 40, 255 });

	// Draw HUD - Score panel
	DrawRectangle(0, 0, 300, 180, Color{ 0, 0, 0, 180 });
	DrawText(TextFormat("SCORE: %d", gameData.currentScore), 20, 20, 25, WHITE);
	DrawText(TextFormat("Previous: %d", gameData.previousScore), 20, 55, 20, GRAY);
	DrawText(TextFormat("High: %d", gameData.highestScore), 20, 85, 20, GOLD);
	DrawText(TextFormat("Balls: %d", gameData.ballsLeft), 20, 120, 25, RED);
	DrawText(TextFormat("Round: %d", gameData.currentRound), 20, 150, 20, SKYBLUE);

	// Combo progress display
	DrawText("COMBO:", SCREEN_WIDTH - 250, 20, 20, YELLOW);
	for (int i = 0; i < 7; i++)
	{
		Color letterColor = (i < gameData.comboProgress) ? GREEN : DARKGRAY;
		DrawText(TextFormat("%c", gameData.comboLetters[i]),
			SCREEN_WIDTH - 180 + i * 25, 20, 25, letterColor);
	}

	// Kicker charge indicator (when charging)
	if (IsKeyDown(KEY_DOWN) && !ballLaunched)
	{
		float chargePercent = kickerForce / MAX_KICKER_FORCE;
		DrawText("CHARGING...", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT - 100, 25, YELLOW);
		DrawRectangle(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 60, 200, 20, DARKGRAY);
		DrawRectangle(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 60,
			(int)(200 * chargePercent), 20, GREEN);
	}

	// Draw ball
	if (ball)
	{
		int x, y;
		ball->GetPosition(x, y);
		DrawCircle(x, y, 15, BLUE);
		// TODO: Draw texture instead
		// DrawTexture(ballTexture, x - 15, y - 15, WHITE);
	}

	// Pause hint
	DrawText("Press P to Pause", SCREEN_WIDTH - 200, SCREEN_HEIGHT - 60, 16, LIGHTGRAY);
}

// ============================================================================
// PAUSED STATE
// ============================================================================
void ModuleGame::UpdatePausedState()
{
	// Resume with P
	if (IsKeyPressed(KEY_P))
	{
		TransitionToState(&gameData, STATE_PLAYING);
	}

	// Return to menu with ESC
	if (IsKeyPressed(KEY_ESCAPE))
	{
		TransitionToState(&gameData, STATE_MENU);
		if (ball) ball->body->SetEnabled(false);
	}
}

void ModuleGame::RenderPausedState()
{
	RenderPlayingState();

	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 180 });

	const char* pauseText = "PAUSED";
	int textWidth = MeasureText(pauseText, 80);
	DrawText(pauseText, SCREEN_WIDTH / 2 - textWidth / 2, 200, 80, YELLOW);

	DrawText("Press P to Resume", SCREEN_WIDTH / 2 - 130, 350, 25, WHITE);
	DrawText("Press ESC to Main Menu", SCREEN_WIDTH / 2 - 150, 400, 25, LIGHTGRAY);
}

void ModuleGame::UpdateGameOverState()
{
	if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE))
	{
		TransitionToState(&gameData, STATE_MENU);
		if (ball) ball->body->SetEnabled(false);
	}
}

void ModuleGame::RenderGameOverState()
{
	ClearBackground(Color{ 30, 10, 10, 255 });

	const char* gameOverText = "GAME OVER";
	int textWidth = MeasureText(gameOverText, 70);
	DrawText(gameOverText, SCREEN_WIDTH / 2 - textWidth / 2, 150, 70, RED);

	DrawText(TextFormat("Final Score: %d", gameData.previousScore),
		SCREEN_WIDTH / 2 - 150, 280, 35, WHITE);

	if (gameData.previousScore == gameData.highestScore && gameData.highestScore > 0)
	{
		DrawText("NEW HIGH SCORE!", SCREEN_WIDTH / 2 - 150, 340, 30, GOLD);
	}

	DrawText(TextFormat("High Score: %d", gameData.highestScore),
		SCREEN_WIDTH / 2 - 140, 380, 30, YELLOW);

	DrawText("Press SPACE to Play Again", SCREEN_WIDTH / 2 - 180, 500, 25, SKYBLUE);
	DrawText("Press ESC to Main Menu", SCREEN_WIDTH / 2 - 160, 550, 22, LIGHTGRAY);
}

// ============================================================================
// GAME LOGIC FUNCTIONS
// ============================================================================

void ModuleGame::LaunchBall()
{
	if (!ball) return;

	LOG("Launching ball with force: %.2f", kickerForce);

	// Apply upward impulse to ball
	b2Vec2 impulse(0.0f, kickerForce);
	ball->body->ApplyLinearImpulseToCenter(impulse, true);

	ballLaunched = true;
	kickerChargeTime = 0.0f;
	kickerForce = 0.0f;

	// Play launch sound
	App->audio->PlayFx(App->audio->LoadFx("assets/audio/flipper_hit.wav"));
}

void ModuleGame::LoseBall()
{
	LOG("Ball lost!");

	// Play ball lost sound
	App->audio->PlayBallLost();

	// Reset ball position
	if (ball)
	{
		ball->body->SetTransform(b2Vec2(SCREEN_WIDTH / 2 * PIXELS_TO_METERS,
			(SCREEN_HEIGHT - 100) * PIXELS_TO_METERS), 0);
		ball->body->SetLinearVelocity(b2Vec2(0, 0));
		ball->body->SetAngularVelocity(0);
	}

	ballLaunched = false;

	// Update game state
	ResetRound(&gameData);

	// Check if game over (handled in ResetRound via TransitionToState)
}

void ModuleGame::AddComboLetter(char letter)
{
	// Check if letter matches next in sequence
	if (gameData.comboProgress < 7 &&
		letter == gameData.comboLetters[gameData.comboProgress])
	{
		gameData.comboProgress++;
		LOG("Combo progress: %d/7 (%c collected)", gameData.comboProgress, letter);

		// Play bonus sound
		App->audio->PlayBonusSound();

		// Award points
		gameData.currentScore += 100;

		// Check if combo complete
		if (gameData.comboProgress >= 7)
		{
			gameData.comboComplete = true;
			gameData.ballsLeft++; // Extra ball!
			gameData.currentScore += 1000; // Bonus points

			LOG("COMBO COMPLETE! Extra ball awarded!");
			App->audio->PlayComboComplete();

			// Reset combo for next round
			gameData.comboProgress = 0;
		}
	}
}

void ModuleGame::SaveHighScore()
{
	FILE* file = fopen("highscore.dat", "wb");
	if (file)
	{
		fwrite(&gameData.highestScore, sizeof(int), 1, file);
		fclose(file);
		LOG("High score saved: %d", gameData.highestScore);
	}
	else
	{
		LOG("Failed to save high score");
	}
}

void ModuleGame::LoadHighScore()
{
	FILE* file = fopen("highscore.dat", "rb");
	if (file)
	{
		fread(&gameData.highestScore, sizeof(int), 1, file);
		fclose(file);
		LOG("High score loaded: %d", gameData.highestScore);
	}
	else
	{
		LOG("No saved high score found");
		gameData.highestScore = 0;
	}
}

// ============================================================================
// COLLISION HANDLING
// ============================================================================

float ModuleGame::CalculateImpactForce(PhysBody* body)
{
	if (!body || !body->body) return 0.0f;

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

	// TODO: Check for bumper collisions
	// TODO: Check for target collisions
	// TODO: Check for combo letter collisions

	// Default: treat as wall
	return COLLISION_WALL;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// Only process collisions during playing state
	if (gameData.currentState != STATE_PLAYING)
		return;

	float impactForce = CalculateImpactForce(bodyA);
	CollisionType type = IdentifyCollision(bodyA, bodyB);

	switch (type)
	{
	case COLLISION_FLIPPER:
		App->audio->PlayFlipperHit(impactForce);
		gameData.currentScore += 10;
		break;

	case COLLISION_BUMPER:
		App->audio->PlayBumperHit(impactForce);
		gameData.currentScore += 50;
		break;

	case COLLISION_TARGET:
		App->audio->PlayBonusSound();
		gameData.currentScore += 100;
		break;

	case COLLISION_COMBO_LETTER:
		// TODO: Identify which letter and add to combo
		// AddComboLetter('P');
		break;

	case COLLISION_WALL:
	default:
		// Soft wall collision sound
		App->audio->PlayBumperHit(impactForce * 0.3f);
		gameData.currentScore += 5;
		break;
	}
}

void ModuleGame::DrawAudioSettings()
{
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 200 });

	DrawText("AUDIO SETTINGS", SCREEN_WIDTH / 2 - 150, 50, 30, WHITE);
	DrawText("Press F2 to close", SCREEN_WIDTH / 2 - 100, 90, 16, GRAY);

	int startY = 150;
	int spacing = 80;

	DrawText("Master Volume:", 100, startY, 20, WHITE);
	DrawText(TextFormat("%.0f%%", App->audio->GetMasterVolume() * 100), 400, startY, 20, YELLOW);
	DrawRectangle(100, startY + 30, 400, 20, DARKGRAY);
	DrawRectangle(100, startY + 30, (int)(400 * App->audio->GetMasterVolume()), 20, GREEN);
	DrawText("[1/2] Decrease/Increase", 520, startY + 5, 16, LIGHTGRAY);

	DrawText("Music Volume:", 100, startY + spacing, 20, WHITE);
	DrawText(TextFormat("%.0f%%", App->audio->GetMusicVolume() * 100), 400, startY + spacing, 20, YELLOW);
	DrawRectangle(100, startY + spacing + 30, 400, 20, DARKGRAY);
	DrawRectangle(100, startY + spacing + 30, (int)(400 * App->audio->GetMusicVolume()), 20, BLUE);
	DrawText("[3/4] Decrease/Increase", 520, startY + spacing + 5, 16, LIGHTGRAY);

	DrawText("SFX Volume:", 100, startY + spacing * 2, 20, WHITE);
	DrawText(TextFormat("%.0f%%", App->audio->GetSFXVolume() * 100), 400, startY + spacing * 2, 20, YELLOW);
	DrawRectangle(100, startY + spacing * 2 + 30, 400, 20, DARKGRAY);
	DrawRectangle(100, startY + spacing * 2 + 30, (int)(400 * App->audio->GetSFXVolume()), 20, RED);
	DrawText("[5/6] Decrease/Increase", 520, startY + spacing * 2 + 5, 16, LIGHTGRAY);

	DrawText("Mute All: [M]", 100, startY + spacing * 3, 20, WHITE);
	if (App->audio->GetMasterVolume() == 0.0f)
		DrawText("MUTED", 300, startY + spacing * 3, 20, RED);
	else
		DrawText("ACTIVE", 300, startY + spacing * 3, 20, GREEN);

	DrawText("Press [S] to save settings", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT - 80, 18, GREEN);

	if (settingsSavedMessage)
	{
		DrawText("Settings Saved!", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT - 50, 20, LIME);
	}
}

void ModuleGame::UpdateAudioSettings()
{
	float step = 0.05f;

	if (IsKeyPressed(KEY_ONE))
		App->audio->SetMasterVolume(App->audio->GetMasterVolume() - step);

	if (IsKeyPressed(KEY_TWO))
		App->audio->SetMasterVolume(App->audio->GetMasterVolume() + step);

	if (IsKeyPressed(KEY_THREE))
		App->audio->SetMusicVolume(App->audio->GetMusicVolume() - step);

	if (IsKeyPressed(KEY_FOUR))
		App->audio->SetMusicVolume(App->audio->GetMusicVolume() + step);

	if (IsKeyPressed(KEY_FIVE))
		App->audio->SetSFXVolume(App->audio->GetSFXVolume() - step);
	if (IsKeyPressed(KEY_SIX))
		App->audio->SetSFXVolume(App->audio->GetSFXVolume() + step);

	if (IsKeyPressed(KEY_M))
	{
		if (App->audio->GetMasterVolume() > 0.0f)
			App->audio->SetMasterVolume(0.0f);
		else
			App->audio->SetMasterVolume(1.0f);
	}

	if (IsKeyPressed(KEY_S))
	{
		SaveAudioSettings();
		settingsSavedMessage = true;
		settingsSavedTimer = 0.0f;
	}

	if (IsKeyPressed(KEY_ESCAPE))
	{
		showAudioSettings = false;
	}
}

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
}

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
		LOG("Audio settings loaded");
	}
	else
	{
		LOG("No saved audio settings, using defaults");
	}
}

// ============================================================================
// TMX MAP LOADING
// ============================================================================

bool ModuleGame::LoadTMXMap(const char* filepath)
{
	LOG("Loading TMX map: %s", filepath);

	// Load the file
	FILE* file = fopen(filepath, "r");
	if (!file)
	{
		LOG("Failed to open TMX file: %s", filepath);
		return false;
	}

	// Read entire file into memory
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* fileContent = (char*)malloc(fileSize + 1);
	fread(fileContent, 1, fileSize, file);
	fileContent[fileSize] = '\0';
	fclose(file);

	// Find the polyline points attribute
	const char* polylineMarker = "<polyline points=\"";
	char* polylineStart = strstr(fileContent, polylineMarker);

	if (!polylineStart)
	{
		LOG("No polyline found in TMX file");
		free(fileContent);
		return false;
	}

	// Skip the marker
	polylineStart += strlen(polylineMarker);

	// Find the end quote
	char* polylineEnd = strchr(polylineStart, '"');
	if (!polylineEnd)
	{
		LOG("Malformed polyline in TMX file");
		free(fileContent);
		return false;
	}

	// Copy the points string
	size_t pointsLen = polylineEnd - polylineStart;
	char* pointsStr = (char*)malloc(pointsLen + 1);
	strncpy(pointsStr, polylineStart, pointsLen);
	pointsStr[pointsLen] = '\0';

	// Parse the points
	mapCollisionPoints.clear();

	char* token = strtok(pointsStr, " ,");
	while (token != NULL)
	{
		float value = (float)atof(token);
		mapCollisionPoints.push_back((int)value);
		token = strtok(NULL, " ,");
	}

	free(pointsStr);
	free(fileContent);

	LOG("Loaded %d collision points from TMX", (int)mapCollisionPoints.size());

	return true;
}

void ModuleGame::CreateMapCollision()
{
	if (mapCollisionPoints.empty())
	{
		LOG("No map collision points to create");
		return;
	}

	// The TMX map is 1280x1280 pixels (40x40 tiles at 32px each)
	// Our screen is 720x1280 pixels (portrait)
	// Scale to fit width: 720/1280 = 0.5625
	float scale = 720.0f / 1280.0f;
	
	// The polyline in TMX has points ranging roughly from Y=-589 to Y=+514 (relative to object)
	// Object is at (259, 767) in TMX coordinates
	// This gives absolute Y range: 767-589=178 (top) to 767+514=1281 (bottom)
	// After scaling: 178*0.5625=100 to 1281*0.5625=720
	// Perfect! It fits our 720 height, but we need to shift it to fill the 1280 height
	
	// The polyline object starts at position (258.667, 766.667) in the TMX
	int objectX = (int)(259 * scale);
	
	// Offset Y to center the map vertically in our taller screen
	// TMX map height after scaling = 720 pixels
	// Our screen height = 1280 pixels
	// Vertical offset needed = (1280 - 720) / 2 = 280 pixels from bottom
	int objectY = 1280 - (int)(767 * scale); // Start from bottom and offset up

	LOG("Creating map collision with %d points", (int)mapCollisionPoints.size() / 2);
	LOG("Scale factor: %.3f, Object origin: (%d, %d)", scale, objectX, objectY);

	// Scale all the collision points (these are relative to object position)
	std::vector<int> scaledPoints;
	scaledPoints.reserve(mapCollisionPoints.size());
	
	for (size_t i = 0; i < mapCollisionPoints.size(); i += 2)
	{
		// Scale both X and Y coordinates
		int scaledX = (int)(mapCollisionPoints[i] * scale);
		int scaledY = (int)(mapCollisionPoints[i + 1] * scale);
		
		scaledPoints.push_back(scaledX);
		scaledPoints.push_back(scaledY);
	}

	LOG("First 3 points (scaled): (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)", 
		scaledPoints[0]*1.0f, scaledPoints[1]*1.0f,
		scaledPoints[2]*1.0f, scaledPoints[3]*1.0f,
		scaledPoints[4]*1.0f, scaledPoints[5]*1.0f);

	// Create the chain collision with scaled points
	mapBoundary = App->physics->CreateChain(objectX, objectY, 
		scaledPoints.data(), 
		(int)scaledPoints.size(), 
		b2_staticBody);

	if (mapBoundary)
	{
		LOG("Map collision created successfully with scaled coordinates");
	}
	else
	{
		LOG("Failed to create map collision");
	}
}