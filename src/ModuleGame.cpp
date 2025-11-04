#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"
#include "PhysBody.h"
#include "GameState.h"
#include <string.h>

// Constructor / destructor
ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	// Initialize members to safe defaults
	ball = nullptr;
	leftFlipper = nullptr;
	rightFlipper = nullptr;
	kicker = nullptr;

	leftFlipperJoint = nullptr;
	rightFlipperJoint = nullptr;

	// Textures already zero-initialized in header, but keep safe resets
	backgroundTexture = { 0 };
	titleTexture = { 0 };
	ballTexture = { 0 };
	flipperTexture = { 0 };
	flipperBaseTexture = { 0 };
	bumper1Texture = { 0 };
	bumper2Texture = { 0 };
	bumper3Texture = { 0 };
	piece1Texture = { 0 };
	piece2Texture = { 0 };

	font = { 0 };
	titleFont = { 0 };

	showAudioSettings = false;
	settingsSavedMessage = false;
	settingsSavedTimer = 0.0f;

	ballLaunched = false;
	kickerForce = 0.0f;
	kickerChargeTime = 0.0f;

	mapBoundary = nullptr;
}

ModuleGame::~ModuleGame()
{
}

// --------------------------------------------------------------------------
// Start
// --------------------------------------------------------------------------
bool ModuleGame::Start()
{
	LOG("ModuleGame Start(): loading assets");
	bool ret = true;

	// Fonts
	font = GetFontDefault();
	titleFont = GetFontDefault(); // Use default font instead of loading

	// Load textures with error checking
	backgroundTexture = LoadTexture("assets/background/BG.png");
	if(backgroundTexture.id == 0) LOG("Warning: Failed to load background texture");
	
	ballTexture = LoadTexture("assets/balls/Planet1.png");
	if(ballTexture.id == 0) LOG("Warning: Failed to load ball texture");
	
	flipperTexture = LoadTexture("assets/flippers/flipper bat.png");
	if(flipperTexture.id == 0) LOG("Warning: Failed to load flipper texture");
	
	flipperBaseTexture = LoadTexture("assets/flippers/Base Flipper Bat.png");
	if(flipperBaseTexture.id == 0) LOG("Warning: Failed to load flipper base texture");
	
	bumper1Texture = LoadTexture("assets/bumpers/bumper1.png");
	if(bumper1Texture.id == 0) LOG("Warning: Failed to load bumper1 texture");
	
	bumper2Texture = LoadTexture("assets/bumpers/bumper2.png");
	if(bumper2Texture.id == 0) LOG("Warning: Failed to load bumper2 texture");
	
	bumper3Texture = LoadTexture("assets/bumpers/bumper3.png");
	if(bumper3Texture.id == 0) LOG("Warning: Failed to load bumper3 texture");
	
	piece1Texture = LoadTexture("assets/extra/piece1.png");
	if(piece1Texture.id == 0) LOG("Warning: Failed to load piece1 texture");
	
	piece2Texture = LoadTexture("assets/extra/piece2.png");
	if(piece2Texture.id == 0) LOG("Warning: Failed to load piece2 texture");

	// Load title texture
	titleTexture = LoadTexture("assets/UI/title.png");
	if(titleTexture.id == 0) LOG("Warning: Failed to load title texture");
	
	// Load sound effects ONCE (not every collision!)
	bumperHitSfx = App->audio->LoadFx("assets/audio/bumper_hit.wav");
	LOG("Loaded bumper hit sound effect: %d", bumperHitSfx);

	// Load saved preferences
	LoadAudioSettings();
	LoadHighScore();

	// Load TMX map (optional)
	if (LoadTMXMap("assets/map/Pinball_Table.tmx"))
	{
		CreateMapCollision();
	}
	else
	{
		LOG("Warning: Could not load TMX map");
	}

	// Create ball and prevent it from moving until game starts
	ball = App->physics->CreateCircle(2.48f * METERS_TO_PIXELS, SCREEN_HEIGHT - (7.44f * METERS_TO_PIXELS), 15, b2_dynamicBody);

	if (ball)
	{
		ball->listener = this;
		if (ball->body)
			ball->body->SetEnabled(false);
	}

	// Example bumpers - positioned for portrait mode relative to screen
	int centerX = SCREEN_WIDTH / 2;
	int bumperY = 300; // near upper area
	PhysBody* b1 = App->physics->CreateCircle(centerX - 80, bumperY + 30, 30, b2_staticBody);
	if (b1)
	{
		if (b1->body && b1->body->GetFixtureList())
			b1->body->GetFixtureList()->SetRestitution(1.5f);
		b1->listener = this;
		bumpers.push_back(b1);
	}
	PhysBody* b2 = App->physics->CreateCircle(centerX, bumperY, 30, b2_staticBody);
	if (b2)
	{
		if (b2->body && b2->body->GetFixtureList())
			b2->body->GetFixtureList()->SetRestitution(1.5f);
		b2->listener = this;
		bumpers.push_back(b2);
	}
	PhysBody* b3 = App->physics->CreateCircle(centerX + 80, bumperY + 30, 30, b2_staticBody);
	if (b3)
	{
		if (b3->body && b3->body->GetFixtureList())
			b3->body->GetFixtureList()->SetRestitution(1.5f);
		b3->listener = this;
		bumpers.push_back(b3);
	}

	// Create flippers - position near bottom relative to height
	int flipperY = SCREEN_HEIGHT - 180;
	int leftX = (int)(SCREEN_WIDTH * 0.35f);
	int rightX = (int)(SCREEN_WIDTH * 0.65f);
	leftFlipperJoint = App->physics->CreateFlipper(leftX, flipperY, 80, 20, true, &leftFlipper);
	rightFlipperJoint = App->physics->CreateFlipper(rightX, flipperY, 80, 20, false, &rightFlipper);
	if (leftFlipper) leftFlipper->listener = this;
	if (rightFlipper) rightFlipper->listener = this;

	// Initialize GameData
	InitGameData(&gameData);

	LOG("ModuleGame Start complete");
	return ret;
}

