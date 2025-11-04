#pragma once

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

// Different point values for different targets
typedef enum {
    TARGET_BUMPER = 50,
    TARGET_COMBO_LETTER = 100,
    TARGET_COMBO_COMPLETE = 1000,
    TARGET_FLIPPER = 10,
    TARGET_WALL = 5,
    TARGET_SPECIAL = 200
} TargetPoints;

typedef struct {
    GameState currentState;
    GameState previousState;

    // Score tracking - IMPROVED SYSTEM
    int currentScore;
    int previousScore;
    int highestScore;

    // Score multipliers and bonuses
    int scoreMultiplier;
    int comboMultiplier;
    int consecutiveHits;

    // Ball management
    int ballsLeft;
    int totalBalls;
    int currentRound;

    // Combo system for POKEMON letters
    char comboLetters[8];  // "POKEMON"
    int comboProgress;
    bool comboComplete;

    // Game flow control
    bool isTransitioning;
    float transitionTimer;

    // Score persistence flag
    bool scoreNeedsSaving;

} GameData;

// State transition functions (declarations)
void InitGameData(GameData* game);
void TransitionToState(GameData* game, GameState newState);
void UpdateGameState(GameData* game, float deltaTime);
void ResetRound(GameData* game);
void ResetGame(GameData* game);

void AddScore(GameData* game, int points, const char* source);
void UpdateHighScore(GameData* game);
void ResetMultipliers(GameData* game);