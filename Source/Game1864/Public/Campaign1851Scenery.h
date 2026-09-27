#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UStaticMesh;

/**
 * Low-poly scenery pieces for the close campaign zoom: town houses, farms, churches and trees.
 *
 * Built at runtime as vertex-coloured static meshes. Units match the map (1 km = 100 units, so
 * 1 unit = 10 m); pieces are roughly 3x real size so towns read from 10-60 km away. Shading from
 * the painted map's north-west light is baked into the vertex colours, each piece carries a soft
 * drop shadow to the south-east, and the material (M_Campaign1851Scenery) is unlit like the map.
 */
namespace Campaign1851Scenery
{
	enum class EPiece : uint8
	{
		TownHouse,       // whitewashed, red tile roof
		TownHouseOchre,  // yellow-washed, red tile roof
		Cottage,         // small thatched house
		Farm,            // four-winged farm ("firlænget gård") around a yard, thatched
		Church,          // white church, red roof, west tower with spire
		Broadleaf,       // beech/oak crown
		Conifer,         // spruce
		Count
	};

	UStaticMesh* Build(EPiece Piece, UMaterialInterface* Material);

	/**
	 * A flat ribbon along each polyline (points in map-local units, already on the terrain):
	 * darker edges, lighter crown, like a dirt road.
	 */
	UStaticMesh* BuildRibbons(const TArray<TArray<FVector>>& Lines, float HalfWidth, const FLinearColor& Colour, UMaterialInterface* Material, const TCHAR* Name);

	const TCHAR* Name(EPiece Piece);

	/** Building-site pieces (ACampaign1851ConstructionSite), in piece units; the barracks faces +Y. */
	enum class ESitePiece : uint8
	{
		Ground,     // parade ground, dug footprint and corner stakes
		Barracks,   // three-storey red-brick infantry barracks, slate roof, chimneys
		Scaffold,   // timber poles, ledgers and boards around the barracks
		CraneMast,  // timber derrick mast
		CraneJib,   // its jib and counterweight, turns about the mast
		Wagon,      // horse and cart loaded with bricks, facing +X
		Flagpole,
		Flag        // Dannebrog, hoist at the origin, flying towards +X
	};

	constexpr float BarracksLength = 16.f;
	constexpr float BarracksWidth = 5.5f;
	constexpr float BarracksEave = 7.5f;
	constexpr float BarracksTop = 12.3f;   // chimney tops

	UStaticMesh* BuildSitePiece(ESitePiece Piece, UMaterialInterface* Material);
}