// --------------------------------------------------------------------------
// CleanUp
// --------------------------------------------------------------------------
bool ModuleGame::CleanUp()
{
	LOG("ModuleGame CleanUp(): unloading assets and freeing resources");

	SaveHighScore();
	SaveAudioSettings();

	// Unload textures if loaded
	if (backgroundTexture.id) UnloadTexture(backgroundTexture);
	if (ballTexture.id) UnloadTexture(ballTexture);
	if (flipperTexture.id) UnloadTexture(flipperTexture);
	if (flipperBaseTexture.id) UnloadTexture(flipperBaseTexture);
	if (bumper1Texture.id) UnloadTexture(bumper1Texture);
	if (bumper2Texture.id) UnloadTexture(bumper2Texture);
	if (bumper3Texture.id) UnloadTexture(bumper3Texture);
	if (piece1Texture.id) UnloadTexture(piece1Texture);
	if (piece2Texture.id) UnloadTexture(piece2Texture);

	// Menu UI textures
	if (titleTexture.id) UnloadTexture(titleTexture);

	// Fonts
	if (titleFont.texture.id) UnloadFont(titleFont);

	bumpers.clear();
	mapCollisionPoints.clear();

	return true;
}

// --------------------------------------------------------------------------
// Update - main loop
// --------------------------------------------------------------------------
update_status ModuleGame::Update()
{
	// If audio settings UI open, handle it
	if (IsKeyPressed(KEY_F2))
	{
		showAudioSettings = !showAudioSettings;
	}

	if (showAudioSettings)
	{
		UpdateAudioSettings();
		DrawAudioSettings();
		return UPDATE_CONTINUE;
	}

	// State machine
	switch (gameData.currentState)
	{
	case STATE_MENU:
		UpdateMenuState();
		RenderMenuState();
		break;

	case STATE_PLAYING:
		UpdatePlayingState();
		RenderPlayingState();
		break;

	case STATE_PAUSED:
		UpdatePausedState();
		RenderPausedState();
		break;

	case STATE_GAME_OVER:
		UpdateGameOverState();
		RenderGameOverState();
		break;

	default:
		UpdatePlayingState();
		RenderPlayingState();
		break;
	}

	// Global debug toggle
	if (IsKeyPressed(KEY_F1))
		showDebug = !showDebug;

	// Manage settings save message timer
	if (settingsSavedMessage)
	{
		settingsSavedTimer += GetFrameTime();
		if (settingsSavedTimer >= 2.0f)
		{
			settingsSavedMessage = false;
			settingsSavedTimer = 0.0f;
		}
	}

	return UPDATE_CONTINUE;
}

