#include "StrategyBattleAudio.h"
#include "../Player/StrategyGameMode.h"
#include "../Player/StrategyPlayerController.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundWave.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

UStrategyBattleAudio::UStrategyBattleAudio()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    AudioRandom.Initialize(18641864);
}

UStrategyBattleAudio* UStrategyBattleAudio::Find(const UObject* Context)
{
    UWorld* AudioWorld = Context ? Context->GetWorld() : nullptr;
    AStrategyGameMode* AudioMode = AudioWorld ? Cast<AStrategyGameMode>(AudioWorld->GetAuthGameMode()) : nullptr;
    return AudioMode ? AudioMode->BattleAudio.Get() : nullptr;
}

void UStrategyBattleAudio::BeginPlay()
{
    Super::BeginPlay();
    LastTickReal = GetWorld()->GetRealTimeSeconds();
    bDisabled = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864NoSound"));
    bDebug = FParse::Param(FCommandLine::Get(), TEXT("Strategy1864DebugAudio"));
    if (GConfig)
    {
        GConfig->GetFloat(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleMasterVolume"), MasterVolume, GGameUserSettingsIni);
        GConfig->GetBool(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleMute"), bMuted, GGameUserSettingsIni);
    }
    MasterVolume = FMath::Clamp(MasterVolume, 0.f, 1.f);
    if (bDisabled) return;
    static const TCHAR* const AudioNames[] = {
        TEXT("musket_shot_01"), TEXT("musket_shot_02"), TEXT("musket_shot_03"),
        TEXT("musket_shot_04"), TEXT("musket_shot_05"), TEXT("musket_shot_06"),
        TEXT("volley_small"), TEXT("volley_large"), TEXT("cannon_01"), TEXT("cannon_02"),
        TEXT("cannon_03"), TEXT("cannon_04"), TEXT("mortar_thump"), TEXT("rifle_crack"),
        TEXT("bayonet_clash"), TEXT("distant_rumble"), TEXT("cavalry_hooves") };
    static_assert(UE_ARRAY_COUNT(AudioNames) == ClipCount, "Lydlisten skal følge indeksene");
    Clips.SetNum(ClipCount);
    // Bløde stier indlæses kun her; aldrig fra affyring eller tick.
    for (int32 AudioIndex = 0; AudioIndex < ClipCount; ++AudioIndex)
    {
        Clips[AudioIndex] = Cast<USoundWave>(FSoftObjectPath(FString::Printf(
            TEXT("/Game/Battle/Audio/%s.%s"), AudioNames[AudioIndex], AudioNames[AudioIndex])).TryLoad());
        if (!Clips[AudioIndex]) UE_LOG(LogTemp, Warning, TEXT("PROJECT1864-AUDIO mangler %s (kør importscript)"), AudioNames[AudioIndex]);
    }
    MusketAttenuation = NewObject<USoundAttenuation>(this);
    CannonAttenuation = NewObject<USoundAttenuation>(this);
    for (USoundAttenuation* AudioAttenuation : { MusketAttenuation.Get(), CannonAttenuation.Get() })
    {
        AudioAttenuation->Attenuation.bAttenuate = true;
        AudioAttenuation->Attenuation.bSpatialize = true;
        AudioAttenuation->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::Inverse;
        AudioAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        AudioAttenuation->Attenuation.AttenuationShapeExtents = FVector(15000.f, 0.f, 0.f);
    }
    MusketAttenuation->Attenuation.FalloffDistance = 235000.f;
    CannonAttenuation->Attenuation.FalloffDistance = 585000.f;
    Concurrency = NewObject<USoundConcurrency>(this);
    Concurrency->Concurrency.MaxCount = VoiceBudget;
    Concurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopQuietest;
    Concurrency->Concurrency.VolumeScaleMode = EConcurrencyVolumeScaleMode::Distance;
}

float UStrategyBattleAudio::Speed() const
{
    if (bDisabled || bMuted || MasterVolume <= 0.f || !GetWorld() || UGameplayStatics::IsGamePaused(this)) return 0.f;
    const AStrategyPlayerController* AudioPC = Cast<AStrategyPlayerController>(GetWorld()->GetFirstPlayerController());
    const float AudioSpeed = AudioPC ? AudioPC->SimulationSpeed : 1.f;
    return AudioSpeed >= 10.f ? 0.f : FMath::Clamp(AudioSpeed, 0.5f, 10.f);
}

float UStrategyBattleAudio::Distance(const FVector& Position) const
{
    APlayerController* AudioPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!AudioPC) return 0.f;
    FVector AudioCamera;
    FRotator AudioRotation;
    AudioPC->GetPlayerViewPoint(AudioCamera, AudioRotation);
    return FVector::Distance(AudioCamera, Position) / 100.f;
}

void UStrategyBattleAudio::SetSettings(float Volume, bool bMute)
{
    MasterVolume = FMath::Clamp(Volume, 0.f, 1.f);
    bMuted = bMute;
    if (GConfig)
    {
        GConfig->SetFloat(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleMasterVolume"), MasterVolume, GGameUserSettingsIni);
        GConfig->SetBool(TEXT("/Script/Strategy1864.Settings"), TEXT("BattleMute"), bMuted, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
    UpdateVoices();
}

void UStrategyBattleAudio::Queue(int32 Clip, const FVector& Position, float Gain, bool bHeavy, float Offset, int32 Region, AActor* Follow)
{
    if (Speed() <= 0.f || !Clips.IsValidIndex(Clip) || !Clips[Clip]) return;
    const float AudioDistance = Distance(Position);
    if (AudioDistance >= (bHeavy ? 6000.f : 2500.f)) return;
    for (FRequest& AudioRequest : Requests)
    {
        if (AudioRequest.bUsed) continue;
        AudioRequest.bUsed = true;
        AudioRequest.Clip = Clip;
        AudioRequest.Position = Position;
        AudioRequest.Gain = Gain;
        AudioRequest.Pitch = AudioRandom.FRandRange(0.93f, 1.07f);
        AudioRequest.bHeavy = bHeavy;
        AudioRequest.Region = Region;
        AudioRequest.Follow = Follow;
        AudioRequest.Due = GetWorld()->GetRealTimeSeconds() + Offset +
            (AudioDistance > 400.f ? FMath::Min(4.f, AudioDistance / 343.f) : 0.f);
        return;
    }
}

void UStrategyBattleAudio::FarFire(const FVector& Position, float Gain, bool bHeavy)
{
    if (Speed() <= 0.f || Distance(Position) >= (bHeavy ? 6000.f : 2500.f)) return;
    const FVector AudioCell(FMath::FloorToFloat(Position.X / 100000.f) * 100000.f + 50000.f,
                            FMath::FloorToFloat(Position.Y / 100000.f) * 100000.f + 50000.f, Position.Z);
    int32 AudioFree = -1;
    for (int32 AudioIndex = 0; AudioIndex < RegionBudget; ++AudioIndex)
    {
        if (!Regions[AudioIndex].bUsed) { AudioFree = AudioIndex; continue; }
        if (FVector::Dist2D(Regions[AudioIndex].Position, AudioCell) < 1.f)
        {
            Regions[AudioIndex].LastFire = GetWorld()->GetRealTimeSeconds();
            bool bAudioRegionPlaying = false;
            for (const FRequest& AudioRequest : Requests)
                if (AudioRequest.bUsed && AudioRequest.Region == AudioIndex) bAudioRegionPlaying = true;
            for (const FVoice& AudioVoice : Voices)
                if (AudioVoice.Region == AudioIndex && AudioVoice.Component.IsValid() && AudioVoice.Component->IsPlaying()) bAudioRegionPlaying = true;
            if (!bAudioRegionPlaying) Queue(15, Position, Gain * 0.45f, bHeavy, 0.f, AudioIndex);
            return;
        }
    }
    if (AudioFree < 0) return;
    Regions[AudioFree].bUsed = true;
    Regions[AudioFree].Position = AudioCell;
    Regions[AudioFree].LastFire = GetWorld()->GetRealTimeSeconds();
    Queue(15, Position, Gain * 0.45f, bHeavy, 0.f, AudioFree);
}

void UStrategyBattleAudio::Volley(const FVector& Position, int32 Shots, bool bRifle)
{
    if (Shots <= 0) return;
    const float AudioGain = FMath::Clamp(FMath::Sqrt(float(Shots) / 40.f), 0.2f, 1.5f);
    const float AudioDistance = Distance(Position);
    if (AudioDistance > 1400.f) FarFire(Position, AudioGain, false);
    if (AudioDistance >= 1500.f) return;
    const int32 AudioClip = bRifle && Shots < 8 ? 13 : (Shots < 8 ? AudioRandom.RandRange(0, 5) : Shots <= 40 ? 6 : 7);
    Queue(AudioClip, Position, AudioGain * FMath::Clamp((1500.f - AudioDistance) / 100.f, 0.f, 1.f), false);
}

void UStrategyBattleAudio::Artillery(const FVector& Position, int32 Guns, bool bMortar)
{
    const float AudioDistance = Distance(Position);
    if (AudioDistance > 1400.f) FarFire(Position, 1.f, true);
    if (AudioDistance >= 1500.f) return;
    for (int32 AudioGun = 0; AudioGun < FMath::Min(Guns, int32(VoiceBudget)); ++AudioGun)
        Queue(bMortar ? 12 : AudioRandom.RandRange(8, 11), Position,
              FMath::Clamp((1500.f - AudioDistance) / 100.f, 0.f, 1.f), true,
              AudioRandom.FRandRange(0.05f, 0.2f));
}

void UStrategyBattleAudio::Clash(const FVector& Position)
{
    if (Distance(Position) < 400.f) Queue(14, Position, 0.3f, false);
}

void UStrategyBattleAudio::Charge(AActor* Cavalry, bool bStart)
{
    if (!Cavalry) return;
    for (FRequest& AudioRequest : Requests)
        if (AudioRequest.Follow.Get() == Cavalry) AudioRequest.bUsed = false;
    for (FVoice& AudioVoice : Voices)
        if (AudioVoice.Follow.Get() == Cavalry)
        {
            if (UAudioComponent* AudioComponent = AudioVoice.Component.Get()) AudioComponent->Stop();
            AudioVoice = FVoice();
        }
    int32 AudioCharges = 0;
    for (const FRequest& AudioRequest : Requests)
        if (AudioRequest.bUsed && AudioRequest.Follow.IsValid()) ++AudioCharges;
    for (const FVoice& AudioVoice : Voices)
        if (AudioVoice.Follow.IsValid() && AudioVoice.Component.IsValid()) ++AudioCharges;
    if (bStart && AudioCharges < 4 && Distance(Cavalry->GetActorLocation()) < 800.f)
        Queue(16, Cavalry->GetActorLocation(), 0.35f, false, 0.f, -1, Cavalry);
}

void UStrategyBattleAudio::UpdateVoices()
{
    const float AudioSpeed = Speed();
    for (FVoice& AudioVoice : Voices)
    {
        UAudioComponent* AudioComponent = AudioVoice.Component.Get();
        if (!AudioComponent) continue;
        if (AudioVoice.Follow.IsStale() ||
            (AudioVoice.Region >= 0 && Distance(AudioComponent->GetComponentLocation()) < 1400.f))
        {
            AudioComponent->Stop();
            AudioVoice = FVoice();
            continue;
        }
        const float AudioRegionFade = AudioVoice.Region >= 0
            ? FMath::Clamp((Distance(AudioComponent->GetComponentLocation()) - 1400.f) / 100.f, 0.f, 1.f) : 1.f;
        AudioComponent->SetVolumeMultiplier(AudioSpeed > 0.f ? MasterVolume * AudioVoice.Gain * AudioRegionFade : 0.f);
        AudioComponent->SetPaused(UGameplayStatics::IsGamePaused(this));
        if (AudioSpeed > 0.f) AudioComponent->SetPitchMultiplier(AudioVoice.Pitch * AudioSpeed);
        if (AudioVoice.Follow.IsValid()) AudioComponent->SetWorldLocation(AudioVoice.Follow->GetActorLocation());
    }
}

void UStrategyBattleAudio::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (bDisabled) return;
    UpdateVoices();
    const double AudioNow = GetWorld()->GetRealTimeSeconds();
    const double AudioRealDelta = FMath::Max(0.0, AudioNow - LastTickReal);
    LastTickReal = AudioNow;
    const bool bAudioPaused = UGameplayStatics::IsGamePaused(this);
    for (int32 AudioIndex = 0; AudioIndex < RegionBudget; ++AudioIndex)
    {
        if (!Regions[AudioIndex].bUsed) continue;
        // Pausen stopper regionernes inaktivitetstid.
        if (bAudioPaused) { Regions[AudioIndex].LastFire += AudioRealDelta; continue; }
        if (AudioNow - Regions[AudioIndex].LastFire < 8.0) continue;
        for (FVoice& AudioVoice : Voices)
            if (AudioVoice.Region == AudioIndex)
            {
                if (UAudioComponent* AudioComponent = AudioVoice.Component.Get()) AudioComponent->Stop();
                AudioVoice = FVoice();
            }
        for (FRequest& AudioRequest : Requests)
            if (AudioRequest.Region == AudioIndex) AudioRequest.bUsed = false;
        Regions[AudioIndex] = FRegion();
    }
    for (FRequest& AudioRequest : Requests)
    {
        if (!AudioRequest.bUsed) continue;
        if (bAudioPaused) { AudioRequest.Due += AudioRealDelta; continue; }
        if (Speed() <= 0.f) { AudioRequest.bUsed = false; continue; }
        if (AudioRequest.Due > AudioNow) continue;
        AudioRequest.bUsed = false;
        if (AudioRequest.Follow.IsStale()) continue;
        if (Distance(AudioRequest.Position) >= (AudioRequest.bHeavy ? 6000.f : 2500.f)) continue;
        if (AudioRequest.Clip != 15 && AudioRequest.Clip != 16 && AudioRequest.Clip != 14 && Distance(AudioRequest.Position) >= 1500.f)
        {
            FarFire(AudioRequest.Position, AudioRequest.Gain, AudioRequest.bHeavy);
            continue;
        }
        if (AudioRequest.Region >= 0 && Distance(AudioRequest.Position) < 1400.f) continue;
        int32 AudioSlot = -1;
        float AudioQuietest = TNumericLimits<float>::Max();
        for (int32 AudioIndex = 0; AudioIndex < VoiceBudget; ++AudioIndex)
        {
            UAudioComponent* AudioComponent = Voices[AudioIndex].Component.Get();
            if (!AudioComponent || !AudioComponent->IsPlaying()) { AudioSlot = AudioIndex; break; }
            const float AudioLevel = Voices[AudioIndex].Gain / FMath::Max(150.f, Distance(AudioComponent->GetComponentLocation()));
            if (AudioLevel < AudioQuietest) { AudioQuietest = AudioLevel; AudioSlot = AudioIndex; }
        }
        if (AudioSlot < 0) continue;
        UAudioComponent* AudioExisting = Voices[AudioSlot].Component.Get();
        const float AudioIncomingLevel = AudioRequest.Gain / FMath::Max(150.f, Distance(AudioRequest.Position));
        if (AudioExisting && AudioExisting->IsPlaying() && AudioIncomingLevel < AudioQuietest) continue;
        if (UAudioComponent* AudioOld = Voices[AudioSlot].Component.Get()) AudioOld->Stop();
        const float AudioSpawnFade = AudioRequest.Region >= 0
            ? FMath::Clamp((Distance(AudioRequest.Position) - 1400.f) / 100.f, 0.f, 1.f) : 1.f;
        UAudioComponent* AudioSpawn = UGameplayStatics::SpawnSoundAtLocation(this, Clips[AudioRequest.Clip], AudioRequest.Position,
            FRotator::ZeroRotator, MasterVolume * AudioRequest.Gain * AudioSpawnFade, AudioRequest.Pitch * Speed(), 0.f,
            AudioRequest.bHeavy ? CannonAttenuation.Get() : MusketAttenuation.Get(), Concurrency);
        Voices[AudioSlot] = FVoice();
        Voices[AudioSlot].Component = AudioSpawn;
        Voices[AudioSlot].Gain = AudioRequest.Gain;
        Voices[AudioSlot].Pitch = AudioRequest.Pitch;
        Voices[AudioSlot].Region = AudioRequest.Region;
        Voices[AudioSlot].Follow = AudioRequest.Follow;
        if (bDebug) UE_LOG(LogTemp, Log, TEXT("PROJECT1864-AUDIO clip=%s distance=%.1fm volume=%.3f"),
            *Clips[AudioRequest.Clip]->GetName(), Distance(AudioRequest.Position), MasterVolume * AudioRequest.Gain);
    }
}

void UStrategyBattleAudio::EndPlay(const EEndPlayReason::Type Reason)
{
    for (FVoice& AudioVoice : Voices)
        if (UAudioComponent* AudioComponent = AudioVoice.Component.Get()) AudioComponent->Stop();
    Super::EndPlay(Reason);
}
