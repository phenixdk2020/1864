#include "Campaign1851Scenery.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"

namespace
{
	FLinearColor Srgb(uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); }

	const FLinearColor Plaster = Srgb(226, 216, 192);
	const FLinearColor Ochre = Srgb(214, 172, 98);
	const FLinearColor Tile = Srgb(168, 70, 46);
	const FLinearColor Thatch = Srgb(128, 104, 72);
	const FLinearColor Slate = Srgb(66, 70, 78);
	const FLinearColor Beech = Srgb(62, 90, 38);
	const FLinearColor Spruce = Srgb(36, 58, 36);
	const FLinearColor Bark = Srgb(72, 54, 38);
	const FLinearColor Shadow = Srgb(30, 38, 20);

	/** Towards the light: the painted map's hillshade is lit from the north-west (north = -Y). */
	const FVector3f ToLight = FVector3f(-0.55f, -0.55f, 0.63f).GetSafeNormal();

	/** Flat-shaded triangle soup with baked lighting in the vertex colours. */
	struct FWriter
	{
		FMeshDescription Mesh;
		FStaticMeshAttributes Attributes{ Mesh };
		FPolygonGroupID Group;

		/** Darken faces near the ground (pieces standing on the map; not the road ribbons). */
		bool bGroundShade = false;

		FWriter()
		{
			Attributes.Register();
			Attributes.GetVertexInstanceUVs().SetNumChannels(1);
			Group = Mesh.CreatePolygonGroup();
		}

		/** Adds a triangle facing away from Inside (a point inside the solid it belongs to). */
		void Tri(FVector3f A, FVector3f B, FVector3f C, const FLinearColor& Base, const FVector3f& Inside, bool bLit = true)
		{
			// Unreal's front face normal is (P2 - P0) x (P1 - P0).
			FVector3f N = FVector3f::CrossProduct(C - A, B - A).GetSafeNormal();
			if (FVector3f::DotProduct(N, (A + B + C) / 3.f - Inside) < 0.f)
			{
				Swap(B, C);
				N = -N;
			}
			const float Light = bLit ? 0.62f + 0.5f * FMath::Max(0.f, FVector3f::DotProduct(N, ToLight)) : 1.f;
			const FLinearColor Lit = Base * Light;
			const bool bShade = bLit && bGroundShade;

			TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
			TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
			TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
			TVertexInstanceAttributesRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
			TVertexInstanceAttributesRef<FVector4f> Colours = Attributes.GetVertexInstanceColors();
			FVertexInstanceID Corners[3];
			const FVector3f P[3] = { A, B, C };
			for (int32 i = 0; i < 3; ++i)
			{
				const FVertexID V = Mesh.CreateVertex();
				Positions[V] = P[i];
				Corners[i] = Mesh.CreateVertexInstance(V);
				Normals[Corners[i]] = N;
				Tangents[Corners[i]] = FVector3f::ForwardVector;
				Signs[Corners[i]] = 1.f;
				// Baked contact shading: walls and trunks darken over the last couple of units above the ground.
				const float Shade = bShade ? 0.7f + 0.3f * FMath::Clamp(P[i].Z / 2.4f, 0.f, 1.f) : 1.f;
				Colours[Corners[i]] = FVector4f(Lit.R * Shade, Lit.G * Shade, Lit.B * Shade, 1.f);
			}
			Mesh.CreateTriangle(Group, Corners);
		}

		void Quad(const FVector3f& A, const FVector3f& B, const FVector3f& C, const FVector3f& D, const FLinearColor& Base, const FVector3f& Inside)
		{
			Tri(A, B, C, Base, Inside);
			Tri(A, C, D, Base, Inside);
		}

		/** Axis-aligned box without its bottom face. */
		void Box(const FVector3f& Min, const FVector3f& Max, const FLinearColor& Base)
		{
			const FVector3f In = (Min + Max) * 0.5f;
			auto P = [&](int32 X, int32 Y, int32 Z) { return FVector3f(X ? Max.X : Min.X, Y ? Max.Y : Min.Y, Z ? Max.Z : Min.Z); };
			Quad(P(0, 0, 1), P(1, 0, 1), P(1, 1, 1), P(0, 1, 1), Base, In);
			Quad(P(0, 0, 0), P(1, 0, 0), P(1, 0, 1), P(0, 0, 1), Base, In);
			Quad(P(0, 1, 0), P(1, 1, 0), P(1, 1, 1), P(0, 1, 1), Base, In);
			Quad(P(0, 0, 0), P(0, 1, 0), P(0, 1, 1), P(0, 0, 1), Base, In);
			Quad(P(1, 0, 0), P(1, 1, 0), P(1, 1, 1), P(1, 0, 1), Base, In);
		}

		/**
		 * A house: walls plus a gabled roof with the ridge along its length.
		 * bAlongY turns the house 90 degrees around its centre.
		 */
		void House(const FVector2f& Centre, float Length, float Width, float Eave, float Ridge, const FLinearColor& Walls, const FLinearColor& Roof, bool bAlongY = false)
		{
			auto At = [&](float L, float W, float Z) { return bAlongY ? FVector3f(Centre.X + W, Centre.Y + L, Z) : FVector3f(Centre.X + L, Centre.Y + W, Z); };
			const float HL = Length * 0.5f, HW = Width * 0.5f, O = 0.35f;  // roof overhang
			const FVector3f In = At(0.f, 0.f, Eave * 0.5f);
			// Walls.
			Quad(At(-HL, -HW, 0.f), At(HL, -HW, 0.f), At(HL, -HW, Eave), At(-HL, -HW, Eave), Walls, In);
			Quad(At(-HL, HW, 0.f), At(HL, HW, 0.f), At(HL, HW, Eave), At(-HL, HW, Eave), Walls, In);
			Quad(At(-HL, -HW, 0.f), At(-HL, HW, 0.f), At(-HL, HW, Eave), At(-HL, -HW, Eave), Walls, In);
			Quad(At(HL, -HW, 0.f), At(HL, HW, 0.f), At(HL, HW, Eave), At(HL, -HW, Eave), Walls, In);
			// Gable ends.
			const FVector3f RoofIn = At(0.f, 0.f, Eave);
			Tri(At(-HL, -HW, Eave), At(-HL, HW, Eave), At(-HL, 0.f, Ridge), Walls, RoofIn);
			Tri(At(HL, -HW, Eave), At(HL, HW, Eave), At(HL, 0.f, Ridge), Walls, RoofIn);
			// Roof slopes, with a little overhang.
			const float Drop = (Ridge - Eave) * O / HW;
			Quad(At(-HL - O, -HW - O, Eave - Drop), At(HL + O, -HW - O, Eave - Drop), At(HL + O, 0.f, Ridge), At(-HL - O, 0.f, Ridge), Roof, RoofIn);
			Quad(At(-HL - O, HW + O, Eave - Drop), At(HL + O, HW + O, Eave - Drop), At(HL + O, 0.f, Ridge), At(-HL - O, 0.f, Ridge), Roof, RoofIn);
		}

		/**
		 * Windows in rows (one per storey) on both long walls of a House, and a door in the middle of the +Y wall.
		 * Quads sit a hair outside the wall so they draw over it.
		 */
		void Facade(const FVector2f& Centre, float Length, float Width, float Eave, int32 Storeys, const FLinearColor& Window, const FLinearColor& DoorColour)
		{
			const float HL = Length * 0.5f, HW = Width * 0.5f + 0.03f;
			const float Storey = Eave / Storeys;
			const int32 Bays = FMath::Max(2, FMath::FloorToInt((Length - 1.f) / 1.4f));
			const FVector3f In(Centre.X, Centre.Y, Eave * 0.5f);
			for (int32 Row = 0; Row < Storeys; ++Row)
			{
				const float Z0 = Row * Storey + Storey * 0.35f, Z1 = Z0 + Storey * 0.42f;
				for (int32 b = 0; b < Bays; ++b)
				{
					const float X = Centre.X - HL + (b + 0.5f) * Length / Bays;
					if (Row == 0 && b == Bays / 2)
					{
						continue;   // the door's place
					}
					for (float Side : { -1.f, 1.f })
					{
						const float Y = Centre.Y + Side * HW;
						Quad(FVector3f(X - 0.28f, Y, Z0), FVector3f(X + 0.28f, Y, Z0), FVector3f(X + 0.28f, Y, Z1), FVector3f(X - 0.28f, Y, Z1), Window, In);
					}
				}
			}
			const float DX = Centre.X - HL + (Bays / 2 + 0.5f) * Length / Bays, Y = Centre.Y + HW;
			Quad(FVector3f(DX - 0.38f, Y, 0.f), FVector3f(DX + 0.38f, Y, 0.f), FVector3f(DX + 0.38f, Y, FMath::Min(1.7f, Storey * 0.8f)), FVector3f(DX - 0.38f, Y, FMath::Min(1.7f, Storey * 0.8f)), DoorColour, In);
		}

		/** Half-timbering on the long walls: posts, a rail at mid-height and the wall plate, dark over the plaster. */
		void TimberFrame(const FVector2f& Centre, float Length, float Width, float Eave, const FLinearColor& Wood)
		{
			const float HL = Length * 0.5f, HW = Width * 0.5f;
			for (float Side : { -1.f, 1.f })
			{
				const float Y0 = Centre.Y + Side * HW, Y1 = Y0 + Side * 0.05f;
				const float YMin = FMath::Min(Y0, Y1), YMax = FMath::Max(Y0, Y1);
				for (float X = -HL; X <= HL + 0.01f; X += Length / FMath::Max(3, FMath::RoundToInt(Length / 1.1f)))
				{
					Box(FVector3f(Centre.X + X - 0.07f, YMin, 0.3f), FVector3f(Centre.X + X + 0.07f, YMax, Eave), Wood);
				}
				for (float Z : { 0.3f, Eave * 0.5f, Eave - 0.12f })
				{
					Box(FVector3f(Centre.X - HL, YMin, Z), FVector3f(Centre.X + HL, YMax, Z + 0.12f), Wood);
				}
			}
		}

		/** Square pyramid (tower spire). */
		void Spire(const FVector2f& Centre, float Half, float Z0, float Z1, const FLinearColor& Base)
		{
			const FVector3f In(Centre.X, Centre.Y, Z0);
			const FVector3f Apex(Centre.X, Centre.Y, Z1);
			const FVector3f C[4] = {
				{ Centre.X - Half, Centre.Y - Half, Z0 }, { Centre.X + Half, Centre.Y - Half, Z0 },
				{ Centre.X + Half, Centre.Y + Half, Z0 }, { Centre.X - Half, Centre.Y + Half, Z0 } };
			for (int32 i = 0; i < 4; ++i)
			{
				Tri(C[i], C[(i + 1) % 4], Apex, Base, In);
			}
		}

		/** A faceted solid of revolution through the given (radius, height) rings; radius 0 = a point. */
		void Lathe(const TArray<FVector2f>& Profile, int32 Sides, const FLinearColor& Base, float Twist = 0.f, const FVector3f& Offset = FVector3f::ZeroVector)
		{
			const FVector3f In = Offset + FVector3f(0.f, 0.f, (Profile[0].Y + Profile.Last().Y) * 0.5f);
			auto P = [&](int32 Ring, int32 Side)
			{
				const float A = (Side + Twist * Ring) * UE_TWO_PI / Sides;
				return Offset + FVector3f(Profile[Ring].X * FMath::Cos(A), Profile[Ring].X * FMath::Sin(A), Profile[Ring].Y);
			};
			for (int32 r = 0; r + 1 < Profile.Num(); ++r)
			{
				for (int32 s = 0; s < Sides; ++s)
				{
					if (Profile[r].X > 0.f)
					{
						Tri(P(r, s), P(r, s + 1), P(r + 1, s), Base, In);
					}
					if (Profile[r + 1].X > 0.f)
					{
						Tri(P(r, s + 1), P(r + 1, s + 1), P(r + 1, s), Base, In);
					}
				}
			}
		}

		/** Flat drop shadow (an ellipse pushed south-east, away from the north-west light). */
		void DropShadow(float RadiusX, float RadiusY, float Offset)
		{
			const int32 Sides = 10;
			const FVector3f Centre(Offset, Offset, 0.12f);
			const FVector3f Below = Centre - FVector3f(0.f, 0.f, 1.f);
			for (int32 s = 0; s < Sides; ++s)
			{
				const float A0 = s * UE_TWO_PI / Sides, A1 = (s + 1) * UE_TWO_PI / Sides;
				Tri(Centre, Centre + FVector3f(RadiusX * FMath::Cos(A0), RadiusY * FMath::Sin(A0), 0.f),
					Centre + FVector3f(RadiusX * FMath::Cos(A1), RadiusY * FMath::Sin(A1), 0.f), Shadow, Below, false);
			}
		}
	};

