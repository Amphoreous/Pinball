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
    COLLISION_COMBO_LETTER,
    COLLISION_SPECIAL_TARGET
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

    // SCORING SYSTEM FUNCTIONS
    void AddScore(int points, const char* source);
    void SaveHighScore();
    void LoadHighScore();
    void UpdateScoreDisplay();
    void ResetScoreMultipliers();

    // TMX Map loading
    bool LoadTMXMap(const char* filepath);
    void CreateMapCollision();

public:
    // Game state system
    GameData gameData = {}; // initialized to avoid warnings

    // Physics bodies
    PhysBody* ball = nullptr;
    PhysBody* leftFlipper = nullptr;
    PhysBody* rightFlipper = nullptr;
    PhysBody* kicker = nullptr;

    // Flipper joints (may be nullptr)
    b2RevoluteJoint* leftFlipperJoint = nullptr;
    b2RevoluteJoint* rightFlipperJoint = nullptr;

    // Bumpers / other bodies
    std::vector<PhysBody*> bumpers;
    std::vector<PhysBody*> targets;
    std::vector<PhysBody*> specialTargets;

    // Textures (initialized to zero to avoid uninitialized warnings)
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

    // UI textures (start menu / pause)
    Texture2D titleTexture = { 0 };

    // Fonts / UI
    Font font = { 0 };
    Font titleFont = { 0 };

    // Debug & UI toggles
    bool showDebug = false;
    bool showAudioSettings = false;
    bool settingsSavedMessage = false;
    float settingsSavedTimer = 0.0f;

    // Ball launch control
    bool ballLaunched = false;
    float kickerForce = 0.0f;
    float kickerChargeTime = 0.0f;
    // Increase launcher power and charge rate
    const float MAX_KICKER_FORCE = 45.0f;      // was 20.0f
    const float KICKER_CHARGE_SPEED = 30.0f;   // was 10.0f

    // Sound effects (loaded once, played multiple times)
    int bumperHitSfx = -1;
    int launchSfx = -1;
    int targetHitSfx = -1;
    int specialHitSfx = -1;

    // TMX Map data & collision boundary
    std::vector<int> mapCollisionPoints;
    PhysBody* mapBoundary = nullptr;

    // Score display animation
    float scoreFlashTimer = 0.0f;
    bool scoreFlashActive = false;
    int lastScoreIncrease = 0;
};