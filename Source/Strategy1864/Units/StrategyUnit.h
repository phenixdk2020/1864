#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "../UI/StrategySemanticZoomTypes.h"
#include "StrategyUnit.generated.h"

class USceneComponent;
class USphereComponent;
class UTextRenderComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStrategyOrderComponent;
class UStrategyCommandComponent;
class UStrategyMovementExecutorComponent;
class UStrategyFormationComponent;
class UStrategyFormationPolicyComponent;
class UStrategyFormationTransitionComponent;
class UStrategyParentExecutionComponent;
class UStrategyRoutePlannerComponent;
class UStrategyVisibilityComponent;
class UStrategyFireControlComponent;
class UStrategyCombatComponent;
class UStrategyOfficerAIComponent;
class UStrategyThreatReactionComponent;
class UStrategyOOBStatusComponent;
class UStrategySemanticZoomComponent;
class UStrategyWorldDebugComponent;
class UStrategyPresentationSnapshotComponent;
class UStrategyLocalDeconflictionComponent;
class UStrategyMissionAnchorComponent;
class UStrategyOfficerProfileComponent;
class UStrategyCommandDelayComponent;
class UStrategyConditionComponent;
class UStrategyContactComponent;
class UStrategyReconComponent;
class UStrategyAutonomousBattleAIComponent;
class UStrategyRoutRecoveryComponent;
class UStrategyFireDisciplineComponent;
class UStrategyStanceComponent;
class UStrategyDirectionalCoverComponent;
class UStrategyFieldworksComponent;
class UStrategySkirmisherComponent;
class UStrategySupplyComponent;
class UStrategyDoctrineComponent;
class UStrategyMissionConstraintsComponent;
class UStrategyAITelemetryComponent;
class UStrategyAutonomyComponent;
class UStrategyAIDifficultyComponent;
class UStrategyTerrainAwarenessComponent;
class UStrategyUniformAppearanceComponent;
class UStrategyHumanAnimationStateComponent;
class UStrategyEquipmentVisualComponent;
class UStrategyVisualCompatibilityComponent;
class UStrategyDetachmentComponent;
class UStrategyNCOComponent;
class UStrategyFireDrillComponent;
class UStrategyPositionOccupancyComponent;
class UStrategyFortificationAssaultComponent;
class UStrategyWorkingPartyComponent;
class UStrategySpecialistStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FStrategyCasualtyVisualEvent,
    int32,
    AppliedLoss,
    FVector,
    WorldLocation);

UENUM(BlueprintType)
enum class EStrategyEchelon : uint8
{
    Company,
    Battalion,
    Regiment,
    Brigade,
    Division,
    Cavalry,
    Artillery,
    Supply,
    Headquarters
};

UENUM(BlueprintType)
enum class EStrategySide : uint8
{
    Neutral,
    Denmark,
    Prussia,
    Austria,
    Allied,
    Enemy
};

UENUM(BlueprintType)
enum class EStrategyUnitState : uint8
{
    Ready,
    Moving,
    Reforming,
    Engaged,
    UnderFire,
    Routed,
    Disabled,
    Abandoned,
    Destroyed
};