// ============================================================================
// MENU STATE - Main menu / Start screen
// ============================================================================
void ModuleGame::UpdateMenuState()
{
	// Ensure ball physics disabled while in menu
	if (ball && ball->body) ball->body->SetEnabled(false);

	// Start game with SPACE
	if (IsKeyPressed(KEY_SPACE))
	{
		LOG("Starting new game from menu");
		ResetGame(&gameData);
		TransitionToState(&gameData, STATE_PLAYING);

		// Enable ball and reset position
		if (ball && ball->body)
		{
			ball->body->SetEnabled(true);
			ball->body->SetTransform(b2Vec2(2.48f, 7.44f), 0);
			ball->body->SetLinearVelocity(b2Vec2(0, 0));
			ball->body->SetAngularVelocity(0);
		}

		ballLaunched = false;
	}

	// Quit with ESC
	if (IsKeyPressed(KEY_ESCAPE))
	{
		// App->window->Close(); // if available
	}
}

void ModuleGame::RenderMenuState()
{
	if (backgroundTexture.id)
	{
		// Scale background to fill portrait screen
		Rectangle src = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Rectangle dst = { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
		Vector2 origin = { 0, 0 };
		DrawTexturePro(backgroundTexture, src, dst, origin, 0.0f, WHITE);
	}
	else
		ClearBackground(Color{ 20,30,50,255 });

	int screenCenterX = SCREEN_WIDTH / 2;
	int titleY = SCREEN_HEIGHT * 0.15f;
	int startTextY = SCREEN_HEIGHT * 0.55f;
	int highScoreY = SCREEN_HEIGHT * 0.75f;
	int controlsY = SCREEN_HEIGHT - 30;

	if (titleTexture.id)
	{
		float scale = 0.25f;
		int titleWidth = (int)(titleTexture.width * scale);
		int titleHeight = (int)(titleTexture.height * scale);
		int titleX = screenCenterX - titleWidth / 2;

		DrawTextureEx(titleTexture, Vector2{ (float)titleX, (float)titleY }, 0.0f, scale, WHITE);
	}
	else
	{
		const char* title = "SPACE PINBALL";
		int fontSize = 40;
		int textWidth = MeasureText(title, fontSize);
		DrawText(title, screenCenterX - textWidth / 2, titleY, fontSize, YELLOW);
	}

	const char* startText = "PRESS SPACE TO START";
	int fontSize = 24;
	int textWidth = MeasureText(startText, fontSize);
	DrawText(startText, screenCenterX - textWidth / 2, startTextY, fontSize, WHITE);

	const char* highScoreText = TextFormat("High Score: %d", gameData.highestScore);
	int highScoreWidth = MeasureText(highScoreText, 22);
	DrawText(highScoreText, screenCenterX - highScoreWidth / 2, highScoreY, 22, GOLD);

	const char* controlsText = "LEFT/RIGHT - Flippers | DOWN - Launch | P - Pause";
	int controlsWidth = MeasureText(controlsText, 14);
	DrawText(controlsText, screenCenterX - controlsWidth / 2, controlsY, 14, LIGHTGRAY);
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

	// Ball launch control
	if (IsKeyDown(KEY_DOWN) && !ballLaunched)
	{
		kickerChargeTime += GetFrameTime();
		kickerForce = MIN(kickerChargeTime * KICKER_CHARGE_SPEED, MAX_KICKER_FORCE);
	}
	if (IsKeyReleased(KEY_DOWN) && !ballLaunched)
	{
		LaunchBall();
	}

	// Flipper controls via joints
	if (leftFlipperJoint)
	{
		if (IsKeyDown(KEY_LEFT))
			leftFlipperJoint->SetMotorSpeed(-20.0f);
		else
			leftFlipperJoint->SetMotorSpeed(10.0f);
	}

	if (rightFlipperJoint)
	{
		if (IsKeyDown(KEY_RIGHT))
			rightFlipperJoint->SetMotorSpeed(20.0f);
		else
			rightFlipperJoint->SetMotorSpeed(-10.0f);
	}

	// Check ball out-of-bounds
	if (ball)
	{
		int x, y;
		ball->GetPosition(x, y);
		if (y > SCREEN_HEIGHT + 50)
			LoseBall();
	}
}

void ModuleGame::RenderPlayingState()
{
	// Background
	if (backgroundTexture.id)
	{
		Rectangle src = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Rectangle dst = { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
		Vector2 origin = { 0, 0 };
		DrawTexturePro(backgroundTexture, src, dst, origin, 0.0f, WHITE);
	}
	else
		ClearBackground(Color{ 15,25,40,255 });

	// HUD panel
	DrawRectangle(0, 0, 300, 180, Color{ 0,0,0,180 });
	DrawText(TextFormat("SCORE: %d", gameData.currentScore), 20, 20, 25, WHITE);
	DrawText(TextFormat("Previous: %d", gameData.previousScore), 20, 55, 20, GRAY);
	DrawText(TextFormat("High: %d", gameData.highestScore), 20, 85, 20, GOLD);
	DrawText(TextFormat("Balls: %d", gameData.ballsLeft), 20, 120, 25, RED);
	DrawText(TextFormat("Round: %d", gameData.currentRound), 20, 150, 20, SKYBLUE);

	// Combo letters
	DrawText("COMBO:", SCREEN_WIDTH - 250, 20, 20, YELLOW);
	for (int i = 0; i < 7; ++i)
	{
		Color letterColor = (i < gameData.comboProgress) ? GREEN : DARKGRAY;
		DrawText(TextFormat("%c", gameData.comboLetters[i]), SCREEN_WIDTH - 180 + i * 25, 20, 25, letterColor);
	}

	// Kicker charge indicator
	if (IsKeyDown(KEY_DOWN) && !ballLaunched)
	{
		float chargePercent = kickerForce / MAX_KICKER_FORCE;
		DrawText("CHARGING...", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT - 100, 25, YELLOW);
		DrawRectangle(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 60, 200, 20, DARKGRAY);
		DrawRectangle(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 60, (int)(200 * chargePercent), 20, GREEN);
	}

	// Draw bumpers
	for (size_t i = 0; i < bumpers.size(); ++i)
	{
		int x = 0, y = 0;
		if (!bumpers[i]) continue;
		bumpers[i]->GetPosition(x, y);

		Texture2D* bumperTex = nullptr;
		if (i == 0) bumperTex = &bumper1Texture;
		else if (i == 1) bumperTex = &bumper2Texture;
		else bumperTex = &bumper3Texture;

		if (bumperTex && bumperTex->id)
		{
			float scale = 60.0f / (float)bumperTex->width;
			int width = (int)(bumperTex->width * scale);
			int height = (int)(bumperTex->height * scale);
			Rectangle src = { 0,0,(float)bumperTex->width,(float)bumperTex->height };
			Rectangle dst = { (float)x, (float)y, (float)width, (float)height };
			Vector2 origin = { width / 2.0f, height / 2.0f };
			DrawTexturePro(*bumperTex, src, dst, origin, 0.0f, WHITE);
		}
		else
		{
			DrawCircle(x, y, 30, ORANGE);
		}
	}

	// Draw flippers
	if (leftFlipper && leftFlipper->body)
	{
		int x, y;
		leftFlipper->GetPosition(x, y);
		float angle = leftFlipper->body->GetAngle() * RADTODEG;

		if (flipperBaseTexture.id)
		{
			float bs = 30.0f / (float)flipperBaseTexture.width;
			int bw = (int)(flipperBaseTexture.width * bs);
			int bh = (int)(flipperBaseTexture.height * bs);
			Rectangle src = { 0,0,(float)flipperBaseTexture.width,(float)flipperBaseTexture.height };
			Rectangle dst = { (float)x, (float)y, (float)bw, (float)bh };
			Vector2 origin = { bw / 2.0f, bh / 2.0f };
			DrawTexturePro(flipperBaseTexture, src, dst, origin, 0.0f, WHITE);
		}

		if (flipperTexture.id)
		{
			float s = 80.0f / (float)flipperTexture.width;
			int w = (int)(flipperTexture.width * s);
			int h = (int)(flipperTexture.height * s);
			Rectangle src = { 0,0,(float)flipperTexture.width,(float)flipperTexture.height };
			Rectangle dst = { (float)x, (float)y, (float)w, (float)h };
			Vector2 origin = { w * 0.2f, h / 2.0f };
			DrawTexturePro(flipperTexture, src, dst, origin, angle, WHITE);
		}
	}

	if (rightFlipper && rightFlipper->body)
	{
		int x, y;
		rightFlipper->GetPosition(x, y);
		float angle = rightFlipper->body->GetAngle() * RADTODEG;

		if (flipperBaseTexture.id)
		{
			float bs = 30.0f / (float)flipperBaseTexture.width;
			int bw = (int)(flipperBaseTexture.width * bs);
			int bh = (int)(flipperBaseTexture.height * bs);
			Rectangle src = { 0,0,(float)flipperBaseTexture.width,(float)flipperBaseTexture.height };
			Rectangle dst = { (float)x, (float)y, (float)bw, (float)bh };
			Vector2 origin = { bw / 2.0f, bh / 2.0f };
			DrawTexturePro(flipperBaseTexture, src, dst, origin, 0.0f, WHITE);
		}

		if (flipperTexture.id)
		{
			float s = 80.0f / (float)flipperTexture.width;
			int w = (int)(flipperTexture.width * s);
			int h = (int)(flipperTexture.height * s);
			Rectangle src = { 0,0,(float)flipperTexture.width,(float)flipperTexture.height };
			Rectangle dst = { (float)x, (float)y, (float)w, (float)h };
			Vector2 origin = { w * 0.8f, h / 2.0f };
			DrawTexturePro(flipperTexture, src, dst, origin, angle, WHITE);
		}
	}

	// Ball drawing
	if (ball && ball->body)
	{
		int x, y;
		ball->GetPosition(x, y);
		if (ballTexture.id)
		{
			float s = 30.0f / (float)ballTexture.width;
			int w = (int)(ballTexture.width * s);
			int h = (int)(ballTexture.height * s);
			Rectangle src = { 0,0,(float)ballTexture.width,(float)ballTexture.height };
			Rectangle dst = { (float)x, (float)y, (float)w, (float)h };
			Vector2 origin = { w / 2.0f, h / 2.0f };
			DrawTexturePro(ballTexture, src, dst, origin, 0.0f, WHITE);
		}
		else
		{
			DrawCircle(x, y, 15, BLUE);
		}
	}

	// Pause hint
	DrawText("Press P to Pause", SCREEN_WIDTH - 200, SCREEN_HEIGHT - 60, 16, LIGHTGRAY);
}

// ============================================================================
// PAUSED STATE
// ============================================================================
void ModuleGame::UpdatePausedState()
{
	// Resume with P or SPACE
	if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
	{
		TransitionToState(&gameData, STATE_PLAYING);
	}

	// Return to menu with M
	if (IsKeyPressed(KEY_M))
	{
		TransitionToState(&gameData, STATE_MENU);
		if (ball && ball->body) ball->body->SetEnabled(false);
	}
}

void ModuleGame::RenderPausedState()
{
	// Render playing scene as background
	RenderPlayingState();

	// Dark overlay
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0, 0, 0, 180 });

	const char* pauseText = "PAUSED";
	int textWidth = MeasureText(pauseText, 80);
	DrawText(pauseText, SCREEN_WIDTH / 2 - textWidth / 2, 200, 80, YELLOW);

	DrawText("Press P or SPACE to Resume", SCREEN_WIDTH / 2 - 180, 350, 25, WHITE);
	DrawText("Press M to Main Menu", SCREEN_WIDTH / 2 - 150, 400, 25, LIGHTGRAY);
}

