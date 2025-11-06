#pragma once

#include <cstring>

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_YOU_WIN
} GameState;

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

    int currentScore;
    int previousScore;
    int highestScore;

    int scoreMultiplier;
    int comboMultiplier;
    int consecutiveHits;

    int ballsLeft;
    int totalBalls;
    int currentRound;

    char comboLetters[5];
    int comboProgress;
    bool comboComplete;

    bool isTransitioning;
    float transitionTimer;

    bool scoreNeedsSaving;

} GameData;

void InitGameData(GameData* game);
void TransitionToState(GameData* game, GameState newState);
void UpdateGameState(GameData* game, float deltaTime);
void ResetRound(GameData* game);
void ResetGame(GameData* game);
void AddScore(GameData* game, int points, const char* source);
void UpdateHighScore(GameData* game);
void ResetMultipliers(GameData* game);