UCLASS(Abstract, Blueprintable)
class STRATEGY1864_API AStrategyUnit : public APawn
{
    GENERATED_BODY()

public:
    AStrategyUnit();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<USphereComponent> SelectionCollider;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UTextRenderComponent> DebugLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|QA")
    TObjectPtr<UStaticMeshComponent> QAPlaceholderMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> QAPlaceholderMaterial;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyOrderComponent> OrderComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyCommandComponent> CommandComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyMovementExecutorComponent> MovementExecutor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFormationComponent> FormationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFormationPolicyComponent> FormationPolicy;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFormationTransitionComponent> FormationTransition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyParentExecutionComponent> ParentExecution;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyRoutePlannerComponent> RoutePlanner;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyVisibilityComponent> VisibilityComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFireControlComponent> FireControlComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyOfficerAIComponent> OfficerAIComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyThreatReactionComponent> ThreatReactionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyOOBStatusComponent> OOBStatusComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategySemanticZoomComponent> SemanticZoomComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyWorldDebugComponent> WorldDebugComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyPresentationSnapshotComponent> PresentationSnapshotComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyLocalDeconflictionComponent> LocalDeconflictionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyMissionAnchorComponent> MissionAnchorComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyOfficerProfileComponent> OfficerProfileComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyCommandDelayComponent> CommandDelayComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyConditionComponent> ConditionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyContactComponent> ContactComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyReconComponent> ReconComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyAutonomousBattleAIComponent> AutonomousBattleAIComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyRoutRecoveryComponent> RoutRecoveryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFireDisciplineComponent> FireDisciplineComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyStanceComponent> StanceComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyDirectionalCoverComponent> DirectionalCoverComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFieldworksComponent> FieldworksComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategySkirmisherComponent> SkirmisherComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategySupplyComponent> SupplyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyDoctrineComponent> DoctrineComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyMissionConstraintsComponent> MissionConstraintsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyAITelemetryComponent> AITelemetryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyAutonomyComponent> AutonomyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyAIDifficultyComponent> AIDifficultyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyTerrainAwarenessComponent> TerrainAwarenessComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyUniformAppearanceComponent> UniformAppearanceComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyHumanAnimationStateComponent> HumanAnimationStateComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyEquipmentVisualComponent> EquipmentVisualComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyVisualCompatibilityComponent> VisualCompatibilityComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyDetachmentComponent> DetachmentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyNCOComponent> NCOComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFireDrillComponent> FireDrillComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyPositionOccupancyComponent> PositionOccupancyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyFortificationAssaultComponent> FortificationAssaultComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategyWorkingPartyComponent> WorkingPartyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Components")
    TObjectPtr<UStrategySpecialistStateComponent> SpecialistStateComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Identity")
    FName StableUnitId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Unit")
    EStrategySide Side = EStrategySide::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Unit")
    EStrategyEchelon Echelon = EStrategyEchelon::Company;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Unit")
    int32 InitialStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Unit")
    int32 CurrentStrength = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float MaximumFireRangeCm = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float Morale = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float Cohesion = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float Fatigue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Combat")
    float Experience = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Unit")
    EStrategyUnitState UnitState = EStrategyUnitState::Ready;

    UPROPERTY(BlueprintAssignable, Category="Strategy|Presentation")
    FStrategyCasualtyVisualEvent OnCasualtyVisualEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|AI")
    bool bOfficerAIEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Strategy|Selection")
    bool bPlayerControllable = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Selection")
    bool bSelected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Strategy|Semantic Zoom")
    EStrategySemanticZoomState SemanticZoomState = EStrategySemanticZoomState::Close;

    UFUNCTION(BlueprintCallable, Category="Strategy|Selection")
    virtual void SetSelected(bool bNewSelected);

    UFUNCTION(BlueprintCallable, Category="Strategy|Unit")
    void SetUnitState(EStrategyUnitState NewState);

    UFUNCTION(BlueprintCallable, Category="Strategy|Combat")
    int32 ApplyStrengthLoss(int32 RequestedLoss);

    UFUNCTION(BlueprintCallable, Category="Strategy|Identity")
    void RefreshDebugLabel();

    UFUNCTION(BlueprintCallable, Category="Strategy|QA")
    void RefreshQAPlaceholderVisual();

    UFUNCTION(BlueprintCallable, Category="Strategy|Semantic Zoom")
    void SetSemanticZoomState(EStrategySemanticZoomState NewState);

    UFUNCTION(BlueprintPure, Category="Strategy|Presentation")
    FString GetNATOEchelonSymbol() const;

    UFUNCTION(BlueprintPure, Category="Strategy|Unit")
    bool IsCombatEffective() const;

    UFUNCTION(BlueprintImplementableEvent, Category="Strategy|Selection")
    void OnSelectionChanged(bool bNewSelected);

    UFUNCTION(BlueprintImplementableEvent, Category="Strategy|Unit")
    void OnUnitStateChanged(EStrategyUnitState NewState);

    UFUNCTION(BlueprintImplementableEvent, Category="Strategy|Semantic Zoom")
    void OnSemanticZoomChanged(EStrategySemanticZoomState NewState);
};