void BuildPiece(FWriter& W, Campaign1851Scenery::EPiece Piece)
	{
		using Campaign1851Scenery::EPiece;
		const FLinearColor Timber = Srgb(64, 46, 34), Brick = Srgb(164, 76, 54), Glass = Srgb(44, 52, 62), Door = Srgb(86, 56, 36);
		const FLinearColor Tar = Srgb(40, 38, 36), Stone = Srgb(122, 116, 106), Hay = Srgb(200, 168, 96);
		W.bGroundShade = true;
		switch (Piece)
		{
		case EPiece::TownHouse:
		case EPiece::TownHouseOchre:
			W.DropShadow(4.6f, 3.4f, 1.6f);
			W.Box(FVector3f(-3.6f, -2.3f, 0.f), FVector3f(3.6f, 2.3f, 0.35f), Stone);
			W.House(FVector2f(0.f, 0.f), 7.f, 4.4f, 3.4f, 6.2f, Piece == EPiece::TownHouse ? Plaster : Ochre, Tile);
			W.Facade(FVector2f(0.f, 0.f), 7.f, 4.4f, 3.4f, 2, Glass, Door);
			W.Box(FVector3f(1.6f, -0.3f, 5.2f), FVector3f(2.2f, 0.3f, 7.f), Brick);
			break;
		case EPiece::TownHouseTimber:
			W.DropShadow(4.4f, 3.2f, 1.5f);
			W.Box(FVector3f(-3.35f, -2.15f, 0.f), FVector3f(3.35f, 2.15f, 0.35f), Stone);
			W.House(FVector2f(0.f, 0.f), 6.5f, 4.f, 3.2f, 5.8f, Plaster, Tile);
			W.TimberFrame(FVector2f(0.f, 0.f), 6.5f, 4.f, 3.2f, Timber);
			W.Facade(FVector2f(0.f, 0.f), 6.5f, 4.f, 3.2f, 2, Glass, Door);
			W.Box(FVector3f(-2.f, -0.28f, 4.8f), FVector3f(-1.45f, 0.28f, 6.5f), Brick);
			break;
		case EPiece::MerchantHouse:
			W.DropShadow(5.4f, 4.f, 2.f);
			W.Box(FVector3f(-4.3f, -2.7f, 0.f), FVector3f(4.3f, 2.7f, 0.4f), Stone);
			W.House(FVector2f(0.f, 0.f), 8.4f, 5.2f, 5.2f, 8.6f, Brick, Tile);
			W.Facade(FVector2f(0.f, 0.f), 8.4f, 5.2f, 5.2f, 3, Srgb(236, 230, 214), Door);   // white window frames on brick
			W.Box(FVector3f(-0.9f, -3.1f, 5.f), FVector3f(0.9f, -2.3f, 7.4f), Brick);            // dormer (kvist) towards the street
			W.Box(FVector3f(-1.1f, -3.2f, 7.4f), FVector3f(1.1f, -2.2f, 7.65f), Tile);
			W.Box(FVector3f(2.4f, -0.3f, 7.6f), FVector3f(3.f, 0.3f, 9.4f), Brick);
			W.Box(FVector3f(-3.f, -0.3f, 7.6f), FVector3f(-2.4f, 0.3f, 9.4f), Brick);
			break;
		case EPiece::Cottage:
			W.DropShadow(3.6f, 2.6f, 1.2f);
			W.Box(FVector3f(-2.8f, -1.75f, 0.f), FVector3f(2.8f, 1.75f, 0.4f), Tar);   // tarred plinth (sokkel)
			W.House(FVector2f(0.f, 0.f), 5.5f, 3.4f, 2.2f, 4.8f, Plaster, Thatch);
			W.Facade(FVector2f(0.f, 0.f), 5.5f, 3.4f, 2.2f, 1, Glass, Door);
			W.Box(FVector3f(0.6f, -0.25f, 3.9f), FVector3f(1.1f, 0.25f, 5.3f), Plaster);
			break;
		case EPiece::Farm:
			W.DropShadow(7.f, 7.f, 1.8f);
			// Four wings round the yard: the dwelling (stuehus) whitewashed with windows, barns and stables with doors.
			W.Box(FVector3f(-5.05f, -5.95f, 0.f), FVector3f(5.05f, -2.45f, 0.4f), Tar);
			W.House(FVector2f(0.f, -4.2f), 10.f, 3.4f, 2.4f, 5.f, Plaster, Thatch);
			W.Facade(FVector2f(0.f, -4.2f), 10.f, 3.4f, 2.4f, 1, Glass, Door);
			W.House(FVector2f(0.f, 4.2f), 10.f, 3.4f, 2.4f, 5.f, Srgb(150, 120, 92), Thatch);
			W.Box(FVector3f(-0.9f, 2.45f, 0.f), FVector3f(0.9f, 2.5f, 2.1f), Door);               // barn gate to the yard
			W.House(FVector2f(-4.8f, 0.f), 5.f, 3.2f, 2.4f, 4.8f, Plaster, Thatch, true);
			W.House(FVector2f(4.8f, 0.f), 5.f, 3.2f, 2.4f, 4.8f, Srgb(150, 120, 92), Thatch, true);
			W.Box(FVector3f(1.2f, -4.45f, 4.1f), FVector3f(1.7f, -3.95f, 5.5f), Plaster);
			break;
		case EPiece::Church:
			W.DropShadow(10.f, 5.f, 3.f);
			W.House(FVector2f(1.5f, 0.f), 12.f, 5.4f, 4.8f, 8.6f, Plaster, Tile);
			W.Box(FVector3f(-7.9f, -2.2f, 0.f), FVector3f(-3.5f, 2.2f, 12.5f), Plaster);
			// Stepped gables (kamtakker) on the tower, and tall narrow windows along the nave.
			for (float Y : { -2.25f, 2.25f })
			{
				for (int32 Step = 0; Step < 3; ++Step)
				{
					const float Half = 2.2f - Step * 0.7f;
					W.Box(FVector3f(-5.7f - Half, Y - 0.12f, 12.5f + Step * 0.8f), FVector3f(-5.7f + Half, Y + 0.12f, 13.3f + Step * 0.8f), Plaster);
				}
			}
			for (int32 w = 0; w < 4; ++w)
			{
				const float X = -2.f + w * 2.6f;
				for (float Side : { -1.f, 1.f })
				{
					const float Y = Side * 2.74f;
					W.Quad(FVector3f(X - 0.35f, Y, 1.4f), FVector3f(X + 0.35f, Y, 1.4f), FVector3f(X + 0.35f, Y, 3.6f), FVector3f(X - 0.35f, Y, 3.6f), Glass, FVector3f(X, 0.f, 2.5f));
				}
			}
			W.Box(FVector3f(-6.4f, -2.25f, 0.f), FVector3f(-5.f, -2.2f, 2.6f), Door);
			W.Spire(FVector2f(-5.7f, 0.f), 2.5f, 14.9f, 21.f, Slate);
			break;
		case EPiece::Windmill:
		{
			W.DropShadow(4.f, 3.f, 2.f);
			W.Lathe({ { 3.f, 0.f }, { 2.8f, 2.4f }, { 1.9f, 8.6f }, { 0.f, 8.6f } }, 8, Srgb(206, 196, 176));    // plastered octagon
			W.Lathe({ { 3.6f, 2.4f }, { 3.6f, 2.7f }, { 0.f, 2.7f } }, 8, Srgb(110, 84, 58));                     // the gallery (omgang)
			W.Lathe({ { 2.1f, 8.6f }, { 1.6f, 9.8f }, { 0.f, 10.6f } }, 8, Srgb(70, 60, 54));                     // the cap
			const FLinearColor Stock = Srgb(88, 66, 46), Sail = Srgb(226, 214, 186);
			const float HubX = 2.3f, HubZ = 9.2f, Arm = 6.4f;
			W.Box(FVector3f(HubX - 0.2f, -0.25f, HubZ - 0.25f), FVector3f(HubX + 0.3f, 0.25f, HubZ + 0.25f), Stock);
			W.Box(FVector3f(HubX, -0.1f, HubZ - Arm), FVector3f(HubX + 0.15f, 0.1f, HubZ + Arm), Stock);    // the + of the stocks
			W.Box(FVector3f(HubX, -Arm, HubZ - 0.1f), FVector3f(HubX + 0.15f, Arm, HubZ + 0.1f), Stock);
			// Sails on the trailing side of each stock.
			W.Box(FVector3f(HubX + 0.05f, 0.1f, HubZ + 1.2f), FVector3f(HubX + 0.1f, 1.2f, HubZ + Arm), Sail);
			W.Box(FVector3f(HubX + 0.05f, -1.2f, HubZ - Arm), FVector3f(HubX + 0.1f, -0.1f, HubZ - 1.2f), Sail);
			W.Box(FVector3f(HubX + 0.05f, 1.2f, HubZ - 1.2f), FVector3f(HubX + 0.1f, Arm, HubZ - 0.1f), Sail);
			W.Box(FVector3f(HubX + 0.05f, -Arm, HubZ + 0.1f), FVector3f(HubX + 0.1f, -1.2f, HubZ + 1.2f), Sail);
			W.Box(FVector3f(2.95f, -0.5f, 0.f), FVector3f(3.05f, 0.5f, 1.6f), Door);
			break;
		}
		case EPiece::Broadleaf:
			// Beech: a trunk, and a crown of three lobes in slightly different greens.
			W.DropShadow(4.8f, 3.8f, 2.4f);
			W.Box(FVector3f(-0.45f, -0.45f, 0.f), FVector3f(0.45f, 0.45f, 3.5f), Bark);
			W.Lathe({ { 0.f, 2.4f }, { 3.4f, 3.6f }, { 4.1f, 6.2f }, { 2.9f, 9.2f }, { 0.f, 10.4f } }, 8, Beech, 0.5f);
			W.Lathe({ { 0.f, 4.8f }, { 2.2f, 5.6f }, { 2.4f, 7.4f }, { 0.f, 9.2f } }, 7, Beech * 1.12f, 0.3f, FVector3f(1.9f, 1.2f, 1.4f));
			W.Lathe({ { 0.f, 3.6f }, { 2.f, 4.4f }, { 2.2f, 6.f }, { 0.f, 7.6f } }, 7, Beech * 0.86f, 0.3f, FVector3f(-1.7f, -1.3f, 0.4f));
			break;
		case EPiece::Oak:
			// Oak: thick trunk, broad and flat, dark and knotty.
			W.DropShadow(5.8f, 4.6f, 2.4f);
			W.Box(FVector3f(-0.65f, -0.65f, 0.f), FVector3f(0.65f, 0.65f, 3.f), Bark);
			W.Lathe({ { 0.f, 2.6f }, { 4.2f, 3.6f }, { 4.8f, 5.6f }, { 3.2f, 7.6f }, { 0.f, 8.4f } }, 8, Srgb(56, 80, 34), 0.5f);
			W.Lathe({ { 0.f, 3.4f }, { 2.6f, 4.2f }, { 2.8f, 6.f }, { 0.f, 7.4f } }, 7, Srgb(64, 92, 38), 0.2f, FVector3f(2.8f, 0.8f, 0.6f));
			W.Lathe({ { 0.f, 3.f }, { 2.4f, 3.8f }, { 2.5f, 5.4f }, { 0.f, 6.8f } }, 7, Srgb(48, 70, 30), 0.2f, FVector3f(-2.4f, -1.6f, 0.2f));
			W.Lathe({ { 0.f, 4.f }, { 2.2f, 4.8f }, { 2.3f, 6.2f }, { 0.f, 7.6f } }, 7, Srgb(60, 86, 36), 0.2f, FVector3f(-0.8f, 2.6f, 0.8f));
			break;
		case EPiece::Conifer:
			// Spruce in three tiers.
			W.DropShadow(3.2f, 2.6f, 2.6f);
			W.Box(FVector3f(-0.35f, -0.35f, 0.f), FVector3f(0.35f, 0.35f, 2.f), Bark);
			W.Lathe({ { 3.2f, 1.6f }, { 0.f, 7.f } }, 7, Spruce);
			W.Lathe({ { 2.5f, 5.f }, { 0.f, 10.f } }, 7, Spruce * 1.1f, 0.5f);
			W.Lathe({ { 1.7f, 8.6f }, { 0.f, 14.f } }, 7, Spruce * 1.2f);
			break;
		case EPiece::Haystack:
			W.DropShadow(1.8f, 1.6f, 0.8f);
			W.Lathe({ { 1.3f, 0.f }, { 1.45f, 1.f }, { 1.1f, 2.f }, { 0.f, 2.7f } }, 8, Hay, 0.5f);
			break;
		default:
			break;
		}
	}
}

