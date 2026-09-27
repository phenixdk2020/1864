#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UMaterialInterface;

/**
 * Builds the low-poly crowd soldier (Danish line infantry, 1864) procedurally.
 *
 * Colour is carried in the vertex colour so one mesh serves every regiment:
 *   RGB   = fixed colour (skin, leather, belts, brass, steel, wood), sRGB-encoded by the mesh build
 *   Alpha = livery slot: 0 fixed, 0.25 coat, 0.5 trousers, 0.75 facings, 1.0 piping
 * The soldier material resolves slots 1-4 from PerInstanceCustomData (see Docs/AssetSpec_Infantry.md).
 *
 * Space: centimetres, origin between the feet, +X forward, +Z up.
 */
namespace Game1864::SoldierMesh
{
	/** Returns the shared infantry mesh, building it on first use. */
	GAME1864_API UStaticMesh* GetInfantry(UMaterialInterface* Material);

	/** Height from the ground to the top of the shako, in cm. */
	constexpr float StandingHeight = 200.f;
}
