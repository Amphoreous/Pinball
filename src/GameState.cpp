#include "GameState.h"
#include <cstring>

void InitGameData(GameData* game)
{
    game->currentState = STATE_MENU;
    game->previousState = STATE_MENU;

    game->currentScore = 0;
    game->previousScore = 0;
    game->highestScore = 0;

    game->scoreMultiplier = 1;
    game->comboMultiplier = 1;
    game->consecutiveHits = 0;

    game->ballsLeft = 3;
    game->totalBalls = 3;
    game->currentRound = 1;

    strcpy_s(game->comboLetters, 5, "STAR");
    game->comboProgress = 0;
    game->comboComplete = false;

    game->isTransitioning = false;
    game->transitionTimer = 0.0f;

    game->scoreNeedsSaving = false;
}

void TransitionToState(GameData* game, GameState newState)
{
    game->previousState = game->currentState;
    game->currentState = newState;
    game->isTransitioning = true;
    game->transitionTimer = 0.0f;
}

void UpdateGameState(GameData* game, float deltaTime)
{
    if (game->isTransitioning)
    {
        game->transitionTimer += deltaTime;
        if (game->transitionTimer >= 0.5f)
        {
            game->isTransitioning = false;
        }
    }
}

void ResetRound(GameData* game)
{
    game->currentScore = 0;
    game->scoreMultiplier = 1;
    game->comboMultiplier = 1;
    game->consecutiveHits = 0;
    game->comboProgress = 0;
    game->comboComplete = false;
}

void ResetGame(GameData* game)
{
    game->currentScore = 0;
    game->previousScore = 0;
    game->scoreMultiplier = 1;
    game->comboMultiplier = 1;
    game->consecutiveHits = 0;
    game->ballsLeft = 3;
    game->totalBalls = 3;
    game->currentRound = 1;
    game->comboProgress = 0;
    game->comboComplete = false;
}

void AddScore(GameData* game, int points, const char* source)
{
    int multipliedPoints = points * game->scoreMultiplier * game->comboMultiplier;
    game->currentScore += multipliedPoints;

    game->consecutiveHits++;
    if (game->consecutiveHits >= 5)
    {
        game->comboMultiplier = 2;
    }
    if (game->consecutiveHits >= 10)
    {
        game->comboMultiplier = 3;
    }

    UpdateHighScore(game);
}

void UpdateHighScore(GameData* game)
{
    if (game->currentScore > game->highestScore)
    {
        game->highestScore = game->currentScore;
        game->scoreNeedsSaving = true;
    }
}

void ResetMultipliers(GameData* game)
{
    game->scoreMultiplier = 1;
    game->comboMultiplier = 1;
    game->consecutiveHits = 0;
}