namespace Campaign1851Scenery
{
	const TCHAR* Name(EPiece Piece)
	{
		static const TCHAR* Names[] = { TEXT("TownHouse"), TEXT("TownHouseOchre"), TEXT("Cottage"), TEXT("Farm"), TEXT("Church"), TEXT("Broadleaf"), TEXT("Conifer"),
			TEXT("TownHouseTimber"), TEXT("MerchantHouse"), TEXT("Windmill"), TEXT("Oak"), TEXT("Haystack") };
		return Names[FMath::Clamp(int32(Piece), 0, int32(EPiece::Count) - 1)];
	}

	static UStaticMesh* Finish(FWriter& Writer, UMaterialInterface* Material, const FString& MeshName)
	{
		// Unique names: several sites build the same pieces, and reusing a name would re-create a mesh
		// that is already on screen (the renderer's ray tracing geometry asserts on that).
		UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UStaticMesh::StaticClass(), FName(*MeshName)), RF_Transient);
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, TEXT("Scenery")));
		Mesh->bSupportRayTracing = false;   // the map is unlit: ray tracing geometry would only cost memory
		UStaticMesh::FBuildMeshDescriptionsParams Params;
		Params.bFastBuild = true;
		Params.bMarkPackageDirty = false;
		Params.bCommitMeshDescription = false;
		Mesh->BuildFromMeshDescriptions({ &Writer.Mesh }, Params);
		return Mesh;
	}

	UStaticMesh* Build(EPiece Piece, UMaterialInterface* Material)
	{
		FWriter Writer;
		BuildPiece(Writer, Piece);
		return Finish(Writer, Material, FString::Printf(TEXT("SM_Campaign1851_%s"), Name(Piece)));
	}

	UStaticMesh* BuildRibbons(const TArray<TArray<FVector>>& Lines, float HalfWidth, const FLinearColor& Colour, UMaterialInterface* Material, const TCHAR* MeshName)
	{
		FWriter Writer;
		const FLinearColor Edge = Colour * 0.74f;
		for (const TArray<FVector>& Line : Lines)
		{
			if (Line.Num() < 2)
			{
				continue;
			}
			// Cross-section: dark verge, lighter crown, dark verge (three strips, one colour each).
			TArray<FVector3f> L, LI, RI, R;
			for (int32 i = 0; i < Line.Num(); ++i)
			{
				// Direction from the neighbours; the side vector is horizontal.
				const FVector Dir = (Line[FMath::Min(i + 1, Line.Num() - 1)] - Line[FMath::Max(i - 1, 0)]).GetSafeNormal2D();
				const FVector Side = FVector(-Dir.Y, Dir.X, 0.0) * HalfWidth;
				const FVector Crown = Line[i] + FVector(0.0, 0.0, 0.2);
				L.Add(FVector3f(Line[i] - Side));
				LI.Add(FVector3f(Crown - Side * 0.55));
				RI.Add(FVector3f(Crown + Side * 0.55));
				R.Add(FVector3f(Line[i] + Side));
			}
			for (int32 i = 0; i + 1 < Line.Num(); ++i)
			{
				const FVector3f Below = LI[i] - FVector3f(0.f, 0.f, 50.f);
				Writer.Quad(L[i], LI[i], LI[i + 1], L[i + 1], Edge, Below);
				Writer.Quad(LI[i], RI[i], RI[i + 1], LI[i + 1], Colour, Below);
				Writer.Quad(RI[i], R[i], R[i + 1], RI[i + 1], Edge, Below);
			}
		}
		return Finish(Writer, Material, FString(MeshName));
	}
}

