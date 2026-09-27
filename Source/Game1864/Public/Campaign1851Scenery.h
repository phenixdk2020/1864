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
}
