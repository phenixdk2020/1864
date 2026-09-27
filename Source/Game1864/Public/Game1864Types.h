#pragma once

#include "CoreMinimal.h"
#include "Game1864Types.generated.h"

/** Infantry formations from the design manual: LINE / COLUMN / SQUARE. */
UENUM(BlueprintType)
enum class EInfantryFormation : uint8
{
	Line,
	Column,
	Square
};

/**
 * Regimental livery. Maps 1:1 to the soldier material's per-instance custom data
 * (see Docs/AssetSpec_Infantry.md, slots 0-11): coat, trousers, facings (collar/cuffs), piping.
 */
USTRUCT(BlueprintType)
struct FRegimentLivery
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Livery")
	FLinearColor Coat = FLinearColor(0.010f, 0.014f, 0.040f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Livery")
	FLinearColor Trousers = FLinearColor(0.10f, 0.16f, 0.30f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Livery")
	FLinearColor Facings = FLinearColor(0.45f, 0.03f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Livery")
	FLinearColor Piping = FLinearColor(0.45f, 0.03f, 0.03f);
};