// ============================================================================
// GAME OVER STATE
// ============================================================================
void ModuleGame::UpdateGameOverState()
{
	if (IsKeyPressed(KEY_SPACE))
	{
		TransitionToState(&gameData, STATE_MENU);
		if (ball && ball->body) ball->body->SetEnabled(false);
	}
}

void ModuleGame::RenderGameOverState()
{
	// Background
	if (backgroundTexture.id)
	{
		Rectangle src = { 0, 0, (float)backgroundTexture.width, (float)backgroundTexture.height };
		Rectangle dst = { 0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT };
		Vector2 origin = { 0, 0 };
		DrawTexturePro(backgroundTexture, src, dst, origin, 0.0f, WHITE);
	}
	else
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

	DrawText("Press SPACE to Main Menu", SCREEN_WIDTH / 2 - 180, 500, 25, SKYBLUE);
}

// ============================================================================
// GAME LOGIC FUNCTIONS (sin cambios)
// ============================================================================
void ModuleGame::LaunchBall()
{
	if (!ball || !ball->body) return;

	LOG("Launching ball with force: %.2f", kickerForce);

	b2Vec2 impulse(0.0f, -kickerForce);
	ball->body->ApplyLinearImpulseToCenter(impulse, true);

	ballLaunched = true;
	kickerChargeTime = 0.0f;
	kickerForce = 0.0f;

	int fx = App->audio->LoadFx("assets/audio/flipper_hit.wav");
	if (fx >= 0) App->audio->PlayFx(fx);
}

