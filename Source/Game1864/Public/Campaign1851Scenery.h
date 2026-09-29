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
		Broadleaf,       // beech: a crown of several lobes
		Conifer,         // spruce in three tiers
		TownHouseTimber, // half-timbered (bindingsværk): white panels in dark timber, red tile roof
		MerchantHouse,   // three-storey red-brick merchant's house in the town core
		Windmill,        // Dutch windmill: octagonal body, cap and four sails
		Oak,             // broad, dark, lobed crown on a thick trunk
		Haystack,        // a stack of hay in the fields by a farm
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
		Flag,       // Dannebrog, hoist at the origin, flying towards +X
		Stables,    // low brick stable range, slate roof, a row of stall doors (12 x 4.4)
		Depot,      // tall red-brick magazine with tiled gables, hoist door and cart doors (7 x 5.6)
		Infirmary,  // two-storey brick infirmary with a slate roof and a porch (9 x 4.6)
		// Town buildings (a single building on its own plot):
		Arsenal,         // U-shaped brick arsenal around a yard open to +Y (18 x 13)
		Lazaret,         // two-storey yellow-plastered military hospital (12 x 5.2)
		Battery,         // coastal battery: earth rampart facing +Y with four guns (16 x 6)
		PowderMagazine,  // turf-covered brick vault inside a wall, lightning rod (10 x 9)
		StarFort,        // five-pointed earthwork star fort with a blockhouse (34 x 34)
		Telegraph,       // station house with a telegraph mast (9 x 4)
		Granary,         // tall yellow-brick grain store with hoist gables (13 x 5.4)
		// Railways:
		Train,           // locomotive, tender and three carriages; the front at the origin, running to +X (14.4 long)
		Station,         // small brick station with platform and canopy on the -Y side (8 x 4)
		// Regiments on the campaign map (board-game miniatures), facing +X:
		FormationInfantry,   // a battalion in column: dark blue coats, light blue trousers, Dannebrog
		FormationGuard,      // the Life Guard: red coats, bearskins
		FormationJager,      // light infantry in green
		FormationCavalry,    // dragoons on horseback
		FormationArtillery,      // guns with limbers and teams, gunners on foot
		FormationHorseArtillery, // guns with six-horse teams and mounted gunners
		// A train in parts, each centred on its origin and facing +X, so every vehicle follows the curve:
		TrainEngine,     // locomotive and tender (5.2 long)
		TrainCarBrown,   // first-class carriage (2.9 long)
		TrainCarGreen,   // second-class carriage (2.9 long)
		// Civil town buildings (Campaign1851Nations: they make towns grow):
		School,          // white village school with a bell turret (10 x 5)
		TownHall,        // yellow two-storey town hall with a clock tower (16 x 7)
		PostOffice,      // plastered post house with a coach shed (9 x 5)
		Hospital,        // three-storey white hospital with two wings (20 x 12)
		CustomsHouse,    // ochre customs house on a quay (10 x 6)
		Lighthouse,      // white tower with a red band and the keeper's house
		MerchantYard,    // merchant's house and warehouses round a yard (14 x 13)
		Brewery,         // brewery and distillery with a malt kiln (16 x 8)
		Brickworks,      // ring kiln, tall chimney and drying sheds (18 x 13)
		Sawmill,         // mill shed, log piles, boiler stack (12 x 12)
		Workshop,        // engineering works with north lights and a chimney (18 x 8)
		Factory,         // four-storey cloth mill with an engine house and chimney (26 x 14)
		Inn              // thatched country inn with the post horses' stable (16 x 10)
	};

	constexpr float BarracksLength = 16.f;
	constexpr float BarracksWidth = 5.5f;
	constexpr float BarracksEave = 7.5f;
	constexpr float BarracksTop = 12.3f;   // chimney tops

	UStaticMesh* BuildSitePiece(ESitePiece Piece, UMaterialInterface* Material);

	/** A staked-out plot: dug footprint around a Length x Width building and corner stakes. */
	UStaticMesh* BuildPlotGround(float Length, float Width, UMaterialInterface* Material);

	/** Timber scaffold (poles, boards, ledgers) around a Length x Width building, up to Top. */
	UStaticMesh* BuildScaffold(float Length, float Width, float Top, UMaterialInterface* Material);
}
