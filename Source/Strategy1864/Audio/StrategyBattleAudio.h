#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StrategyBattleAudio.generated.h"
class USoundWave;
class USoundAttenuation;
class USoundConcurrency;
class UAudioComponent;

/** Kun præsentation. Faste køer og eget tilfældighedsforløb ændrer ikke kampens RNG. */
UCLASS()
class STRATEGY1864_API UStrategyBattleAudio : public UActorComponent
{
    GENERATED_BODY()
public:
    UStrategyBattleAudio();
    static UStrategyBattleAudio* Find(const UObject* Context);
    void Volley(const FVector& Position, int32 Shots, bool bRifle = false);
    void Artillery(const FVector& Position, int32 Guns, bool bMortar = false);
    void Clash(const FVector& Position);
    void Charge(AActor* Cavalry, bool bStart);
    float GetMasterVolume() const { return MasterVolume; }
    bool IsMuted() const { return bMuted; }
    void SetSettings(float Volume, bool bMute);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    enum { ClipCount = 17, VoiceBudget = 24, QueueBudget = 96, RegionBudget = 4 };
    struct FRequest
    {
        bool bUsed = false;
        int32 Clip = 0;
        FVector Position = FVector::ZeroVector;
        double Due = 0;
        float Gain = 1;
        float Pitch = 1;
        bool bHeavy = false;
        int32 Region = -1;
        TWeakObjectPtr<AActor> Follow;
    };
    struct FVoice
    {
        TWeakObjectPtr<UAudioComponent> Component;
        float Gain = 1;
        float Pitch = 1;
        int32 Region = -1;
        TWeakObjectPtr<AActor> Follow;
    };
    struct FRegion
    {
        bool bUsed = false;
        FVector Position = FVector::ZeroVector;
        double LastFire = 0;
    };
    UPROPERTY(Transient) TArray<TObjectPtr<USoundWave>> Clips;
    UPROPERTY(Transient) TObjectPtr<USoundAttenuation> MusketAttenuation;
    UPROPERTY(Transient) TObjectPtr<USoundAttenuation> CannonAttenuation;
    UPROPERTY(Transient) TObjectPtr<USoundConcurrency> Concurrency;
    FRequest Requests[QueueBudget];
    FVoice Voices[VoiceBudget];
    FRegion Regions[RegionBudget];
    FRandomStream AudioRandom;
    double LastTickReal = 0;
    float MasterVolume = 0.8f;
    bool bMuted = false;
    bool bDisabled = false;
    bool bDebug = false;
    float Speed() const;
    float Distance(const FVector& Position) const;
    void Queue(int32 Clip, const FVector& Position, float Gain, bool bHeavy, float Offset = 0,
               int32 Region = -1, AActor* Follow = nullptr);
    void FarFire(const FVector& Position, float Gain, bool bHeavy);
    void UpdateVoices();
};
