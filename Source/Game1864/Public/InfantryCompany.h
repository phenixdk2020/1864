#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game1864Types.h"
#include "InfantryCompany.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * One company-scale infantry unit rendered 1:1 (one simulated soldier = one visible figure).
 * Soldiers are instances, conformed to the terrain under each slot. Fallen soldiers stay on the
 * field as a second instance set.
 *
 * Until the authored soldier asset exists, the procedural crowd soldier (SoldierMeshBuilder) stands in.
 */
UCLASS()
class GAME1864_API AInfantryCompany : public AActor
{
	GENERATED_BODY()

public:
	AInfantryCompany();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company")
	FText DisplayName = FText::FromString(TEXT("1. Kompagni"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company")
	FText RegimentName = FText::FromString(TEXT("Sjællandske Livregiment"));

	/** Design manual baseline: full-strength company is 190 men. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company", meta = (ClampMin = "1", ClampMax = "400"))
	int32 FullStrength = 190;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Company", meta = (ClampMin = "0", ClampMax = "400"))
	int32 CurrentStrength = 190;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation")
	EInfantryFormation Formation = EInfantryFormation::Line;

	/** Line is three ranks deep. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation", meta = (ClampMin = "1", ClampMax = "6"))
	int32 Ranks = 3;

	/** 64 files x 75 cm = 48 m frontage, matching the manual's ~48 m for 190 men in three ranks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation", meta = (Units = "cm"))
	float FileSpacing = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation", meta = (Units = "cm"))
	float RankSpacing = 95.f;

	/** Small per-soldier position/yaw noise so the line reads as men, not a grid. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation", meta = (Units = "cm"))
	float PositionJitter = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	FRegimentLivery Livery;

	/** Authored soldier mesh (static/VAT). Empty = procedural crowd soldier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UStaticMesh> SoldierMesh;

	/** Material reading PerInstanceCustomData 0-11 as the livery. Empty = M_Soldier_Livery. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	TObjectPtr<UMaterialInterface> SoldierMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look")
	bool bShowColours = true;

	UFUNCTION(BlueprintCallable, Category = "Company")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Company")
	bool IsSelected() const { return bSelected; }

	/** Moves the last standing soldiers to the fallen set. */
	UFUNCTION(BlueprintCallable, Category = "Company")
	void ApplyCasualties(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void RebuildFormation();

	/** Formation footprint in local space (X = depth, Y = frontage), in cm. */
	UFUNCTION(BlueprintPure, Category = "Formation")
	FVector2D GetFootprintSize() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Soldiers;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Fallen;

	/** Yellow formation-sized selection footprint (manual F30I). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SelectionFootprint;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NationalColourPole;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NationalColour;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RegimentalColourPole;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RegimentalColour;

private:
	TArray<FTransform> ComputeSlots(int32 Count) const;
	FVector ProjectToGround(const FVector& WorldPoint) const;
	void ApplyLivery();
	UMaterialInterface* ResolveSoldierMaterial() const;
	void PlaceColours();
	void UpdateFootprint();

	bool bSelected = false;
	int32 FallenCount = 0;
};