namespace Campaign1851Scenery
{
	UStaticMesh* BuildSitePiece(ESitePiece Piece, UMaterialInterface* Material)
	{
		const FLinearColor Brick = Srgb(172, 82, 56);
		const FLinearColor Stone = Srgb(158, 152, 140);
		const FLinearColor RoofSlate = Srgb(74, 78, 88);
		const FLinearColor Window = Srgb(38, 44, 54);
		const FLinearColor Sand = Srgb(198, 178, 132);
		const FLinearColor Dug = Srgb(128, 100, 70);
		const FLinearColor Timber = Srgb(156, 116, 72);
		const FLinearColor DarkTimber = Srgb(96, 70, 46);
		const float HL = BarracksLength * 0.5f, HW = BarracksWidth * 0.5f;

		FWriter W;
		switch (Piece)
		{
		case ESitePiece::Ground:
		{
			const FVector3f Below(0.f, 0.f, -5.f);
			auto Flat = [&](float X0, float Y0, float X1, float Y1, float Z, const FLinearColor& C)
			{
				W.Quad(FVector3f(X0, Y0, Z), FVector3f(X1, Y0, Z), FVector3f(X1, Y1, Z), FVector3f(X0, Y1, Z), C, Below);
			};
			Flat(-HL - 1.2f, -HW - 1.2f, HL + 1.2f, HW + 1.2f, 0.12f, Dug);       // dug footprint
			Flat(-HL - 1.5f, HW + 1.2f, HL + 1.5f, 13.8f, 0.14f, Sand);           // parade ground
			for (float X : { -HL - 1.f, 0.f, HL + 1.f })                         // corner and middle stakes
			{
				for (float Y : { -HW - 1.f, HW + 1.f })
				{
					W.Box(FVector3f(X - 0.15f, Y - 0.15f, 0.f), FVector3f(X + 0.15f, Y + 0.15f, 1.3f), DarkTimber);
				}
			}
			break;
		}
		case ESitePiece::Barracks:
		{
			W.House(FVector2f(0.f, 0.f), BarracksLength, BarracksWidth, BarracksEave, 11.f, Brick, RoofSlate);
			W.Box(FVector3f(-HL - 0.12f, -HW - 0.12f, 0.f), FVector3f(HL + 0.12f, HW + 0.12f, 0.6f), Stone);
			// Windows: three storeys on both long facades, two per storey on the gable ends.
			const FVector3f In(0.f, 0.f, 3.f);
			for (int32 Storey = 0; Storey < 3; ++Storey)
			{
				const float Z0 = 1.3f + Storey * 2.1f, Z1 = Z0 + 1.15f;
				for (int32 c = 0; c < 11; ++c)
				{
					const float X = -7.f + c * 1.4f;
					if (Storey == 0 && c == 5)
					{
						continue;  // the door
					}
					for (float Side : { -1.f, 1.f })
					{
						const float Y = Side * (HW + 0.03f);
						W.Quad(FVector3f(X - 0.3f, Y, Z0), FVector3f(X + 0.3f, Y, Z0), FVector3f(X + 0.3f, Y, Z1), FVector3f(X - 0.3f, Y, Z1), Window, In);
					}
				}
				for (float End : { -1.f, 1.f })
				{
					const float X = End * (HL + 0.03f);
					for (float Y : { -1.1f, 1.1f })
					{
						W.Quad(FVector3f(X, Y - 0.3f, Z0), FVector3f(X, Y + 0.3f, Z0), FVector3f(X, Y + 0.3f, Z1), FVector3f(X, Y - 0.3f, Z1), Window, In);
					}
				}
			}
			// Door with a stone surround, and the chimneys along the ridge.
			W.Box(FVector3f(-0.9f, HW, 0.f), FVector3f(0.9f, HW + 0.35f, 3.1f), Stone);
			W.Quad(FVector3f(-0.45f, HW + 0.37f, 0.6f), FVector3f(0.45f, HW + 0.37f, 0.6f), FVector3f(0.45f, HW + 0.37f, 2.5f), FVector3f(-0.45f, HW + 0.37f, 2.5f), Window, In);
			for (float X : { -6.5f, -3.9f, -1.3f, 1.3f, 3.9f, 6.5f })
			{
				W.Box(FVector3f(X - 0.35f, -0.45f, 9.6f), FVector3f(X + 0.35f, 0.45f, BarracksTop), Brick);
			}
			break;
		}
		case ESitePiece::Scaffold:
		{
			const float SX = HL + 0.9f, SY = HW + 0.9f, Top = BarracksTop + 1.f;
			auto Pole = [&](float X, float Y) { W.Box(FVector3f(X - 0.1f, Y - 0.1f, 0.f), FVector3f(X + 0.1f, Y + 0.1f, Top), Timber); };
			for (float X = -SX; X <= SX + 0.01f; X += SX / 4.f)
			{
				Pole(X, -SY);
				Pole(X, SY);
			}
			for (float Y : { -SY / 3.f, SY / 3.f })
			{
				Pole(-SX, Y);
				Pole(SX, Y);
			}
			for (float Z = 2.1f; Z < Top; Z += 2.1f)
			{
				// Boards between the poles and the wall, with a ledger on the outside.
				for (float Side : { -1.f, 1.f })
				{
					W.Box(FVector3f(-SX, Side > 0 ? HW + 0.05f : -SY, Z - 0.08f), FVector3f(SX, Side > 0 ? SY : -HW - 0.05f, Z), DarkTimber);
					W.Box(FVector3f(-SX, Side * SY - 0.07f, Z + 0.9f), FVector3f(SX, Side * SY + 0.07f, Z + 1.02f), Timber);
				}
				for (float End : { -1.f, 1.f })
				{
					W.Box(FVector3f(End > 0 ? HL + 0.05f : -SX, -SY, Z - 0.08f), FVector3f(End > 0 ? SX : -HL - 0.05f, SY, Z), DarkTimber);
				}
			}
			break;
		}
		case ESitePiece::CraneMast:
			W.Box(FVector3f(-0.3f, -0.3f, 0.f), FVector3f(0.3f, 0.3f, 16.f), Timber);
			W.Box(FVector3f(-1.4f, -1.4f, 0.f), FVector3f(1.4f, 1.4f, 0.5f), DarkTimber);
			break;
		case ESitePiece::CraneJib:
			W.Box(FVector3f(-2.6f, -0.22f, 14.6f), FVector3f(11.f, 0.22f, 15.1f), Timber);
			W.Box(FVector3f(-2.6f, -0.6f, 13.4f), FVector3f(-1.2f, 0.6f, 14.6f), Stone);     // counterweight
			W.Box(FVector3f(9.7f, -0.05f, 9.5f), FVector3f(9.8f, 0.05f, 14.6f), DarkTimber); // rope
			W.Box(FVector3f(9.3f, -0.4f, 8.8f), FVector3f(10.2f, 0.4f, 9.5f), Brick);       // hod of bricks
			break;
		case ESitePiece::Wagon:
			W.DropShadow(3.2f, 1.6f, 0.6f);
			W.Box(FVector3f(-2.f, -0.9f, 0.7f), FVector3f(1.2f, 0.9f, 1.5f), DarkTimber);
			W.Box(FVector3f(-1.8f, -0.7f, 1.5f), FVector3f(1.f, 0.7f, 2.2f), Brick);
			for (float X : { -1.4f, 0.7f })
			{
				for (float Y : { -1.f, 0.9f })
				{
					W.Box(FVector3f(X - 0.45f, Y, 0.f), FVector3f(X + 0.45f, Y + 0.12f, 0.9f), Srgb(60, 44, 30));
				}
			}
			W.Box(FVector3f(1.9f, -0.35f, 1.f), FVector3f(3.6f, 0.35f, 1.9f), Srgb(104, 70, 44));     // horse
			W.Box(FVector3f(3.4f, -0.22f, 1.7f), FVector3f(4.2f, 0.22f, 2.5f), Srgb(104, 70, 44));    // neck and head
			for (float X : { 2.1f, 3.3f })
			{
				W.Box(FVector3f(X - 0.12f, -0.3f, 0.f), FVector3f(X + 0.12f, 0.3f, 1.f), Srgb(80, 54, 34));
			}
			break;
		case ESitePiece::Flagpole:
			W.Box(FVector3f(-0.1f, -0.1f, 0.f), FVector3f(0.1f, 0.1f, 12.6f), Srgb(236, 232, 222));
			W.Box(FVector3f(-0.18f, -0.18f, 12.6f), FVector3f(0.18f, 0.18f, 12.95f), Srgb(214, 180, 96));
			break;
		case ESitePiece::Flag:
		{
			// Dannebrog 37:28 with the cross offset to the hoist; cells coloured red or white.
			const float Xs[] = { 0.f, 1.2f, 1.6f, 3.7f };
			const float Zs[] = { 0.f, 1.2f, 1.6f, 2.8f };
			const FLinearColor Red = Srgb(200, 16, 46), White = Srgb(245, 245, 240);
			for (int32 i = 0; i < 3; ++i)
			{
				for (int32 j = 0; j < 3; ++j)
				{
					const FLinearColor& C = (i == 1 || j == 1) ? White : Red;
					W.Tri(FVector3f(Xs[i], 0.f, Zs[j]), FVector3f(Xs[i + 1], 0.f, Zs[j]), FVector3f(Xs[i + 1], 0.f, Zs[j + 1]), C, FVector3f(1.f, -1.f, 1.f), false);
					W.Tri(FVector3f(Xs[i], 0.f, Zs[j]), FVector3f(Xs[i + 1], 0.f, Zs[j + 1]), FVector3f(Xs[i], 0.f, Zs[j + 1]), C, FVector3f(1.f, -1.f, 1.f), false);
				}
			}
			break;
		}
		case ESitePiece::Stables:
		{
			const float L = 6.f, Wd = 2.2f;
			W.House(FVector2f(0.f, 0.f), 12.f, 4.4f, 3.2f, 6.4f, Brick, RoofSlate);
			W.Box(FVector3f(-L - 0.1f, -Wd - 0.1f, 0.f), FVector3f(L + 0.1f, Wd + 0.1f, 0.45f), Stone);
			const FVector3f In(0.f, 0.f, 1.5f);
			for (int32 c = 0; c < 7; ++c)
			{
				const float X = -5.1f + c * 1.7f;
				for (float Side : { -1.f, 1.f })
				{
					const float Y = Side * (Wd + 0.03f);
					W.Quad(FVector3f(X - 0.45f, Y, 0.45f), FVector3f(X + 0.45f, Y, 0.45f), FVector3f(X + 0.45f, Y, 2.3f), FVector3f(X - 0.45f, Y, 2.3f), Srgb(70, 52, 36), In);
				}
			}
			W.Box(FVector3f(-0.4f, -0.4f, 5.4f), FVector3f(0.4f, 0.4f, 6.9f), Srgb(214, 204, 186));   // ventilation lantern
			break;
		}
		case ESitePiece::Depot:
		{
			const float L = 3.5f, Wd = 2.8f;
			W.House(FVector2f(0.f, 0.f), 7.f, 5.6f, 7.f, 11.6f, Brick, Srgb(176, 76, 50));
			W.Box(FVector3f(-L - 0.1f, -Wd - 0.1f, 0.f), FVector3f(L + 0.1f, Wd + 0.1f, 0.6f), Stone);
			const FVector3f In(0.f, 0.f, 3.f);
			for (int32 Storey = 0; Storey < 3; ++Storey)
			{
				const float Z0 = 1.2f + Storey * 2.f, Z1 = Z0 + 1.f;
				for (float X : { -2.3f, -1.1f, 1.1f, 2.3f })
				{
					for (float Side : { -1.f, 1.f })
					{
						const float Y = Side * (Wd + 0.03f);
						W.Quad(FVector3f(X - 0.28f, Y, Z0), FVector3f(X + 0.28f, Y, Z0), FVector3f(X + 0.28f, Y, Z1), FVector3f(X - 0.28f, Y, Z1), Window, In);
					}
				}
			}
			for (float Side : { -1.f, 1.f })   // cart doors below, hoist doors above, a green-painted stack
			{
				const float Y = Side * (Wd + 0.04f);
				W.Quad(FVector3f(-0.7f, Y, 0.6f), FVector3f(0.7f, Y, 0.6f), FVector3f(0.7f, Y, 3.f), FVector3f(-0.7f, Y, 3.f), Srgb(58, 84, 62), In);
				W.Quad(FVector3f(-0.45f, Y, 3.6f), FVector3f(0.45f, Y, 3.6f), FVector3f(0.45f, Y, 6.4f), FVector3f(-0.45f, Y, 6.4f), Srgb(58, 84, 62), In);
			}
			W.Box(FVector3f(-0.12f, Wd, 7.f), FVector3f(0.12f, Wd + 1.4f, 7.3f), DarkTimber);   // hoist beam
			break;
		}
		case ESitePiece::Infirmary:
		{
			const float L = 4.5f, Wd = 2.3f;
			W.House(FVector2f(0.f, 0.f), 9.f, 4.6f, 4.6f, 7.8f, Brick, RoofSlate);
			W.Box(FVector3f(-L - 0.1f, -Wd - 0.1f, 0.f), FVector3f(L + 0.1f, Wd + 0.1f, 0.55f), Stone);
			const FVector3f In(0.f, 0.f, 2.f);
			for (int32 Storey = 0; Storey < 2; ++Storey)
			{
				const float Z0 = 1.1f + Storey * 1.9f, Z1 = Z0 + 1.1f;
				for (int32 c = 0; c < 6; ++c)
				{
					const float X = -3.5f + c * 1.4f;
					for (float Side : { -1.f, 1.f })
					{
						const float Y = Side * (Wd + 0.03f);
						W.Quad(FVector3f(X - 0.3f, Y, Z0), FVector3f(X + 0.3f, Y, Z0), FVector3f(X + 0.3f, Y, Z1), FVector3f(X - 0.3f, Y, Z1), Window, In);
					}
				}
			}
			W.Box(FVector3f(-1.f, Wd, 0.f), FVector3f(1.f, Wd + 1.2f, 2.6f), Stone);                 // porch
			W.Box(FVector3f(-1.2f, Wd - 0.1f, 2.6f), FVector3f(1.2f, Wd + 1.4f, 2.9f), RoofSlate);
			for (float X : { -2.4f, 2.4f })
			{
				W.Box(FVector3f(X - 0.3f, -0.4f, 6.6f), FVector3f(X + 0.3f, 0.4f, 8.2f), Brick);
			}
			break;
		}
		case ESitePiece::Arsenal:
		{
			W.House(FVector2f(0.f, -3.f), 18.f, 6.5f, 6.f, 10.f, Brick, RoofSlate);
			W.House(FVector2f(-7.2f, 4.f), 8.f, 4.4f, 4.6f, 7.6f, Brick, RoofSlate, true);
			W.House(FVector2f(7.2f, 4.f), 8.f, 4.4f, 4.6f, 7.6f, Brick, RoofSlate, true);
			W.Box(FVector3f(-9.1f, -6.35f, 0.f), FVector3f(9.1f, 0.35f, 0.55f), Stone);
			const FVector3f In(0.f, -3.f, 3.f);
			for (int32 Storey = 0; Storey < 2; ++Storey)
			{
				const float Z0 = 1.2f + Storey * 2.4f, Z1 = Z0 + 1.2f;
				for (int32 c = 0; c < 11; ++c)
				{
					const float X = -7.5f + c * 1.5f, Y = 0.28f;
					if (Storey == 0 && c == 5)
					{
						W.Quad(FVector3f(-0.9f, Y, 0.55f), FVector3f(0.9f, Y, 0.55f), FVector3f(0.9f, Y, 3.4f), FVector3f(-0.9f, Y, 3.4f), Srgb(58, 70, 60), In);   // gate
						continue;
					}
					W.Quad(FVector3f(X - 0.32f, Y, Z0), FVector3f(X + 0.32f, Y, Z0), FVector3f(X + 0.32f, Y, Z1), FVector3f(X - 0.32f, Y, Z1), Window, In);
				}
			}
			for (float X : { -5.f, 0.f, 5.f })
			{
				W.Box(FVector3f(X - 0.35f, -3.4f, 8.8f), FVector3f(X + 0.35f, -2.6f, 10.6f), Brick);
			}
			break;
		}
		case ESitePiece::Lazaret:
		{
			const FLinearColor Plastered = Srgb(222, 200, 150);
			W.House(FVector2f(0.f, 0.f), 12.f, 5.2f, 5.6f, 9.2f, Plastered, Srgb(168, 70, 46));
			W.Box(FVector3f(-6.1f, -2.7f, 0.f), FVector3f(6.1f, 2.7f, 0.5f), Stone);
			const FVector3f In(0.f, 0.f, 2.f);
			for (int32 Storey = 0; Storey < 2; ++Storey)
			{
				const float Z0 = 1.1f + Storey * 2.3f, Z1 = Z0 + 1.25f;
				for (int32 c = 0; c < 8; ++c)
				{
					const float X = -4.9f + c * 1.4f;
					for (float Side : { -1.f, 1.f })
					{
						const float Y = Side * 2.63f;
						W.Quad(FVector3f(X - 0.3f, Y, Z0), FVector3f(X + 0.3f, Y, Z0), FVector3f(X + 0.3f, Y, Z1), FVector3f(X - 0.3f, Y, Z1), Window, In);
					}
				}
			}
			W.House(FVector2f(0.f, 2.8f), 3.2f, 1.6f, 5.6f, 7.6f, Plastered, Srgb(168, 70, 46), true);   // central gable bay
			for (float X : { -3.8f, 3.8f })
			{
				W.Box(FVector3f(X - 0.3f, -0.4f, 8.f), FVector3f(X + 0.3f, 0.4f, 9.8f), Brick);
			}
			break;
		}
		case ESitePiece::Battery:
		{
			const FLinearColor Turf = Srgb(96, 116, 60), TurfTop = Srgb(110, 128, 66), Gravel = Srgb(170, 156, 124), Iron = Srgb(44, 46, 50);
			const FVector3f Below(0.f, 0.f, -3.f);
			const float Half = 8.f;
			// Profile across the rampart (y, z): back foot, back crest, front crest, front foot (the sea side is +Y).
			const FVector2f Prof[] = { { -3.f, 0.f }, { -1.4f, 2.2f }, { 1.2f, 2.2f }, { 3.2f, 0.f } };
			for (int32 i = 0; i < 3; ++i)
			{
				const FLinearColor& C = i == 1 ? TurfTop : Turf;
				W.Quad(FVector3f(-Half, Prof[i].X, Prof[i].Y), FVector3f(Half, Prof[i].X, Prof[i].Y), FVector3f(Half, Prof[i + 1].X, Prof[i + 1].Y), FVector3f(-Half, Prof[i + 1].X, Prof[i + 1].Y), C, Below);
			}
			for (float End : { -1.f, 1.f })   // rampart ends
			{
				const float X = End * Half;
				W.Quad(FVector3f(X, -3.f, 0.f), FVector3f(X, -1.4f, 2.2f), FVector3f(X, 1.2f, 2.2f), FVector3f(X, 3.2f, 0.f), Turf, FVector3f(0.f, 0.f, 1.f));
			}
			W.Quad(FVector3f(-Half, -6.f, 0.1f), FVector3f(Half, -6.f, 0.1f), FVector3f(Half, -3.f, 0.1f), FVector3f(-Half, -3.f, 0.1f), Gravel, Below);   // gun yard
			for (float X : { -5.4f, -1.8f, 1.8f, 5.4f })
			{
				W.Box(FVector3f(X - 0.6f, -1.2f, 2.2f), FVector3f(X + 0.6f, 0.4f, 2.8f), Srgb(88, 64, 42));     // carriage
				W.Box(FVector3f(X - 0.22f, -0.6f, 2.6f), FVector3f(X + 0.22f, 2.4f, 3.0f), Iron);               // barrel over the crest
			}
			break;
		}
		case ESitePiece::PowderMagazine:
		{
			const FLinearColor Turf = Srgb(96, 116, 60);
			W.House(FVector2f(0.f, 0.f), 5.f, 4.f, 2.6f, 4.4f, Brick, Turf);
			W.Box(FVector3f(-2.6f, -2.1f, 0.f), FVector3f(2.6f, 2.1f, 0.4f), Stone);
			W.Quad(FVector3f(-0.5f, 2.03f, 0.4f), FVector3f(0.5f, 2.03f, 0.4f), FVector3f(0.5f, 2.03f, 2.1f), FVector3f(-0.5f, 2.03f, 2.1f), Srgb(58, 70, 60), FVector3f(0.f, 0.f, 1.f));
			// Blast wall round the vault, open at the door.
			W.Box(FVector3f(-5.f, -4.5f, 0.f), FVector3f(5.f, -4.1f, 1.3f), Brick);
			W.Box(FVector3f(-5.f, -4.5f, 0.f), FVector3f(-4.6f, 4.5f, 1.3f), Brick);
			W.Box(FVector3f(4.6f, -4.5f, 0.f), FVector3f(5.f, 4.5f, 1.3f), Brick);
			W.Box(FVector3f(-5.f, 4.1f, 0.f), FVector3f(-1.4f, 4.5f, 1.3f), Brick);
			W.Box(FVector3f(1.4f, 4.1f, 0.f), FVector3f(5.f, 4.5f, 1.3f), Brick);
			W.Box(FVector3f(-0.06f, -0.06f, 4.4f), FVector3f(0.06f, 0.06f, 6.6f), Srgb(60, 60, 64));   // lightning rod
			break;
		}
		case ESitePiece::StarFort:
		{
			const FLinearColor Turf = Srgb(96, 116, 60), TurfTop = Srgb(112, 130, 68), Yard = Srgb(150, 140, 110);
			// Star outline: 5 points (radius 17) and 5 re-entrant angles (radius 10.5).
			TArray<FVector2f> Outline;
			for (int32 k = 0; k < 10; ++k)
			{
				const float A = UE_HALF_PI + k * UE_PI / 5.f;
				const float R = (k % 2 == 0) ? 17.f : 10.5f;
				Outline.Add(FVector2f(FMath::Cos(A) * R, FMath::Sin(A) * R));
			}
			const float H = 2.6f;
			for (int32 i = 0; i < Outline.Num(); ++i)
			{
				const FVector2f P0 = Outline[i], P1 = Outline[(i + 1) % Outline.Num()];
				auto At = [](const FVector2f& P, float S, float Z) { return FVector3f(P.X * S, P.Y * S, Z); };
				const FVector2f Mid = (P0 + P1) * 0.5f;
				// Outer slope, flat crest, inner slope.
				W.Quad(At(P0, 1.f, 0.f), At(P1, 1.f, 0.f), At(P1, 0.86f, H), At(P0, 0.86f, H), Turf, FVector3f(0.f, 0.f, 0.f));
				W.Quad(At(P0, 0.86f, H), At(P1, 0.86f, H), At(P1, 0.76f, H), At(P0, 0.76f, H), TurfTop, FVector3f(Mid.X * 0.8f, Mid.Y * 0.8f, -1.f));
				W.Quad(At(P0, 0.76f, H), At(P1, 0.76f, H), At(P1, 0.66f, 0.f), At(P0, 0.66f, 0.f), Turf, FVector3f(Mid.X * 1.5f, Mid.Y * 1.5f, 0.f));
				// Yard inside the rampart.
				W.Tri(FVector3f(0.f, 0.f, 0.08f), At(P0, 0.66f, 0.08f), At(P1, 0.66f, 0.08f), Yard, FVector3f(0.f, 0.f, -1.f));
			}
			W.House(FVector2f(0.f, -1.f), 6.f, 3.4f, 2.2f, 4.2f, Srgb(118, 88, 58), Srgb(84, 74, 62));   // blockhouse
			break;
		}
		case ESitePiece::Telegraph:
		{
			W.House(FVector2f(-1.5f, 0.f), 6.f, 4.f, 3.6f, 6.2f, Brick, RoofSlate);
			W.Box(FVector3f(-4.55f, -2.05f, 0.f), FVector3f(1.55f, 2.05f, 0.45f), Stone);
			const FVector3f In(-1.5f, 0.f, 1.5f);
			for (float X : { -3.3f, -1.5f, 0.3f })
			{
				W.Quad(FVector3f(X - 0.3f, 2.03f, 1.1f), FVector3f(X + 0.3f, 2.03f, 1.1f), FVector3f(X + 0.3f, 2.03f, 2.4f), FVector3f(X - 0.3f, 2.03f, 2.4f), Window, In);
			}
			const FLinearColor Mast = Srgb(108, 84, 58);
			W.Box(FVector3f(3.5f, -0.18f, 0.f), FVector3f(3.86f, 0.18f, 11.f), Mast);
			W.Box(FVector3f(2.4f, -0.1f, 10.1f), FVector3f(4.96f, 0.1f, 10.35f), Mast);
			W.Box(FVector3f(2.6f, -0.1f, 8.9f), FVector3f(4.76f, 0.1f, 9.1f), Mast);
			for (float X : { 2.5f, 3.1f, 4.3f, 4.9f })
			{
				W.Box(FVector3f(X - 0.07f, -0.07f, 10.35f), FVector3f(X + 0.07f, 0.07f, 10.6f), Srgb(230, 230, 222));   // insulators
			}
			break;
		}
		case ESitePiece::Granary:
		{
			const FLinearColor Yellow = Srgb(200, 160, 100);
			W.House(FVector2f(0.f, 0.f), 13.f, 5.4f, 6.4f, 10.4f, Yellow, Tile);
			W.Box(FVector3f(-6.6f, -2.8f, 0.f), FVector3f(6.6f, 2.8f, 0.6f), Stone);
			const FVector3f In(0.f, 0.f, 3.f);
			for (int32 Storey = 0; Storey < 3; ++Storey)
			{
				const float Z0 = 1.2f + Storey * 1.8f, Z1 = Z0 + 0.8f;
				for (int32 c = 0; c < 9; ++c)
				{
					const float X = -5.6f + c * 1.4f;
					for (float Side : { -1.f, 1.f })
					{
						const float Y = Side * 2.73f;
						W.Quad(FVector3f(X - 0.25f, Y, Z0), FVector3f(X + 0.25f, Y, Z0), FVector3f(X + 0.25f, Y, Z1), FVector3f(X - 0.25f, Y, Z1), Window, In);
					}
				}
			}
			for (float X : { -3.f, 3.f })   // hoist gables with doors, and their beams
			{
				W.House(FVector2f(X, 2.2f), 1.8f, 2.2f, 8.2f, 9.6f, Yellow, Tile, true);
				W.Quad(FVector3f(X - 0.45f, 3.33f, 6.6f), FVector3f(X + 0.45f, 3.33f, 6.6f), FVector3f(X + 0.45f, 3.33f, 8.f), FVector3f(X - 0.45f, 3.33f, 8.f), Srgb(58, 84, 62), In);
				W.Box(FVector3f(X - 0.1f, 3.3f, 8.4f), FVector3f(X + 0.1f, 4.4f, 8.6f), DarkTimber);
			}
			break;
		}
		case ESitePiece::Train:
		{
			// Locomotive at the front (origin, running towards +X), tender, three carriages behind.
			const FLinearColor Iron = Srgb(34, 34, 38), Wheel = Srgb(120, 38, 30), Brass = Srgb(206, 170, 90);
			const FLinearColor Green = Srgb(52, 88, 60), Brown = Srgb(112, 70, 44), Roof = Srgb(70, 70, 74);
			W.Box(FVector3f(-3.4f, -0.62f, 0.35f), FVector3f(0.f, 0.62f, 0.6f), Iron);        // frame
			W.Box(FVector3f(-2.4f, -0.52f, 0.6f), FVector3f(-0.15f, 0.52f, 1.55f), Iron);     // boiler
			W.Box(FVector3f(-0.9f, -0.2f, 1.55f), FVector3f(-0.5f, 0.2f, 2.5f), Iron);        // chimney
			W.Box(FVector3f(-1.6f, -0.24f, 1.55f), FVector3f(-1.2f, 0.24f, 1.9f), Brass);     // dome
			W.Box(FVector3f(-3.4f, -0.62f, 0.6f), FVector3f(-2.4f, 0.62f, 2.1f), Iron);       // cab
			W.Box(FVector3f(-3.5f, -0.7f, 2.1f), FVector3f(-2.3f, 0.7f, 2.25f), Roof);
			W.Box(FVector3f(0.f, -0.6f, 0.3f), FVector3f(0.12f, 0.6f, 0.6f), Wheel);          // buffer beam
			for (float X : { -2.9f, -1.8f, -0.7f })
			{
				W.Box(FVector3f(X - 0.4f, -0.68f, 0.f), FVector3f(X + 0.4f, 0.68f, 0.8f), Wheel);
			}
			W.Box(FVector3f(-5.1f, -0.6f, 0.2f), FVector3f(-3.6f, 0.6f, 1.3f), Iron);         // tender
			W.Box(FVector3f(-5.f, -0.5f, 1.3f), FVector3f(-3.7f, 0.5f, 1.5f), Srgb(40, 36, 32));
			for (int32 c = 0; c < 3; ++c)
			{
				const float X1 = -5.35f - c * 3.15f, X0 = X1 - 2.9f;
				const FLinearColor& Body = c == 0 ? Brown : Green;
				W.Box(FVector3f(X0, -0.62f, 0.3f), FVector3f(X1, 0.62f, 1.6f), Body);
				W.Box(FVector3f(X0 - 0.08f, -0.7f, 1.6f), FVector3f(X1 + 0.08f, 0.7f, 1.8f), Roof);
				for (int32 k = 0; k < 4; ++k)
				{
					const float X = X0 + 0.45f + k * 0.67f;
					for (float Side : { -1.f, 1.f })
					{
						W.Quad(FVector3f(X, Side * 0.63f, 0.9f), FVector3f(X + 0.4f, Side * 0.63f, 0.9f), FVector3f(X + 0.4f, Side * 0.63f, 1.4f), FVector3f(X, Side * 0.63f, 1.4f),
							Srgb(222, 206, 150), FVector3f((X0 + X1) * 0.5f, 0.f, 1.f));
					}
				}
				for (float X : { X0 + 0.5f, X1 - 0.5f })
				{
					W.Box(FVector3f(X - 0.3f, -0.66f, 0.f), FVector3f(X + 0.3f, 0.66f, 0.45f), Iron);
				}
			}
			break;
		}
		case ESitePiece::Station:
		{
			// Small brick station house; platform and canopy on the track side (-Y).
			W.House(FVector2f(0.f, 0.8f), 6.f, 3.2f, 3.f, 5.f, Srgb(178, 96, 66), RoofSlate);
			W.Box(FVector3f(-4.f, -1.9f, 0.f), FVector3f(4.f, -0.8f, 0.35f), Stone);
			W.Box(FVector3f(-3.6f, -1.8f, 2.4f), FVector3f(3.6f, -0.8f, 2.6f), RoofSlate);
			for (float X : { -3.3f, 0.f, 3.3f })
			{
				W.Box(FVector3f(X - 0.08f, -1.7f, 0.35f), FVector3f(X + 0.08f, -1.54f, 2.4f), DarkTimber);
			}
			break;
		}
		case ESitePiece::FormationInfantry:
		case ESitePiece::FormationGuard:
		case ESitePiece::FormationJager:
		{
			// Four ranks of eight files behind a colour party; one soldier = body, legs, head and headgear.
			const bool bGuard = Piece == ESitePiece::FormationGuard, bJager = Piece == ESitePiece::FormationJager;
			const FLinearColor Coat = bGuard ? Srgb(176, 30, 34) : bJager ? Srgb(46, 70, 44) : Srgb(34, 44, 86);
			const FLinearColor Trousers = bJager ? Srgb(60, 76, 58) : Srgb(122, 150, 190);
			const FLinearColor Hat = bGuard ? Srgb(24, 22, 22) : Srgb(30, 30, 36);
			const FLinearColor Skin = Srgb(214, 170, 140), Steel = Srgb(200, 200, 206);
			auto Soldier = [&](float X, float Y)
			{
				W.Box(FVector3f(X - 0.12f, Y - 0.1f, 0.f), FVector3f(X + 0.12f, Y + 0.1f, 0.5f), Trousers);
				W.Box(FVector3f(X - 0.14f, Y - 0.13f, 0.5f), FVector3f(X + 0.14f, Y + 0.13f, 1.05f), Coat);
				W.Box(FVector3f(X - 0.08f, Y - 0.08f, 1.05f), FVector3f(X + 0.08f, Y + 0.08f, 1.22f), Skin);
				W.Box(FVector3f(X - 0.09f, Y - 0.09f, 1.22f), FVector3f(X + 0.09f, Y + 0.09f, bGuard ? 1.6f : 1.4f), Hat);
				W.Box(FVector3f(X - 0.02f, Y + 0.12f, 0.6f), FVector3f(X + 0.02f, Y + 0.16f, 1.65f), Steel);   // musket on the shoulder
			};
			for (int32 Rank = 0; Rank < 4; ++Rank)
			{
				for (int32 File = 0; File < 8; ++File)
				{
					Soldier(-Rank * 0.55f - 0.6f, (File - 3.5f) * 0.36f);
				}
			}
			// Officer ahead, and the colour: pole with Dannebrog.
			Soldier(0.9f, 0.f);
			W.Box(FVector3f(0.3f, -0.03f, 0.f), FVector3f(0.36f, 0.03f, 2.6f), Srgb(120, 90, 60));
			const FLinearColor Red = Srgb(200, 16, 46), White = Srgb(245, 245, 240);
			const float Xs[] = { -0.64f, -0.34f, -0.24f, 0.33f };
			const float Zs[] = { 1.8f, 2.08f, 2.18f, 2.58f };
			for (int32 i = 0; i < 3; ++i)
			{
				for (int32 j = 0; j < 3; ++j)
				{
					const FLinearColor& C = (i == 1 || j == 1) ? White : Red;
					W.Quad(FVector3f(Xs[i], 0.f, Zs[j]), FVector3f(Xs[i + 1], 0.f, Zs[j]), FVector3f(Xs[i + 1], 0.f, Zs[j + 1]), FVector3f(Xs[i], 0.f, Zs[j + 1]), C, FVector3f(0.f, -1.f, 2.f));
					W.Quad(FVector3f(Xs[i], 0.01f, Zs[j]), FVector3f(Xs[i + 1], 0.01f, Zs[j]), FVector3f(Xs[i + 1], 0.01f, Zs[j + 1]), FVector3f(Xs[i], 0.01f, Zs[j + 1]), C, FVector3f(0.f, 1.f, 2.f));
				}
			}
			break;
		}
		case ESitePiece::FormationCavalry:
		{
			// Two ranks of six riders: horse body, legs, neck; rider in light blue with a helmet.
			const FLinearColor Horse = Srgb(96, 64, 40), Dark = Srgb(52, 36, 24), Coat = Srgb(118, 146, 188), Helmet = Srgb(210, 206, 196);
			for (int32 Rank = 0; Rank < 2; ++Rank)
			{
				for (int32 File = 0; File < 6; ++File)
				{
					const float X = -Rank * 1.4f, Y = (File - 2.5f) * 0.55f;
					W.Box(FVector3f(X - 0.5f, Y - 0.14f, 0.55f), FVector3f(X + 0.4f, Y + 0.14f, 0.95f), File % 3 == 1 ? Dark : Horse);
					W.Box(FVector3f(X + 0.3f, Y - 0.09f, 0.85f), FVector3f(X + 0.62f, Y + 0.09f, 1.3f), Horse);
					for (float LX : { -0.4f, 0.3f })
					{
						W.Box(FVector3f(X + LX - 0.05f, Y - 0.12f, 0.f), FVector3f(X + LX + 0.05f, Y + 0.12f, 0.55f), Dark);
					}
					W.Box(FVector3f(X - 0.16f, Y - 0.12f, 0.95f), FVector3f(X + 0.08f, Y + 0.12f, 1.5f), Coat);
					W.Box(FVector3f(X - 0.09f, Y - 0.08f, 1.5f), FVector3f(X + 0.03f, Y + 0.08f, 1.75f), Helmet);
				}
			}
			W.Box(FVector3f(0.9f, -0.03f, 0.f), FVector3f(0.95f, 0.03f, 2.6f), Srgb(120, 90, 60));   // guidon
			W.Quad(FVector3f(0.95f, 0.f, 2.1f), FVector3f(1.5f, 0.f, 2.3f), FVector3f(0.95f, 0.f, 2.55f), FVector3f(0.95f, 0.f, 2.1f), Srgb(200, 16, 46), FVector3f(1.f, -1.f, 2.f));
			break;
		}
		case ESitePiece::FormationArtillery:
		case ESitePiece::FormationHorseArtillery:
		{
			// Two guns in battery, each with its limber and a team behind (four horses; six for horse artillery),
			// and the crew: on foot beside the gun, or mounted alongside the team.
			const bool bRiding = Piece == ESitePiece::FormationHorseArtillery;
			const FLinearColor Wood = Srgb(110, 118, 84), Barrel = Srgb(48, 52, 50), Horse = Srgb(96, 64, 40), Coat = Srgb(34, 44, 86);
			for (float Y : { -1.f, 1.f })
			{
				W.Box(FVector3f(0.1f, Y - 0.1f, 0.35f), FVector3f(1.1f, Y + 0.1f, 0.55f), Barrel);                // barrel
				W.Box(FVector3f(-0.9f, Y - 0.08f, 0.2f), FVector3f(0.3f, Y + 0.08f, 0.4f), Wood);                // trail
				for (float S : { -0.35f, 0.35f })
				{
					W.Box(FVector3f(-0.05f, Y + S - 0.05f, 0.f), FVector3f(0.55f, Y + S + 0.05f, 0.6f), Wood);   // wheels
				}
				W.Box(FVector3f(-2.1f, Y - 0.35f, 0.3f), FVector3f(-1.4f, Y + 0.35f, 0.75f), Wood);             // limber
				for (int32 H = 0; H < (bRiding ? 3 : 2); ++H)
				{
					for (float S : { -0.2f, 0.2f })
					{
						const float X = -2.6f - H * 0.9f;
						W.Box(FVector3f(X - 0.4f, Y + S - 0.1f, 0.45f), FVector3f(X + 0.3f, Y + S + 0.1f, 0.8f), Horse);
						W.Box(FVector3f(X + 0.2f, Y + S - 0.07f, 0.7f), FVector3f(X + 0.45f, Y + S + 0.07f, 1.05f), Horse);
					}
				}
				if (bRiding)
				{
					// Mounted gunners riding beside the team.
					for (float RX : { -2.4f, -3.4f })
					{
						const float RY = Y + (Y > 0.f ? 0.75f : -0.75f);
						W.Box(FVector3f(RX - 0.4f, RY - 0.12f, 0.5f), FVector3f(RX + 0.35f, RY + 0.12f, 0.85f), Horse);
						W.Box(FVector3f(RX + 0.25f, RY - 0.08f, 0.75f), FVector3f(RX + 0.5f, RY + 0.08f, 1.15f), Horse);
						W.Box(FVector3f(RX - 0.14f, RY - 0.11f, 0.85f), FVector3f(RX + 0.08f, RY + 0.11f, 1.35f), Coat);
						W.Box(FVector3f(RX - 0.08f, RY - 0.07f, 1.35f), FVector3f(RX + 0.03f, RY + 0.07f, 1.55f), Srgb(30, 30, 36));
					}
				}
				else
				{
					for (float CX : { -0.4f, -0.8f })
					{
						W.Box(FVector3f(CX - 0.1f, Y + 0.55f, 0.f), FVector3f(CX + 0.1f, Y + 0.75f, 1.1f), Coat);
					}
				}
			}
			break;
		}
		case ESitePiece::TrainEngine:
		{
			// The locomotive and tender of the Train piece, moved so their middle is the origin.
			const FLinearColor Iron = Srgb(34, 34, 38), Wheel = Srgb(120, 38, 30), Brass = Srgb(206, 170, 90), Roof = Srgb(70, 70, 74);
			const float O = 2.55f;
			auto B = [&](float X0, float Y0, float Z0, float X1, float Y1, float Z1, const FLinearColor& C) { W.Box(FVector3f(X0 + O, Y0, Z0), FVector3f(X1 + O, Y1, Z1), C); };
			B(-3.4f, -0.62f, 0.35f, 0.f, 0.62f, 0.6f, Iron);
			B(-2.4f, -0.52f, 0.6f, -0.15f, 0.52f, 1.55f, Iron);
			B(-0.9f, -0.2f, 1.55f, -0.5f, 0.2f, 2.5f, Iron);
			B(-1.6f, -0.24f, 1.55f, -1.2f, 0.24f, 1.9f, Brass);
			B(-3.4f, -0.62f, 0.6f, -2.4f, 0.62f, 2.1f, Iron);
			B(-3.5f, -0.7f, 2.1f, -2.3f, 0.7f, 2.25f, Roof);
			B(0.f, -0.6f, 0.3f, 0.12f, 0.6f, 0.6f, Wheel);
			for (float X : { -2.9f, -1.8f, -0.7f })
			{
				B(X - 0.4f, -0.68f, 0.f, X + 0.4f, 0.68f, 0.8f, Wheel);
			}
			B(-5.1f, -0.6f, 0.2f, -3.6f, 0.6f, 1.3f, Iron);
			B(-5.f, -0.5f, 1.3f, -3.7f, 0.5f, 1.5f, Srgb(40, 36, 32));
			break;
		}
		case ESitePiece::TrainCarBrown:
		case ESitePiece::TrainCarGreen:
		{
			const FLinearColor Iron = Srgb(34, 34, 38), Roof = Srgb(70, 70, 74);
			const FLinearColor Body = Piece == ESitePiece::TrainCarBrown ? Srgb(112, 70, 44) : Srgb(52, 88, 60);
			const float X0 = -1.45f, X1 = 1.45f;
			W.Box(FVector3f(X0, -0.62f, 0.3f), FVector3f(X1, 0.62f, 1.6f), Body);
			W.Box(FVector3f(X0 - 0.08f, -0.7f, 1.6f), FVector3f(X1 + 0.08f, 0.7f, 1.8f), Roof);
			for (int32 k = 0; k < 4; ++k)
			{
				const float X = X0 + 0.45f + k * 0.67f;
				for (float Side : { -1.f, 1.f })
				{
					W.Quad(FVector3f(X, Side * 0.63f, 0.9f), FVector3f(X + 0.4f, Side * 0.63f, 0.9f), FVector3f(X + 0.4f, Side * 0.63f, 1.4f), FVector3f(X, Side * 0.63f, 1.4f),
						Srgb(222, 206, 150), FVector3f(0.f, 0.f, 1.f));
				}
			}
			for (float X : { X0 + 0.5f, X1 - 0.5f })
			{
				W.Box(FVector3f(X - 0.3f, -0.66f, 0.f), FVector3f(X + 0.3f, 0.66f, 0.45f), Iron);
			}
			break;
		}
		}
		static const TCHAR* Names[] = { TEXT("Ground"), TEXT("Barracks"), TEXT("Scaffold"), TEXT("CraneMast"), TEXT("CraneJib"), TEXT("Wagon"), TEXT("Flagpole"), TEXT("Flag"),
			TEXT("Stables"), TEXT("Depot"), TEXT("Infirmary"), TEXT("Arsenal"), TEXT("Lazaret"), TEXT("Battery"), TEXT("PowderMagazine"), TEXT("StarFort"),
			TEXT("Telegraph"), TEXT("Granary"), TEXT("Train"), TEXT("Station"),
			TEXT("FormationInfantry"), TEXT("FormationGuard"), TEXT("FormationJager"), TEXT("FormationCavalry"), TEXT("FormationArtillery"), TEXT("FormationHorseArtillery"),
			TEXT("TrainEngine"), TEXT("TrainCarBrown"), TEXT("TrainCarGreen") };
		return Finish(W, Material, FString::Printf(TEXT("SM_Campaign1851_Site_%s"), Names[int32(Piece)]));
	}
}

