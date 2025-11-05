#pragma once

#include "Globals.h"
#include "Module.h"
#include "GameState.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>
#include <cstring>

class PhysBody;
class PhysicEntity;
class b2RevoluteJoint;

enum CollisionType
{
    COLLISION_WALL,
    COLLISION_FLIPPER,
    COLLISION_BUMPER,
    COLLISION_TARGET,
    COLLISION_COMBO_LETTER,
    COLLISION_SPECIAL_TARGET,
    COLLISION_BALL_LOSS_SENSOR,
    COLLISION_BLACK_HOLE
};

class ModuleGame : public Module
{
public:
    ModuleGame(Application* app, bool start_enabled = true);
    ~ModuleGame();

    bool Start();
    update_status Update();
    bool CleanUp();
    void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

    float CalculateImpactForce(PhysBody* body);
    CollisionType IdentifyCollision(PhysBody* bodyA, PhysBody* bodyB);

    void DrawAudioSettings();
    void UpdateAudioSettings();
    void SaveAudioSettings();
    void LoadAudioSettings();

    void RenderMenuState();
    void RenderPlayingState();
    void RenderPausedState();
    void RenderGameOverState();

    void UpdateMenuState();
    void UpdatePlayingState();
    void UpdatePausedState();
    void UpdateGameOverState();

    void LaunchBall();
    void LoseBall();
    void RespawnBall();
    void AddComboLetter(char letter);

    void AddScore(int points, const char* source);
    void SaveHighScore();
    void LoadHighScore();
    void UpdateScoreDisplay();
    void ResetScoreMultipliers();

    bool LoadTMXMap(const char* filepath);
    void CreateMapCollision();
    void CreateBallLossSensor();

    void PauseGame();
    void ResumeGame();

    void ApplyBlackHoleForces(float dt);

public:
    GameData gameData = {};

    PhysBody* ball = nullptr;
    PhysBody* leftFlipper = nullptr;
    PhysBody* rightFlipper = nullptr;
    PhysBody* kicker = nullptr;
    PhysBody* ballLossSensor = nullptr;

    b2RevoluteJoint* leftFlipperJoint = nullptr;
    b2RevoluteJoint* rightFlipperJoint = nullptr;

    std::vector<PhysBody*> bumpers;
    std::vector<PhysBody*> targets;
    std::vector<PhysBody*> specialTargets;
    std::vector<PhysBody*> blackHoles;

    Texture2D ballTexture = { 0 };
    Texture2D backgroundTexture = { 0 };
    Texture2D flipperTexture = { 0 };
    Texture2D flipperBaseTexture = { 0 };
    Texture2D bumper1Texture = { 0 };
    Texture2D bumper2Texture = { 0 };
    Texture2D bumper3Texture = { 0 };
    Texture2D piece1Texture = { 0 };
    Texture2D piece2Texture = { 0 };
    Texture2D targetTexture = { 0 };
    Texture2D specialTargetTexture = { 0 };
    Texture2D blackHoleTexture = { 0 };

    Texture2D titleTexture = { 0 };

    Font font = { 0 };
    Font titleFont = { 0 };

    bool showDebug = false;
    bool showAudioSettings = false;
    bool settingsSavedMessage = false;
    float settingsSavedTimer = 0.0f;

    bool ballLaunched = false;
    float kickerForce = 0.0f;
    float kickerChargeTime = 0.0f;
    const float MAX_KICKER_FORCE = 45.0f;
    const float KICKER_CHARGE_SPEED = 30.0f;

    int bumperHitSfx = -1;
    int launchSfx = -1;
    int targetHitSfx = -1;
    int specialHitSfx = -1;
    int ballLostSfx = -1;

    std::vector<int> mapCollisionPoints;
    std::vector<Rectangle> tmxBlackHoles;
    std::vector<Rectangle> tmxBumpers;
    PhysBody* mapBoundary = nullptr;

    float scoreFlashTimer = 0.0f;
    bool scoreFlashActive = false;
    int lastScoreIncrease = 0;

    float ballLossTimer = 0.0f;

    float ballSavedPosX, ballSavedPosY;
    float ballSavedVelX, ballSavedVelY;
    float ballSavedAngularVel;
    bool ballSavedAwake;

    bool isGamePaused = false;
};