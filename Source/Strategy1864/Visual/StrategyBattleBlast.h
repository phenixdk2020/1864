#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyBattleBlast.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/** What a blast shows: the gun's discharge, the strike of a round shot, a bursting shell, ... */
UENUM()
enum class EStrategyBlastKind : uint8
{
    CannonMuzzle,   // flash, a bank of white powder smoke pushed forward, dust thrown up before the muzzle
    MortarMuzzle,   // the same, thrown upwards
    GroundImpact,   // a round shot strikes: a fountain of earth and a dust cloud, a scar in the grass
    ShellBurst,     // a shell bursts on the ground: fire, black smoke, more earth, a crater
    AirBurst,       // a shrapnel shell bursts above the men: a white puff, the balls kick up the dust below
    Canister,       // case shot: a cone of dust kicked up in front of the gun
    HoofDust        // a galloping horse: a small puff of dust
};

/**
 * A blast of the battle, built from engine content only (spheres, a disc, a point light): no particle assets.
 * Each piece grows, drifts, falls and fades by itself; the actor removes itself when the last piece is gone.
 */
UCLASS(NotBlueprintable)
class STRATEGY1864_API AStrategyBattleBlast : public AActor
{
    GENERATED_BODY()

public:
    AStrategyBattleBlast();

    /** Spawns a blast at Location; Direction is the line of fire (for the smoke and the canister cone). */
    static AStrategyBattleBlast* Spawn(UWorld* World, EStrategyBlastKind Kind, const FVector& Location, const FVector& Direction, float Scale = 1.0f, float RangeCm = 0.0f);

    virtual void Tick(float DeltaTime) override;

private:
    struct FPiece
    {
        TObjectPtr<UStaticMeshComponent> Mesh;
        TObjectPtr<UMaterialInstanceDynamic> Material;
        FVector Position = FVector::ZeroVector;
        FVector Velocity = FVector::ZeroVector;
        float Delay = 0.0f;
        float Life = 1.0f;
        float StartDiameter = 50.0f;
        float EndDiameter = 100.0f;
        float Opacity = 1.0f;
        FLinearColor Colour = FLinearColor::White;
        float Drag = 0.0f;          // 1/s
        bool bGravity = false;
        bool bFlat = false;         // a scar on the ground: flat, does not grow after the first moment
        bool bGlow = false;         // fire: the colour above 1 (bloom)
    };

    void Build(EStrategyBlastKind Kind, const FVector& Direction, float Scale, float RangeCm);
    void AddPiece(const FPiece& Piece);
    FPiece Smoke(const FVector& Offset, const FVector& Velocity, float D0, float D1, float Life, float Opacity, const FLinearColor& Colour) const;
    void AddEarth(int32 Count, float UpMin, float UpMax, float Side, float Scale);
    void AddFlash(float Intensity, float RadiusCm, float Seconds);
    float GroundZAt(const FVector& At) const;

    TArray<FPiece> Pieces;
    UPROPERTY() TObjectPtr<USceneComponent> Root;
    UPROPERTY() TObjectPtr<UPointLightComponent> Flash;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Meshes;
    float FlashSeconds = 0.0f;
    float FlashIntensity = 0.0f;
    float Age = 0.0f;
};

/**
 * Where the shot will fall: the gun knows the casualties when it fires, the men should fall when the ball arrives,
 * next to where it strikes. The infantry visual asks for a strike near it and waits for it.
 */
struct STRATEGY1864_API FStrategyImpactRegistry
{
    struct FImpact
    {
        TWeakObjectPtr<UWorld> World;
        FVector Location = FVector::ZeroVector;
        float LandTime = 0.0f;
        float RadiusCm = 600.0f;
        /** Case shot: the men fall in the cone from the gun (Origin) through Location. */
        bool bCone = false;
        FVector Origin = FVector::ZeroVector;
    };
    static void Add(UWorld* World, const FVector& Location, float LandTime, float RadiusCm);
    static void AddCone(UWorld* World, const FVector& Origin, const FVector& Location, float LandTime);
    /** The strike nearest to Centre (within ReachCm) landing between Now - 0.5 s and Now + 6 s; false if none. */
    static bool Find(UWorld* World, const FVector& Centre, float ReachCm, float Now, FImpact& Out);

private:
    static TArray<FImpact>& Impacts();
};
