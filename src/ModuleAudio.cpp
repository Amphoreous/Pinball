#include "Globals.h"
#include "Application.h"
#include "ModuleAudio.h"

#include "raylib.h"

#define MAX_FX_SOUNDS   64

ModuleAudio::ModuleAudio(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	fx_count = 0;
	music = Music{ 0 };
		
	flipperHitFx = 0;
	bumperHitFx = 0;
	ballLostFx = 0;
	bonusFx = 0;
	comboCompleteFx = 0;

	masterVolume = 1.0f;
	sfxVolume = 1.0f;
	musicVolume = 0.5f;
}

// Destructor
ModuleAudio::~ModuleAudio()
{}

// Called before render is available
bool ModuleAudio::Init()
{
	LOG("Loading Audio Mixer");
	bool ret = true;

    LOG("Loading raylib audio system");

    InitAudioDevice();

	// Load pinball SFX
	flipperHitFx = LoadFx("assets/audio/flipper_hit.wav");
	bumperHitFx = LoadFx("assets/audio/bumper_hit.wav");
	ballLostFx = LoadFx("assets/audio/ball_lost.wav");
	bonusFx = LoadFx("assets/audio/bonus.wav");
	comboCompleteFx = LoadFx("assets/audio/combo_complete.wav");

	PlayMusic("assets/audio/pinball_theme.wav");

	return ret;
}

update_status ModuleAudio::Update()
{
	if (IsMusicValid(music))
	{
		UpdateMusicStream(music);
	}

	return UPDATE_CONTINUE;
}

// Called before quitting
bool ModuleAudio::CleanUp()
{
	LOG("Freeing sound FX, closing Mixer and Audio subsystem");

    // Unload sounds
	for (unsigned int i = 0; i < fx_count; i++)
	{
		UnloadSound(fx[i]);
	}

    // Unload music
	if (IsMusicValid(music))
	{
		StopMusicStream(music);
		UnloadMusicStream(music);
	}

    CloseAudioDevice();

	return true;
}

// Play a music file
bool ModuleAudio::PlayMusic(const char* path, float fade_time)
{
	if(IsEnabled() == false)
		return false;

	bool ret = true;
	
	StopMusicStream(music);
	music = LoadMusicStream(path);
    
	if (IsMusicValid(music))
	{
		::SetMusicVolume(music, musicVolume * masterVolume);
		PlayMusicStream(music);
		LOG("Successfully playing %s", path);
	}
	else
	{
		LOG("Failed to load music: %s", path);
		ret = false;
	}

	LOG("Successfully playing %s", path);

	return ret;
}

// Load WAV
unsigned int ModuleAudio::LoadFx(const char* path)
{
	if(IsEnabled() == false)
		return 0;

	unsigned int ret = 0;

	Sound sound = LoadSound(path);

	if(sound.stream.buffer == NULL)
	{
		LOG("Cannot load sound: %s", path);
	}
	else
	{
        fx[fx_count++] = sound;
		ret = fx_count;
	}

	return ret;
}

// Play WAV
bool ModuleAudio::PlayFx(unsigned int id, int repeat)
{
	if (IsEnabled() == false)
	{
		return false;
	}

	bool ret = false;

	if (id > 0 && id <= fx_count)
	{
		SetSoundVolume(fx[id - 1], sfxVolume * masterVolume);
		PlaySound(fx[id - 1]);
		ret = true;
	}

	return ret;
}

void ModuleAudio::PlayFlipperHit(float impactForce)
{
	PlayFxWithVariation(flipperHitFx, impactForce); 
}

void ModuleAudio::PlayBumperHit(float impactForce)
{
	PlayFxWithVariation(bumperHitFx, impactForce);
}

void ModuleAudio::PlayBonusSound()
{
	PlayFx(bonusFx);
}

void ModuleAudio::PlayComboComplete()
{
	PlayFx(comboCompleteFx);
}

void ModuleAudio::PlayBallLost()
{
	PlayFx(ballLostFx);
}

void ModuleAudio::PlayFxWithPitch(unsigned int id, float pitch)
{
	if (IsEnabled() == false || id == 0 || id > fx_count)
		return;

	if (pitch < 0.1f) pitch = 0.1f;
	if (pitch > 2.0f) pitch = 2.0f;

	SetSoundPitch(fx[id - 1], pitch);
	PlayFx(id);
	SetSoundPitch(fx[id - 1], 1.0f);
}

void ModuleAudio::PlayFxWithVolume(unsigned int id, float volume)
{
	if (IsEnabled() == false || id == 0 || id > fx_count)
		return;

	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;

	SetSoundVolume(fx[id - 1], volume * sfxVolume * masterVolume);
	PlaySound(fx[id - 1]);
}

void ModuleAudio::PlayFxWithVariation(unsigned int id, float impactForce)
{
	if (IsEnabled() == false || id == 0 || id > fx_count)
		return;

	if (impactForce < 0.0f) impactForce = 0.0f;
	if (impactForce > 1.0f) impactForce = 1.0f;

	// Vary pitch: 0.8 to 1.2 based on impact
	float pitch = 0.8f + (impactForce * 0.4f);

	// Vary volume: 0.6 to 1.0 based on impact
	float volume = 0.6f + (impactForce * 0.4f);

	SetSoundPitch(fx[id - 1], pitch);
	SetSoundVolume(fx[id - 1], volume * sfxVolume * masterVolume);
	PlaySound(fx[id - 1]);

	SetSoundPitch(fx[id - 1], 1.0f);
}

void ModuleAudio::SetMasterVolume(float volume)
{
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;

	masterVolume = volume;
	::SetMasterVolume(masterVolume);

	if (IsMusicValid(music))
	{
		::SetMusicVolume(music, musicVolume * masterVolume);
	}
}

void ModuleAudio::SetSFXVolume(float volume)
{
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;

	sfxVolume = volume;
}

void ModuleAudio::SetMusicVolume(float volume)
{
	if (volume < 0.0f) volume = 0.0f;
	if (volume > 1.0f) volume = 1.0f;

	musicVolume = volume;

	if (IsMusicValid(music))
	{
		::SetMusicVolume(music, musicVolume * masterVolume);
	}
}