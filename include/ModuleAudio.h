#pragma once

#include "Module.h"
#include "raylib.h"

#define MAX_SOUNDS	16
#define DEFAULT_MUSIC_FADE_TIME 2.0f

class ModuleAudio : public Module
{
public:

	ModuleAudio(Application* app, bool start_enabled = true);
	~ModuleAudio();

	bool Init();
	bool CleanUp();
	update_status Update();

	// Play a music file
	bool PlayMusic(const char* path, float fade_time = DEFAULT_MUSIC_FADE_TIME);

	// Load a sound in memory
	unsigned int LoadFx(const char* path);

	// Play a previously loaded sound
	bool PlayFx(unsigned int fx, int repeat = 0);

	void PlayFlipperHit(float impactForce = 0.5f);
	void PlayBumperHit(float impactForce = 0.5f);
	void PlayBonusSound();
	void PlayComboComplete();
	void PlayBallLost();

	// Sound variation
	void PlayFxWithPitch(unsigned int fx, float pitch);
	void PlayFxWithVolume(unsigned int fx, float volume);
	void PlayFxWithVariation(unsigned int fx, float impactForce);

	// Volume controls
	void SetMasterVolume(float volume);
	void SetSFXVolume(float volume);
	void SetMusicVolume(float volume);

	float GetMasterVolume() const { return masterVolume; }
	float GetSFXVolume() const { return sfxVolume; }
	float GetMusicVolume() const { return musicVolume; }
private:

	Music music;
	Sound fx[MAX_SOUNDS];
    unsigned int fx_count;



	unsigned int flipperHitFx;
	unsigned int bumperHitFx;
	unsigned int ballLostFx;
	unsigned int bonusFx;
	unsigned int comboCompleteFx;


	// Volume settings
	float masterVolume;
	float sfxVolume;
	float musicVolume;

};