namespace Campaign1851Scenery
{
	UStaticMesh* BuildScaffold(float Length, float Width, float Top, UMaterialInterface* Material)
	{
		const FLinearColor Timber = Srgb(156, 116, 72);
		const FLinearColor DarkTimber = Srgb(96, 70, 46);
		const float HL = Length * 0.5f, HW = Width * 0.5f, SX = HL + 0.9f, SY = HW + 0.9f;
		FWriter W;
		auto Pole = [&](float X, float Y) { W.Box(FVector3f(X - 0.1f, Y - 0.1f, 0.f), FVector3f(X + 0.1f, Y + 0.1f, Top), Timber); };
		const int32 Bays = FMath::Max(2, FMath::RoundToInt(Length / 4.f));
		for (int32 b = 0; b <= Bays; ++b)
		{
			const float X = -SX + 2.f * SX * b / Bays;
			Pole(X, -SY);
			Pole(X, SY);
		}
		for (float Y : { -SY / 3.f, SY / 3.f })
		{
			Pole(-SX, Y);
			Pole(SX, Y);
		}
		for (float Z = 2.1f; Z < Top; Z += 2.1f)
		{
			for (float Side : { -1.f, 1.f })
			{
				W.Box(FVector3f(-SX, Side > 0 ? HW + 0.05f : -SY, Z - 0.08f), FVector3f(SX, Side > 0 ? SY : -HW - 0.05f, Z), DarkTimber);
				W.Box(FVector3f(-SX, Side * SY - 0.07f, Z + 0.9f), FVector3f(SX, Side * SY + 0.07f, Z + 1.02f), Timber);
			}
			for (float End : { -1.f, 1.f })
			{
				W.Box(FVector3f(End > 0 ? HL + 0.05f : -SX, -SY, Z - 0.08f), FVector3f(End > 0 ? SX : -HL - 0.05f, SY, Z), DarkTimber);
			}
		}
		return Finish(W, Material, FString::Printf(TEXT("SM_Campaign1851_Scaffold_%.0fx%.0f"), Length, Width));
	}
}

namespace Campaign1851Scenery
{
	UStaticMesh* BuildPlotGround(float Length, float Width, UMaterialInterface* Material)
	{
		const FLinearColor Dug = Srgb(128, 100, 70), Stake = Srgb(96, 70, 46);
		const float HL = Length * 0.5f + 1.2f, HW = Width * 0.5f + 1.2f;
		FWriter W;
		W.Quad(FVector3f(-HL, -HW, 0.12f), FVector3f(HL, -HW, 0.12f), FVector3f(HL, HW, 0.12f), FVector3f(-HL, HW, 0.12f), Dug, FVector3f(0.f, 0.f, -5.f));
		for (float X : { -HL + 0.3f, HL - 0.3f })
		{
			for (float Y : { -HW + 0.3f, HW - 0.3f })
			{
				W.Box(FVector3f(X - 0.15f, Y - 0.15f, 0.f), FVector3f(X + 0.15f, Y + 0.15f, 1.3f), Stake);
			}
		}
		return Finish(W, Material, FString::Printf(TEXT("SM_Campaign1851_Plot_%.0fx%.0f"), Length, Width));
	}
}