void ModuleGame::LoseBall()
{
	LOG("Ball lost!");

	App->audio->PlayBallLost();

	if (ball && ball->body)
	{
		ball->body->SetTransform(b2Vec2(2.48f, 7.44f), 0);
		ball->body->SetLinearVelocity(b2Vec2(0, 0));
		ball->body->SetAngularVelocity(0);
		ball->body->SetEnabled(false);
	}


	ballLaunched = false;

	ResetRound(&gameData);
}

void ModuleGame::AddComboLetter(char letter)
{
	if (gameData.comboProgress < 7 &&
		letter == gameData.comboLetters[gameData.comboProgress])
	{
		gameData.comboProgress++;
		App->audio->PlayBonusSound();
		gameData.currentScore += 100;

		if (gameData.comboProgress >= 7)
		{
			gameData.comboComplete = true;
			gameData.ballsLeft++;
			gameData.currentScore += 1000;
			App->audio->PlayComboComplete();
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
}

void ModuleGame::LoadHighScore()
{
	FILE* file = fopen("highscore.dat", "rb");
	if (file)
	{
		int loadedScore = 0;
		size_t read = fread(&loadedScore, sizeof(int), 1, file);
		fclose(file);
		
		// Validate the loaded score (prevent corruption)
		if (read == 1 && loadedScore >= 0 && loadedScore < 10000000)
		{
			gameData.highestScore = loadedScore;
			LOG("High score loaded: %d", gameData.highestScore);
		}
		else
		{
			gameData.highestScore = 0;
			LOG("Invalid high score in file, reset to 0");
		}
	}
	else
	{
		gameData.highestScore = 0;
		LOG("No high score file found, starting at 0");
	}
}

// ============================================================================
// COLLISION HANDLING (sin cambios)
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
	if (bodyB == leftFlipper || bodyB == rightFlipper) return COLLISION_FLIPPER;
	return COLLISION_WALL;
}

void ModuleGame::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	for (size_t i = 0; i < bumpers.size(); ++i)
	{
		if (bodyA == bumpers[i] || bodyB == bumpers[i])
		{
			if (ball && ball->body && (bodyA == ball || bodyB == ball))
			{
				b2Vec2 vel = ball->body->GetLinearVelocity();
				vel *= 1.3f;
				ball->body->SetLinearVelocity(vel);
				
				// Play pre-loaded sound effect
				if (bumperHitSfx >= 0) 
				{
					App->audio->PlayFx(bumperHitSfx);
				}
				
				gameData.currentScore += 50;
				LOG("Bumper hit! Score: %d", gameData.currentScore);
			}
			break;
		}
	}
}

