#include "GameState.h"
#include "Globals.h"
#include <string.h>

void InitGameData(GameData* game)
{
	game->currentState = STATE_MENU;
	game->previousState = STATE_MENU;

	game->currentScore = 0;
	game->previousScore = 0;
	game->highestScore = 0;

	game->ballsLeft = 3;
	game->totalBalls = 3;
	game->currentRound = 1;

	// Initialize combo system
	strcpy_s(game->comboLetters, sizeof(game->comboLetters), "POKEMON");
	game->comboProgress = 0;
	game->comboComplete = false;

	game->isTransitioning = false;
	game->transitionTimer = 0.0f;

	LOG("Game data initialized - State: MENU, Balls: %d", game->ballsLeft);
}

void TransitionToState(GameData* game, GameState newState)
{
	if (game->currentState == newState)
		return;

	LOG("State transition: %d -> %d", game->currentState, newState);

	game->previousState = game->currentState;
	game->currentState = newState;
	game->isTransitioning = true;
	game->transitionTimer = 0.0f;

	switch (newState)
	{
	case STATE_MENU:
		LOG("Entered MENU state");
		break;

	case STATE_PLAYING:
		LOG("Entered PLAYING state - Round %d, Balls: %d",
			game->currentRound, game->ballsLeft);
		break;

	case STATE_PAUSED:
		LOG("Game PAUSED");
		break;

	case STATE_GAME_OVER:
		LOG("GAME OVER - Final Score: %d", game->currentScore);
		game->previousScore = game->currentScore;
		break;
	}
}

void UpdateGameState(GameData* game, float deltaTime)
{
	if (game->isTransitioning)
	{
		game->transitionTimer += deltaTime;

		if (game->transitionTimer >= 0.5f)
		{
			game->isTransitioning = false;
			game->transitionTimer = 0.0f;
		}
	}
}

void ResetRound(GameData* game)
{
	LOG("Resetting round - Balls left: %d", game->ballsLeft);

	game->ballsLeft--;

	if (game->ballsLeft <= 0)
	{
		TransitionToState(game, STATE_GAME_OVER);
	}
	else
	{
		game->comboProgress = 0;
		game->comboComplete = false;
	}
}

// Start new game from menu
void ResetGame(GameData* game)
{
	LOG("Resetting complete game");

	if (game->currentScore > game->highestScore)
	{
		game->highestScore = game->currentScore;
		LOG("New high score: %d", game->highestScore);
	}

	game->previousScore = game->currentScore;

	game->currentScore = 0;
	game->ballsLeft = game->totalBalls;
	game->currentRound = 1;

	game->comboProgress = 0;
	game->comboComplete = false;

	game->isTransitioning = false;
	game->transitionTimer = 0.0f;
}