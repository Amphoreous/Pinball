#pragma once

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

typedef struct {
    GameState currentState;
    GameState previousState;

    // Score tracking
    int currentScore;
    int previousScore;
    int highestScore;

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

} GameData;

// State transition functions (declarations)
void InitGameData(GameData* game);
void TransitionToState(GameData* game, GameState newState);
void UpdateGameState(GameData* game, float deltaTime);
void ResetRound(GameData* game);
void ResetGame(GameData* game);