// ============================================================================
// AUDIO SETTINGS UI (sin cambios)
// ============================================================================
void ModuleGame::DrawAudioSettings()
{
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color{ 0,0,0,200 });
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
	if (App->audio->GetMasterVolume() == 0.0f) DrawText("MUTED", 300, startY + spacing * 3, 20, RED);
	else DrawText("ACTIVE", 300, startY + spacing * 3, 20, GREEN);

	DrawText("Press [S] to save settings", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT - 80, 18, GREEN);
	if (settingsSavedMessage) DrawText("Settings Saved!", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT - 50, 20, LIME);
}

void ModuleGame::UpdateAudioSettings()
{
	float step = 0.05f;
	if (IsKeyPressed(KEY_ONE)) App->audio->SetMasterVolume(App->audio->GetMasterVolume() - step);
	if (IsKeyPressed(KEY_TWO)) App->audio->SetMasterVolume(App->audio->GetMasterVolume() + step);
	if (IsKeyPressed(KEY_THREE)) App->audio->SetMusicVolume(App->audio->GetMusicVolume() - step);
	if (IsKeyPressed(KEY_FOUR)) App->audio->SetMusicVolume(App->audio->GetMusicVolume() + step);
	if (IsKeyPressed(KEY_FIVE)) App->audio->SetSFXVolume(App->audio->GetSFXVolume() - step);
	if (IsKeyPressed(KEY_SIX)) App->audio->SetSFXVolume(App->audio->GetSFXVolume() + step);
	if (IsKeyPressed(KEY_M))
	{
		if (App->audio->GetMasterVolume() > 0.0f) App->audio->SetMasterVolume(0.0f);
		else App->audio->SetMasterVolume(1.0f);
	}
	if (IsKeyPressed(KEY_S))
	{
		SaveAudioSettings();
		settingsSavedMessage = true;
		settingsSavedTimer = 0.0f;
	}
	if (IsKeyPressed(KEY_ESCAPE)) showAudioSettings = false;
}

// ============================================================================
// TMX MAP LOADING (sin cambios)
// ============================================================================
bool ModuleGame::LoadTMXMap(const char* filepath)
{
	LOG("Loading TMX map: %s", filepath);

	FILE* file = fopen(filepath, "r");
	if (!file)
	{
		LOG("Failed to open TMX file: %s", filepath);
		return false;
	}

	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* fileContent = (char*)malloc(fileSize + 1);
	fread(fileContent, 1, fileSize, file);
	fileContent[fileSize] = '\0';
	fclose(file);

	const char* polylineMarker = "<polyline points=\"";
	char* polylineStart = strstr(fileContent, polylineMarker);
	if (!polylineStart)
	{
		free(fileContent);
		return false;
	}

	polylineStart += strlen(polylineMarker);
	char* polylineEnd = strchr(polylineStart, '"');
	if (!polylineEnd)
	{
		free(fileContent);
		return false;
	}

	size_t pointsLen = polylineEnd - polylineStart;
	char* pointsStr = (char*)malloc(pointsLen + 1);
	strncpy(pointsStr, polylineStart, pointsLen);
	pointsStr[pointsLen] = '\0';

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

	// TMX map is 23×32 tiles at 32px/tile = 736×1024 pixels
	// Polyline points are relative to object origin, already centered around (0,0)
	const int TMX_MAP_W = 736;  // 23 * 32
	const int TMX_MAP_H = 1024; // 32 * 32
	
	// Scale to fit screen width
	float scale = (float)SCREEN_WIDTH / (float)TMX_MAP_W; // 720/736 ≈ 0.978

	// Scale polyline points (they're already relative/centered)
	std::vector<int> scaledPoints;
	scaledPoints.reserve(mapCollisionPoints.size());
	
	// First pass: scale points and find their bounding box
	int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
	for (size_t i = 0; i < mapCollisionPoints.size(); i += 2)
	{
		int scaledX = (int)roundf(mapCollisionPoints[i] * scale);
		int scaledY = (int)roundf(mapCollisionPoints[i + 1] * scale);
		scaledPoints.push_back(scaledX);
		scaledPoints.push_back(scaledY);
		
		if (scaledX < minX) minX = scaledX;
		if (scaledX > maxX) maxX = scaledX;
		if (scaledY < minY) minY = scaledY;
		if (scaledY > maxY) maxY = scaledY;
	}
	
	// Calculate the center of the polyline's bounding box
	int polylineCenterX = (minX + maxX) / 2;
	int polylineCenterY = (minY + maxY) / 2;
	
	// Place object origin so that the polyline center ends up at screen center
	int objectX = (SCREEN_WIDTH / 2) - polylineCenterX;
	int objectY = (SCREEN_HEIGHT / 2) - polylineCenterY;

	mapBoundary = App->physics->CreateChain(objectX, objectY,
		scaledPoints.data(),
		(int)scaledPoints.size(),
		b2_staticBody);

	if (!mapBoundary)
	{
		LOG("Failed to create map collision");
	}
}

// -----------------------------
// SaveAudioSettings / LoadAudioSettings
// -----------------------------
void ModuleGame::SaveAudioSettings()
{
	const char* filename = "audio_settings.dat";
	FILE* file = fopen(filename, "wb");
	if (!file)
	{
		LOG("ModuleGame::SaveAudioSettings() -> Failed to open %s for writing", filename);
		return;
	}

	float master = 1.0f;
	float music = 1.0f;
	float sfx = 1.0f;

	if (App && App->audio)
	{
		master = App->audio->GetMasterVolume();
		music = App->audio->GetMusicVolume();
		sfx = App->audio->GetSFXVolume();
	}

	fwrite(&master, sizeof(float), 1, file);
	fwrite(&music, sizeof(float), 1, file);
	fwrite(&sfx, sizeof(float), 1, file);

	fclose(file);
	LOG("ModuleGame::SaveAudioSettings() -> Saved audio settings (master=%.2f music=%.2f sfx=%.2f) to %s", master, music, sfx, filename);

	settingsSavedMessage = true;
	settingsSavedTimer = 0.0f;
}

void ModuleGame::LoadAudioSettings()
{
	const char* filename = "audio_settings.dat";
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		LOG("ModuleGame::LoadAudioSettings() -> No settings file (%s), using defaults", filename);
		return;
	}

	float master = 1.0f;
	float music = 1.0f;
	float sfx = 1.0f;

	size_t read = fread(&master, sizeof(float), 1, file);
	read += fread(&music, sizeof(float), 1, file);
	read += fread(&sfx, sizeof(float), 1, file);
	fclose(file);

	if (read < 3)
	{
		LOG("ModuleGame::LoadAudioSettings() -> Corrupt or incomplete file, ignoring");
		return;
	}

	LOG("ModuleGame::LoadAudioSettings() -> Loaded audio settings (master=%.2f music=%.2f sfx=%.2f) from %s", master, music, sfx, filename);

	if (App && App->audio)
	{
		App->audio->SetMasterVolume(master);
		App->audio->SetMusicVolume(music);
		App->audio->SetSFXVolume(sfx);
	}
}