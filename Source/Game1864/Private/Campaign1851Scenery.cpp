#include "Campaign1851Scenery.h"

#include "Campaign1851Fort.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"

namespace
{
#ifndef CAMPAIGN1851_SRGB   // one definition per unity blob
#define CAMPAIGN1851_SRGB
	FLinearColor Srgb(uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); }
#endif

	const FLinearColor Plaster = Srgb(226, 216, 192);
	const FLinearColor Ochre = Srgb(214, 172, 98);
	const FLinearColor Tile = Srgb(168, 70, 46);
	const FLinearColor Thatch = Srgb(128, 104, 72);
	const FLinearColor Slate = Srgb(66, 70, 78);
	const FLinearColor Beech = Srgb(62, 90, 38);
	const FLinearColor Spruce = Srgb(36, 58, 36);
	const FLinearColor Bark = Srgb(72, 54, 38);
	const FLinearColor ShadeGreen = Srgb(30, 38, 20);

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

		/** A tapered round rod from P0 to P1 (radius R0 to R1), N sides, closed at the ends: legs, arms, barrels. */
		void Frustum(const FVector3f& P0, const FVector3f& P1, float R0, float R1, const FLinearColor& Base, int32 N = 8)
		{
			FVector3f Axis = P1 - P0;
			const float Len = Axis.Size();
			if (Len < 1e-4f)
			{
				return;
			}
			Axis /= Len;
			const FVector3f U = (FMath::Abs(Axis.Z) < 0.9f ? FVector3f::CrossProduct(Axis, FVector3f(0.f, 0.f, 1.f)) : FVector3f::CrossProduct(Axis, FVector3f(1.f, 0.f, 0.f))).GetSafeNormal();
			const FVector3f V = FVector3f::CrossProduct(Axis, U);
			const FVector3f In = (P0 + P1) * 0.5f;
			for (int32 k = 0; k < N; ++k)
			{
				const float A0 = k * UE_TWO_PI / N, A1 = (k + 1) * UE_TWO_PI / N;
				const FVector3f D0 = U * FMath::Cos(A0) + V * FMath::Sin(A0), D1 = U * FMath::Cos(A1) + V * FMath::Sin(A1);
				Quad(P0 + D0 * R0, P0 + D1 * R0, P1 + D1 * R1, P1 + D0 * R1, Base, In);
				if (R0 > 1e-4f) { Tri(P0, P0 + D0 * R0, P0 + D1 * R0, Base, In); }
				if (R1 > 1e-4f) { Tri(P1, P1 + D0 * R1, P1 + D1 * R1, Base, In); }
			}
		}

		/** A low-poly ellipsoid: heads, horse bodies, crowns. */
		void Ball(const FVector3f& C, const FVector3f& Radii, const FLinearColor& Base, int32 N = 8, int32 M = 5)
		{
			auto At = [&](int32 i, int32 j)
			{
				const float Lat = -UE_HALF_PI + UE_PI * j / M, Lon = UE_TWO_PI * i / N;
				return C + FVector3f(FMath::Cos(Lat) * FMath::Cos(Lon) * Radii.X, FMath::Cos(Lat) * FMath::Sin(Lon) * Radii.Y, FMath::Sin(Lat) * Radii.Z);
			};
			for (int32 j = 0; j < M; ++j)
			{
				for (int32 i = 0; i < N; ++i)
				{
					if (j == 0) { Tri(At(i, 0), At(i, 1), At(i + 1, 1), Base, C); }
					else if (j == M - 1) { Tri(At(i, j), At(i + 1, j), At(i, M), Base, C); }
					else { Quad(At(i, j), At(i + 1, j), At(i + 1, j + 1), At(i, j + 1), Base, C); }
				}
			}
		}

		/** A soldier of 1851 on foot, facing +X, about 1.7 units tall. */
		void Soldier(float X, float Y, const FLinearColor& Coat, const FLinearColor& Trousers, const FLinearColor& Hat, float HatHeight, float HatTop, const FLinearColor& Belts, bool bRifle = true)
		{
			const FLinearColor Skin(0.66f, 0.42f, 0.32f), Leather(0.05f, 0.04f, 0.035f), Steel(0.62f, 0.62f, 0.66f), Stock(0.3f, 0.18f, 0.09f);
			// Legs, a little apart; boots.
			for (float S : { -0.065f, 0.065f })
			{
				Frustum(FVector3f(X, Y + S, 0.52f), FVector3f(X, Y + S * 1.2f, 0.1f), 0.055f, 0.045f, Trousers, 6);
				Frustum(FVector3f(X + 0.02f, Y + S * 1.2f, 0.12f), FVector3f(X + 0.05f, Y + S * 1.2f, 0.f), 0.05f, 0.05f, Leather, 6);
			}
			// Coat: skirts, waist, shoulders.
			Frustum(FVector3f(X, Y, 0.42f), FVector3f(X, Y, 0.62f), 0.15f, 0.13f, Coat);
			Frustum(FVector3f(X, Y, 0.62f), FVector3f(X, Y, 1.02f), 0.13f, 0.16f, Coat);
			Ball(FVector3f(X, Y, 1.02f), FVector3f(0.15f, 0.17f, 0.06f), Coat, 8, 3);
			// Cross belts over the chest and the waist belt.
			Frustum(FVector3f(X + 0.14f, Y - 0.12f, 0.98f), FVector3f(X + 0.15f, Y + 0.1f, 0.6f), 0.018f, 0.018f, Belts, 4);
			Frustum(FVector3f(X + 0.14f, Y + 0.12f, 0.98f), FVector3f(X + 0.15f, Y - 0.1f, 0.6f), 0.018f, 0.018f, Belts, 4);
			Frustum(FVector3f(X, Y, 0.63f), FVector3f(X, Y, 0.67f), 0.14f, 0.14f, Belts);
			// Arms: the right hand at the musket, the left hanging.
			Frustum(FVector3f(X, Y - 0.17f, 0.98f), FVector3f(X + 0.03f, Y - 0.19f, 0.62f), 0.045f, 0.04f, Coat, 6);
			Frustum(FVector3f(X, Y + 0.17f, 0.98f), FVector3f(X + 0.1f, Y + 0.2f, 0.72f), 0.045f, 0.04f, Coat, 6);
			Ball(FVector3f(X + 0.11f, Y + 0.2f, 0.7f), FVector3f(0.035f, 0.035f, 0.035f), Skin, 6, 3);
			// Collar, head and headgear with its peak.
			Frustum(FVector3f(X, Y, 1.04f), FVector3f(X, Y, 1.1f), 0.06f, 0.055f, Coat, 6);
			Ball(FVector3f(X + 0.01f, Y, 1.18f), FVector3f(0.085f, 0.08f, 0.1f), Skin);
			Frustum(FVector3f(X, Y, 1.22f), FVector3f(X - 0.01f, Y, 1.22f + HatHeight), 0.088f, HatTop, Hat);
			Frustum(FVector3f(X + 0.06f, Y, 1.24f), FVector3f(X + 0.14f, Y, 1.23f), 0.05f, 0.03f, Leather, 4);
			if (bRifle)
			{
				// The musket shouldered: butt at the hip, barrel up past the head, bayonet fixed.
				Frustum(FVector3f(X + 0.12f, Y + 0.22f, 0.6f), FVector3f(X + 0.1f, Y + 0.23f, 0.9f), 0.03f, 0.02f, Stock, 4);
				Frustum(FVector3f(X + 0.1f, Y + 0.23f, 0.9f), FVector3f(X + 0.07f, Y + 0.24f, 1.62f), 0.014f, 0.012f, Steel, 4);
				Frustum(FVector3f(X + 0.07f, Y + 0.24f, 1.62f), FVector3f(X + 0.065f, Y + 0.24f, 1.9f), 0.008f, 0.002f, Steel, 4);
			}
		}

		/** A horse facing +X, about 1.4 units at the head; a darker mane, tail and hind legs. */
		void Horse(float X, float Y, const FLinearColor& Coat, const FLinearColor& Dark, float Walk = 0.f)
		{
			Ball(FVector3f(X - 0.05f, Y, 0.82f), FVector3f(0.5f, 0.17f, 0.2f), Coat, 10, 5);
			Frustum(FVector3f(X + 0.35f, Y, 0.9f), FVector3f(X + 0.6f, Y, 1.28f), 0.11f, 0.07f, Coat, 6);
			Frustum(FVector3f(X + 0.58f, Y, 1.3f), FVector3f(X + 0.8f, Y, 1.12f), 0.075f, 0.045f, Coat, 6);
			Frustum(FVector3f(X + 0.36f, Y, 1.08f), FVector3f(X + 0.58f, Y, 1.36f), 0.03f, 0.03f, Dark, 4);
			Frustum(FVector3f(X - 0.52f, Y, 0.9f), FVector3f(X - 0.68f, Y, 0.5f), 0.05f, 0.02f, Dark, 4);
			const float Legs[4][2] = { { 0.3f, -0.1f }, { 0.3f, 0.1f }, { -0.38f, -0.1f }, { -0.38f, 0.1f } };
			for (int32 l = 0; l < 4; ++l)
			{
				const float Step = (l % 2 == 0 ? 1.f : -1.f) * Walk;
				Frustum(FVector3f(X + Legs[l][0], Y + Legs[l][1], 0.72f), FVector3f(X + Legs[l][0] + Step, Y + Legs[l][1], 0.05f), 0.045f, 0.03f, l < 2 ? Coat : Dark, 5);
			}
		}

		/** A rider seated on a Horse at X, Y. */
		void Rider(float X, float Y, const FLinearColor& Coat, const FLinearColor& Trousers, const FLinearColor& Hat, float HatHeight, bool bSabre = true)
		{
			const FLinearColor Skin(0.66f, 0.42f, 0.32f), Steel(0.66f, 0.66f, 0.7f), Leather(0.05f, 0.04f, 0.035f);
			for (float S : { -1.f, 1.f })
			{
				Frustum(FVector3f(X - 0.02f, Y + S * 0.1f, 1.0f), FVector3f(X + 0.06f, Y + S * 0.19f, 0.62f), 0.05f, 0.045f, Trousers, 5);
				Frustum(FVector3f(X + 0.06f, Y + S * 0.19f, 0.66f), FVector3f(X + 0.09f, Y + S * 0.19f, 0.5f), 0.05f, 0.05f, Leather, 5);
			}
			Frustum(FVector3f(X - 0.04f, Y, 1.0f), FVector3f(X - 0.02f, Y, 1.42f), 0.13f, 0.15f, Coat);
			Ball(FVector3f(X - 0.02f, Y, 1.42f), FVector3f(0.13f, 0.15f, 0.05f), Coat, 8, 3);
			Frustum(FVector3f(X - 0.02f, Y - 0.15f, 1.38f), FVector3f(X + 0.12f, Y - 0.14f, 1.12f), 0.04f, 0.035f, Coat, 5);
			Frustum(FVector3f(X - 0.02f, Y + 0.15f, 1.38f), FVector3f(X + 0.14f, Y + 0.16f, 1.14f), 0.04f, 0.035f, Coat, 5);
			Ball(FVector3f(X, Y, 1.56f), FVector3f(0.08f, 0.075f, 0.095f), Skin);
			Frustum(FVector3f(X - 0.01f, Y, 1.6f), FVector3f(X - 0.02f, Y, 1.6f + HatHeight), 0.085f, 0.06f, Hat);
			if (bSabre)
			{
				Frustum(FVector3f(X + 0.14f, Y + 0.18f, 1.14f), FVector3f(X + 0.02f, Y + 0.2f, 1.9f), 0.012f, 0.006f, Steel, 4);
			}
		}

		/** A field gun on its carriage, muzzle to +X: round barrel, spoked wheels, trail. */
		void FieldGun(float X, float Y, const FLinearColor& Wood, const FLinearColor& Barrel)
		{
			Frustum(FVector3f(X - 0.2f, Y, 0.55f), FVector3f(X + 1.0f, Y, 0.58f), 0.12f, 0.085f, Barrel, 10);
			Ball(FVector3f(X - 0.22f, Y, 0.55f), FVector3f(0.07f, 0.07f, 0.07f), Barrel, 6, 3);
			Frustum(FVector3f(X - 0.05f, Y, 0.45f), FVector3f(X - 1.05f, Y, 0.08f), 0.09f, 0.07f, Wood, 6);
			Frustum(FVector3f(X + 0.1f, Y - 0.36f, 0.35f), FVector3f(X + 0.1f, Y + 0.36f, 0.35f), 0.035f, 0.035f, Wood, 6);
			for (float S : { -0.36f, 0.36f })
			{
				const FVector3f Hub(X + 0.1f, Y + S, 0.35f);
				const int32 Spokes = 10;
				for (int32 k = 0; k < Spokes; ++k)
				{
					const float A0 = k * UE_TWO_PI / Spokes, A1 = (k + 1) * UE_TWO_PI / Spokes;
					const FVector3f R0 = Hub + FVector3f(FMath::Cos(A0) * 0.35f, 0.f, FMath::Sin(A0) * 0.35f);
					const FVector3f R1 = Hub + FVector3f(FMath::Cos(A1) * 0.35f, 0.f, FMath::Sin(A1) * 0.35f);
					Frustum(R0, R1, 0.04f, 0.04f, Wood, 4);
					Frustum(Hub, R0, 0.02f, 0.02f, Wood, 4);
				}
				Frustum(Hub - FVector3f(0.f, 0.05f, 0.f), Hub + FVector3f(0.f, 0.05f, 0.f), 0.06f, 0.06f, Barrel, 6);
			}
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
					Centre + FVector3f(RadiusX * FMath::Cos(A1), RadiusY * FMath::Sin(A1), 0.f), ShadeGreen, Below, false);
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
		case EPiece::Knick:
			// The bank of earth and the bushes on it (hazel, hawthorn, blackthorn), uneven.
			W.Box(FVector3f(-2.f, -0.55f, 0.f), FVector3f(2.f, 0.55f, 0.4f), Srgb(112, 92, 62));
			W.Box(FVector3f(-2.f, -0.4f, 0.35f), FVector3f(-0.6f, 0.4f, 1.4f), Srgb(58, 88, 40));
			W.Box(FVector3f(-0.7f, -0.45f, 0.35f), FVector3f(0.8f, 0.45f, 1.7f), Srgb(66, 98, 44));
			W.Box(FVector3f(0.7f, -0.38f, 0.35f), FVector3f(2.f, 0.38f, 1.3f), Srgb(50, 78, 36));
			break;
		case EPiece::StoneDike:
			W.Box(FVector3f(-2.f, -0.42f, 0.f), FVector3f(2.f, 0.42f, 0.5f), Srgb(150, 146, 136));
			W.Box(FVector3f(-1.7f, -0.3f, 0.45f), FVector3f(-0.5f, 0.3f, 0.72f), Srgb(128, 124, 116));
			W.Box(FVector3f(0.2f, -0.32f, 0.45f), FVector3f(1.5f, 0.32f, 0.78f), Srgb(168, 162, 150));
			break;
		case EPiece::Ditch:
		{
			const FVector3f Below(0.f, 0.f, -5.f);
			W.Quad(FVector3f(-2.f, -0.75f, 0.14f), FVector3f(2.f, -0.75f, 0.14f), FVector3f(2.f, 0.75f, 0.14f), FVector3f(-2.f, 0.75f, 0.14f), Srgb(104, 118, 66), Below);
			W.Quad(FVector3f(-2.f, -0.35f, 0.18f), FVector3f(2.f, -0.35f, 0.18f), FVector3f(2.f, 0.35f, 0.18f), FVector3f(-2.f, 0.35f, 0.18f), Srgb(70, 104, 128), Below);
			break;
		}
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
			TEXT("TownHouseTimber"), TEXT("MerchantHouse"), TEXT("Windmill"), TEXT("Oak"), TEXT("Haystack"), TEXT("Knick"), TEXT("StoneDike"), TEXT("Ditch") };
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

	UStaticMesh* BuildFlat(const TArray<FVector>& Triangles, const FLinearColor& Colour, UMaterialInterface* Material, const TCHAR* MeshName)
	{
		FWriter Writer;
		for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
		{
			const FVector3f A(Triangles[i]), B(Triangles[i + 1]), C(Triangles[i + 2]);
			Writer.Tri(A, B, C, Colour, (A + B + C) / 3.f - FVector3f(0.f, 0.f, 50.f), false);
		}
		return Finish(Writer, Material, FString(MeshName));
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
			// Four ranks of eight files behind the colour party; the officer ahead with drawn sabre.
			const bool bGuard = Piece == ESitePiece::FormationGuard, bJager = Piece == ESitePiece::FormationJager;
			const FLinearColor Coat = bGuard ? Srgb(176, 30, 34) : bJager ? Srgb(46, 70, 44) : Srgb(34, 44, 86);
			const FLinearColor Trousers = bJager ? Srgb(60, 76, 58) : Srgb(122, 150, 190);
			const FLinearColor Hat = bGuard ? Srgb(24, 22, 22) : Srgb(30, 30, 36);
			const FLinearColor Belts = bJager ? Srgb(40, 32, 24) : Srgb(236, 232, 220);
			// Guard: tall bearskin; line: shako; jaegere: low cap.
			const float HatHeight = bGuard ? 0.34f : bJager ? 0.1f : 0.18f, HatTop = bGuard ? 0.1f : bJager ? 0.08f : 0.095f;
			for (int32 Rank = 0; Rank < 4; ++Rank)
			{
				for (int32 File = 0; File < 8; ++File)
				{
					W.Soldier(-Rank * 0.55f - 0.6f, (File - 3.5f) * 0.36f + (Rank % 2) * 0.03f, Coat, Trousers, Hat, HatHeight, HatTop, Belts);
				}
			}
			W.Soldier(0.9f, 0.f, Coat, Trousers, Hat, HatHeight, HatTop, Srgb(214, 176, 102), false);
			W.Frustum(FVector3f(1.02f, 0.2f, 0.72f), FVector3f(1.25f, 0.24f, 1.55f), 0.012f, 0.006f, Srgb(200, 200, 206), 4);
			// The colour: pole with Dannebrog, and its bearer.
			W.Frustum(FVector3f(0.33f, 0.f, 0.f), FVector3f(0.33f, 0.f, 2.65f), 0.03f, 0.025f, Srgb(120, 90, 60), 6);
			W.Ball(FVector3f(0.33f, 0.f, 2.68f), FVector3f(0.05f, 0.05f, 0.05f), Srgb(214, 176, 102), 6, 3);
			W.Soldier(0.2f, 0.12f, Coat, Trousers, Hat, HatHeight, HatTop, Belts, false);
			const FLinearColor Red = Srgb(200, 16, 46), White = Srgb(245, 245, 240);
			const float Xs[] = { -0.64f, -0.34f, -0.24f, 0.31f };
			const float Zs[] = { 1.8f, 2.08f, 2.18f, 2.58f };
			for (int32 i = 0; i < 3; ++i)
			{
				for (int32 j = 0; j < 3; ++j)
				{
					const FLinearColor& C = (i == 1 || j == 1) ? White : Red;
					const float W0 = 0.05f * FMath::Sin(Xs[i] * 6.f), W1 = 0.05f * FMath::Sin(Xs[i + 1] * 6.f);
					W.Quad(FVector3f(Xs[i], W0, Zs[j]), FVector3f(Xs[i + 1], W1, Zs[j]), FVector3f(Xs[i + 1], W1, Zs[j + 1]), FVector3f(Xs[i], W0, Zs[j + 1]), C, FVector3f(0.f, -1.f, 2.f));
					W.Quad(FVector3f(Xs[i], W0 + 0.01f, Zs[j]), FVector3f(Xs[i + 1], W1 + 0.01f, Zs[j]), FVector3f(Xs[i + 1], W1 + 0.01f, Zs[j + 1]), FVector3f(Xs[i], W0 + 0.01f, Zs[j + 1]), C, FVector3f(0.f, 1.f, 2.f));
				}
			}
			break;
		}
		case ESitePiece::FormationCavalry:
		{
			// Two ranks of six riders in light blue with helmets; bays, blacks and chestnuts; the guidon ahead.
			const FLinearColor Bay = Srgb(110, 70, 42), Black = Srgb(40, 32, 28), Chestnut = Srgb(140, 84, 46), Dark = Srgb(46, 32, 22);
			const FLinearColor Coat = Srgb(118, 146, 188), Trousers = Srgb(40, 44, 70), Helmet = Srgb(210, 206, 196);
			for (int32 Rank = 0; Rank < 2; ++Rank)
			{
				for (int32 File = 0; File < 6; ++File)
				{
					const float X = -Rank * 1.5f, Y = (File - 2.5f) * 0.55f;
					W.Horse(X, Y, File % 3 == 1 ? Black : File % 3 == 2 ? Chestnut : Bay, Dark, (File + Rank) % 2 ? 0.06f : -0.06f);
					W.Rider(X - 0.05f, Y, Coat, Trousers, Helmet, 0.16f);
				}
			}
			W.Frustum(FVector3f(1.0f, 0.f, 0.f), FVector3f(1.0f, 0.f, 2.6f), 0.025f, 0.02f, Srgb(120, 90, 60), 6);
			W.Quad(FVector3f(1.0f, 0.f, 2.1f), FVector3f(1.55f, 0.03f, 2.3f), FVector3f(1.0f, 0.f, 2.55f), FVector3f(1.0f, 0.f, 2.1f), Srgb(200, 16, 46), FVector3f(1.f, -1.f, 2.f));
			break;
		}
		case ESitePiece::FormationArtillery:
		case ESitePiece::FormationHorseArtillery:
		{
			// Two guns in battery, each with its limber and team behind (four horses; six for horse artillery),
			// the crew about the gun, or mounted alongside the team.
			const bool bRiding = Piece == ESitePiece::FormationHorseArtillery;
			const FLinearColor Wood = Srgb(70, 78, 52), Barrel = Srgb(62, 58, 52), Bay = Srgb(110, 70, 42), Dark = Srgb(46, 32, 22);
			const FLinearColor Coat = Srgb(34, 44, 86), Trousers = Srgb(122, 150, 190), Hat = Srgb(30, 30, 36), Belts = Srgb(236, 232, 220);
			for (float Y : { -1.1f, 1.1f })
			{
				W.FieldGun(0.3f, Y, Wood, Barrel);
				// Limber: its chest on two wheels, the pole to the team.
				W.Box(FVector3f(-2.2f, Y - 0.32f, 0.45f), FVector3f(-1.6f, Y + 0.32f, 0.8f), Wood);
				for (float S : { -0.36f, 0.36f })
				{
					W.Frustum(FVector3f(-1.9f, Y + S - 0.04f, 0.33f), FVector3f(-1.9f, Y + S + 0.04f, 0.33f), 0.33f, 0.33f, Wood, 10);
				}
				W.Frustum(FVector3f(-2.2f, Y, 0.55f), FVector3f(-4.6f, Y, 0.6f), 0.025f, 0.025f, Wood, 4);
				for (int32 H = 0; H < (bRiding ? 3 : 2); ++H)
				{
					for (float S : { -0.22f, 0.22f })
					{
						W.Horse(-2.5f - H * 1.05f, Y + S, H % 2 ? Dark : Bay, Dark);
					}
					if (!bRiding || H == 0)
					{
						W.Rider(-2.55f - H * 1.05f, Y - 0.22f, Coat, Trousers, Hat, 0.14f, false);
					}
				}
				if (bRiding)
				{
					for (float RX : { -2.6f, -3.6f })
					{
						const float RY = Y + (Y > 0.f ? 0.8f : -0.8f);
						W.Horse(RX, RY, Bay, Dark);
						W.Rider(RX - 0.05f, RY, Coat, Trousers, Hat, 0.14f, false);
					}
				}
				else
				{
					// The crew: loader at the muzzle with the rammer, gunner at the trail, a third man.
					W.Soldier(1.1f, Y + 0.45f, Coat, Trousers, Hat, 0.16f, 0.09f, Belts, false);
					W.Soldier(-0.8f, Y + 0.4f, Coat, Trousers, Hat, 0.16f, 0.09f, Belts, false);
					W.Soldier(0.2f, Y - 0.55f, Coat, Trousers, Hat, 0.16f, 0.09f, Belts, false);
					W.Frustum(FVector3f(1.2f, Y + 0.6f, 0.7f), FVector3f(1.9f, Y + 0.35f, 1.2f), 0.015f, 0.015f, Wood, 4);
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
		// ---- Field fortifications (skanser): the front towards +X.
		case ESitePiece::RedoubtSmall:
		case ESitePiece::RedoubtLarge:
		{
			const bool bLarge = Piece == ESitePiece::RedoubtLarge;
			const FLinearColor Turf = Srgb(104, 118, 62), TurfTop = Srgb(122, 134, 70), Earth = Srgb(120, 98, 66), Ditch = Srgb(64, 56, 42), Plank = Srgb(150, 122, 88);
			const TArray<FVector2f> Outline = bLarge
				? TArray<FVector2f>{ FVector2f(14.f, 0.f), FVector2f(4.f, -14.f), FVector2f(-12.f, -14.f), FVector2f(-12.f, 14.f), FVector2f(4.f, 14.f), FVector2f(14.f, 0.f) }
				: TArray<FVector2f>{ FVector2f(-8.f, -9.f), FVector2f(-4.f, -9.f), FVector2f(8.f, 0.f), FVector2f(-4.f, 9.f), FVector2f(-8.f, 9.f) };
			const FVector2f Centre = bLarge ? FVector2f(0.f, 0.f) : FVector2f(-2.f, 0.f);
			const float H = bLarge ? 2.8f : 2.2f, Foot = 2.2f, Crest = 0.8f, DitchW = 3.2f;
			// The parade inside, trodden earth.
			if (bLarge)
			{
				W.Box(FVector3f(-11.f, -13.f, 0.f), FVector3f(3.f, 13.f, 0.12f), Earth);
				W.Box(FVector3f(3.f, -8.f, 0.f), FVector3f(9.f, 8.f, 0.12f), Earth);
			}
			else
			{
				W.Box(FVector3f(-8.f, -7.5f, 0.f), FVector3f(2.f, 7.5f, 0.12f), Earth);
			}
			for (int32 s = 0; s + 1 < Outline.Num(); ++s)
			{
				const FVector2f A = Outline[s], B = Outline[s + 1];
				const FVector2f Along = (B - A).GetSafeNormal();
				FVector2f Out(Along.Y, -Along.X);
				if (FVector2f::DotProduct(Out, (A + B) * 0.5f - Centre) < 0.f)
				{
					Out = -Out;
				}
				// Stretch a little along the face so the corners close.
				const FVector2f A2 = A - Along * 1.2f, B2 = B + Along * 1.2f;
				auto P = [](const FVector2f& V, float Z) { return FVector3f(V.X, V.Y, Z); };
				const FVector3f Below = P((A + B) * 0.5f, -2.f);
				// Parapet: outer slope, crest, inner slope (the gun side), then the ditch in front.
				W.Quad(P(A2 + Out * Foot, 0.f), P(B2 + Out * Foot, 0.f), P(B2 + Out * Crest, H), P(A2 + Out * Crest, H), Turf, Below);
				W.Quad(P(A2 + Out * Crest, H), P(B2 + Out * Crest, H), P(B2 - Out * Crest, H), P(A2 - Out * Crest, H), TurfTop, Below);
				W.Quad(P(A2 - Out * Crest, H), P(B2 - Out * Crest, H), P(B2 - Out * Foot, 0.f), P(A2 - Out * Foot, 0.f), Turf, Below);
				W.Quad(P(A2 + Out * Foot, 0.04f), P(B2 + Out * Foot, 0.04f), P(B2 + Out * (Foot + DitchW), 0.04f), P(A2 + Out * (Foot + DitchW), 0.04f), Ditch, Below);
			}
			// The earth foot: from the outer edge of the ditch down and out to the terrain (the fort stands
			// on the highest ground under it), all round, across the open rear of a lunette too.
			TArray<FVector2f> Ring = Outline;
			if (!bLarge)
			{
				Ring.Add(Outline[0]);
			}
			for (int32 s = 0; s + 1 < Ring.Num(); ++s)
			{
				const FVector2f A = Ring[s], B = Ring[s + 1];
				const FVector2f Along = (B - A).GetSafeNormal();
				FVector2f Out(Along.Y, -Along.X);
				if (FVector2f::DotProduct(Out, (A + B) * 0.5f - Centre) < 0.f)
				{
					Out = -Out;
				}
				const bool bRear = !bLarge && s == Ring.Num() - 2;
				const float Edge = bRear ? 0.5f : Foot + DitchW, Drop = 7.f;
				const FVector2f A2 = A - Along * (Edge + 1.2f), B2 = B + Along * (Edge + 1.2f);
				const FVector3f Below((A.X + B.X) * 0.5f, (A.Y + B.Y) * 0.5f, -20.f);
				W.Quad(FVector3f(A2.X + Out.X * Edge, A2.Y + Out.Y * Edge, 0.05f), FVector3f(B2.X + Out.X * Edge, B2.Y + Out.Y * Edge, 0.05f),
					FVector3f(B2.X + Out.X * (Edge + Drop), B2.Y + Out.Y * (Edge + Drop), -Drop), FVector3f(A2.X + Out.X * (Edge + Drop), A2.Y + Out.Y * (Edge + Drop), -Drop), Srgb(112, 124, 64), Below);
				if (bRear)
				{
					// The open gorge: the parade runs out to the foot.
					W.Quad(FVector3f(A.X, A.Y, 0.1f), FVector3f(B.X, B.Y, 0.1f), FVector3f(B.X + Out.X * Edge, B.Y + Out.Y * Edge, 0.05f), FVector3f(A.X + Out.X * Edge, A.Y + Out.Y * Edge, 0.05f), Earth, Below);
				}
			}
			// Gun platforms of planks behind the parapet (the guns stand on them as they come).
			TArray<FVector2f> Slots;
			TArray<float> Yaws;
			Campaign1851Forts::GunSlots(bLarge, Slots, Yaws);
			for (const FVector2f& S : Slots)
			{
				W.Box(FVector3f(S.X - 1.1f, S.Y - 1.1f, 0.f), FVector3f(S.X + 1.1f, S.Y + 1.1f, 0.25f), Plank);
			}
			// A powder niche and the gate path at the rear.
			W.Box(FVector3f(Centre.X - 2.f, -0.6f, 0.f), FVector3f(Centre.X - 0.8f, 0.6f, 0.9f), Earth);
			break;
		}
		case ESitePiece::FortGun:
		{
			const FLinearColor Carriage = Srgb(96, 72, 48), Iron = Srgb(40, 42, 46);
			W.Box(FVector3f(-0.9f, -0.35f, 0.25f), FVector3f(0.5f, 0.35f, 0.75f), Carriage);
			for (float Y : { -0.45f, 0.45f })
			{
				W.Box(FVector3f(-0.3f, Y - 0.08f, 0.25f), FVector3f(0.3f, Y + 0.08f, 0.9f), Carriage);   // trunnion wheels
			}
			W.Box(FVector3f(-0.5f, -0.15f, 0.8f), FVector3f(1.7f, 0.15f, 1.08f), Iron);                    // the barrel over the parapet
			break;
		}
		case ESitePiece::PalisadeSmall:
		case ESitePiece::PalisadeLarge:
		{
			const bool bLarge = Piece == ESitePiece::PalisadeLarge;
			const FLinearColor Stake = Srgb(110, 84, 56);
			const TArray<FVector2f> Outline = bLarge
				? TArray<FVector2f>{ FVector2f(14.f, 0.f), FVector2f(4.f, -14.f), FVector2f(-12.f, -14.f), FVector2f(-12.f, 14.f), FVector2f(4.f, 14.f), FVector2f(14.f, 0.f) }
				: TArray<FVector2f>{ FVector2f(-8.f, -9.f), FVector2f(-4.f, -9.f), FVector2f(8.f, 0.f), FVector2f(-4.f, 9.f), FVector2f(-8.f, 9.f), FVector2f(-8.f, 2.f) };
			const FVector2f Centre = bLarge ? FVector2f(0.f, 0.f) : FVector2f(-2.f, 0.f);
			for (int32 s = 0; s + 1 < Outline.Num(); ++s)
			{
				const FVector2f A = Outline[s], B = Outline[s + 1];
				const FVector2f Along = (B - A).GetSafeNormal();
				FVector2f Out(Along.Y, -Along.X);
				if (FVector2f::DotProduct(Out, (A + B) * 0.5f - Centre) < 0.f)
				{
					Out = -Out;
				}
				// Stakes in the middle of the ditch (the small fort's rear: across the gorge, a gap for the gate).
				const bool bGorge = !bLarge && s == Outline.Num() - 2;
				const float Offset = bGorge ? -1.f : 3.8f;
				const float Len = (B - A).Size();
				for (float T = 0.f; T <= Len; T += 0.7f)
				{
					const FVector2f Pt = A + Along * T + Out * Offset;
					W.Box(FVector3f(Pt.X - 0.09f, Pt.Y - 0.09f, 0.f), FVector3f(Pt.X + 0.09f, Pt.Y + 0.09f, 1.9f), Stake);
				}
			}
			break;
		}
		case ESitePiece::Blockhouse:
		{
			// Heavy timbers under a thick earth cover, loopholes all round.
			const FLinearColor Log = Srgb(112, 86, 58), EarthCover = Srgb(98, 112, 60);
			W.Box(FVector3f(-3.f, -1.8f, 0.f), FVector3f(3.f, 1.8f, 1.7f), Log);
			W.House(FVector2f(0.f, 0.f), 6.8f, 4.4f, 1.7f, 2.6f, Log, EarthCover);
			const FVector3f In(0.f, 0.f, 0.8f);
			for (float X : { -2.f, -0.7f, 0.7f, 2.f })
			{
				for (float Side : { -1.f, 1.f })
				{
					W.Quad(FVector3f(X - 0.2f, Side * 1.82f, 0.9f), FVector3f(X + 0.2f, Side * 1.82f, 0.9f), FVector3f(X + 0.2f, Side * 1.82f, 1.1f), FVector3f(X - 0.2f, Side * 1.82f, 1.1f), Window, In);
				}
			}
			break;
		}
		case ESitePiece::Traverse:
		{
			// An earth mound between two guns, running from the parapet inwards (-X).
			const FLinearColor Turf = Srgb(104, 118, 62);
			const FVector3f Below(-1.5f, 0.f, -2.f);
			W.Quad(FVector3f(0.5f, -0.9f, 0.f), FVector3f(-3.f, -0.9f, 0.f), FVector3f(-3.f, -0.2f, 1.9f), FVector3f(0.5f, -0.2f, 1.9f), Turf, Below);
			W.Quad(FVector3f(0.5f, 0.9f, 0.f), FVector3f(-3.f, 0.9f, 0.f), FVector3f(-3.f, 0.2f, 1.9f), FVector3f(0.5f, 0.2f, 1.9f), Turf, Below);
			W.Quad(FVector3f(0.5f, -0.2f, 1.9f), FVector3f(-3.f, -0.2f, 1.9f), FVector3f(-3.f, 0.2f, 1.9f), FVector3f(0.5f, 0.2f, 1.9f), Turf, Below);
			W.Quad(FVector3f(-3.f, -0.9f, 0.f), FVector3f(-3.f, 0.9f, 0.f), FVector3f(-3.f, 0.2f, 1.9f), FVector3f(-3.f, -0.2f, 1.9f), Turf, Below);
			break;
		}
		case ESitePiece::TrenchSegment:
		{
			const FLinearColor TrenchFloor = Srgb(78, 64, 46), Lip = Srgb(112, 118, 66);
			const FVector3f Below(0.f, 0.f, -3.f);
			W.Quad(FVector3f(-5.f, -0.5f, 0.06f), FVector3f(5.f, -0.5f, 0.06f), FVector3f(5.f, 0.5f, 0.06f), FVector3f(-5.f, 0.5f, 0.06f), TrenchFloor, Below);
			W.Quad(FVector3f(-5.f, -0.5f, 0.f), FVector3f(5.f, -0.5f, 0.f), FVector3f(5.f, -1.f, 0.7f), FVector3f(-5.f, -1.f, 0.7f), Lip, Below);
			W.Quad(FVector3f(-5.f, -1.f, 0.7f), FVector3f(5.f, -1.f, 0.7f), FVector3f(5.f, -1.8f, 0.f), FVector3f(-5.f, -1.8f, 0.f), Lip, Below);
			break;
		}
		// ---- Civil town buildings (each on its own plot; the long side along X, the front on +Y).
		case ESitePiece::School:
		{
			const FLinearColor White = Srgb(232, 226, 210);
			W.House(FVector2f(0.f, 0.f), 10.f, 5.f, 3.4f, 6.4f, White, Tile);
			W.Facade(FVector2f(0.f, 0.f), 10.f, 5.f, 3.4f, 1, Window, Srgb(70, 90, 60));
			W.Box(FVector3f(-0.5f, -0.5f, 6.f), FVector3f(0.5f, 0.5f, 7.4f), White);          // bell turret
			W.House(FVector2f(0.f, 0.f), 1.3f, 1.3f, 7.4f, 8.4f, White, RoofSlate);
			W.Box(FVector3f(-5.6f, 3.2f, 0.f), FVector3f(5.6f, 3.35f, 1.1f), DarkTimber);      // schoolyard fence
			break;
		}
		case ESitePiece::TownHall:
		{
			const FLinearColor Yellow = Srgb(214, 180, 112);
			W.House(FVector2f(0.f, 0.f), 16.f, 7.f, 7.f, 11.f, Yellow, Tile);
			W.Facade(FVector2f(0.f, 0.f), 16.f, 7.f, 7.f, 2, Window, Srgb(90, 56, 40));
			W.Box(FVector3f(-8.2f, -3.7f, 0.f), FVector3f(8.2f, 3.7f, 0.6f), Stone);
			W.Box(FVector3f(-1.4f, 3.5f, 0.f), FVector3f(1.4f, 4.6f, 1.2f), Stone);          // front steps
			W.Box(FVector3f(-1.3f, -1.3f, 9.f), FVector3f(1.3f, 1.3f, 14.f), Yellow);          // clock tower
			W.Box(FVector3f(-0.7f, 1.32f, 12.f), FVector3f(0.7f, 1.36f, 13.4f), Srgb(236, 232, 220));   // clock face
			W.House(FVector2f(0.f, 0.f), 2.8f, 2.8f, 14.f, 17.f, Yellow, Srgb(80, 120, 100));   // copper spire
			break;
		}
		case ESitePiece::PostOffice:
		{
			const FLinearColor Red = Srgb(176, 64, 48);
			W.House(FVector2f(0.f, 0.f), 9.f, 5.f, 5.4f, 8.6f, Plaster, RoofSlate);
			W.Facade(FVector2f(0.f, 0.f), 9.f, 5.f, 5.4f, 2, Window, Red);
			W.Box(FVector3f(-3.9f, 2.52f, 4.7f), FVector3f(3.9f, 2.6f, 5.2f), Red);            // the red post sign band
			W.Box(FVector3f(-6.5f, -2.f, 0.f), FVector3f(-4.8f, 2.f, 2.8f), Srgb(150, 120, 92)); // coach shed
			break;
		}
		case ESitePiece::Hospital:
		{
			const FLinearColor White = Srgb(230, 224, 208);
			W.House(FVector2f(0.f, -1.5f), 20.f, 7.f, 8.f, 12.f, White, RoofSlate);
			W.Facade(FVector2f(0.f, -1.5f), 20.f, 7.f, 8.f, 3, Window, Srgb(80, 70, 60));
			for (float X : { -8.f, 8.f })   // wings towards the front
			{
				W.House(FVector2f(X, 4.f), 4.f, 5.f, 7.f, 10.f, White, RoofSlate, true);
			}
			W.Box(FVector3f(-10.4f, -5.4f, 0.f), FVector3f(10.4f, 2.4f, 0.6f), Stone);
			break;
		}
		case ESitePiece::CustomsHouse:
		{
			W.House(FVector2f(0.f, 0.f), 10.f, 6.f, 6.f, 9.6f, Srgb(206, 164, 96), Tile);
			W.Facade(FVector2f(0.f, 0.f), 10.f, 6.f, 6.f, 2, Window, Srgb(60, 70, 90));
			W.Box(FVector3f(-6.f, 3.4f, 0.f), FVector3f(6.f, 6.f, 0.5f), Stone);               // quay
			W.Box(FVector3f(4.2f, 4.6f, 0.f), FVector3f(4.4f, 4.8f, 8.f), DarkTimber);          // flagstaff
			break;
		}
		case ESitePiece::Lighthouse:
		{
			const FLinearColor White = Srgb(236, 232, 222), Lamp = Srgb(250, 226, 150);
			for (int32 Step = 0; Step < 5; ++Step)   // a tapering white tower
			{
				const float R = 2.2f - Step * 0.3f, Z0 = Step * 3.2f;
				W.Box(FVector3f(-R, -R, Z0), FVector3f(R, R, Z0 + 3.2f), Step == 2 ? Srgb(176, 50, 40) : White);
			}
			W.Box(FVector3f(-1.1f, -1.1f, 16.f), FVector3f(1.1f, 1.1f, 17.6f), Lamp);
			W.House(FVector2f(0.f, 0.f), 2.4f, 2.4f, 17.6f, 19.f, White, Srgb(40, 40, 44));
			W.House(FVector2f(0.f, 4.5f), 6.f, 3.6f, 2.8f, 5.f, White, Tile);                  // keeper's house
			break;
		}
		case ESitePiece::MerchantYard:
		{
			// Four ranges round a yard: the house on the street (+Y), warehouses behind.
			W.House(FVector2f(0.f, 4.f), 14.f, 5.f, 5.6f, 9.f, Plaster, Tile);
			W.Facade(FVector2f(0.f, 4.f), 14.f, 5.f, 5.6f, 2, Window, Srgb(100, 60, 40));
			W.House(FVector2f(0.f, -4.f), 14.f, 4.f, 4.4f, 7.4f, Srgb(150, 120, 92), Tile);
			W.House(FVector2f(-6.f, 0.f), 4.f, 3.f, 3.6f, 6.f, Srgb(150, 120, 92), Tile, true);
			W.House(FVector2f(6.f, 0.f), 4.f, 3.f, 3.6f, 6.f, Srgb(150, 120, 92), Tile, true);
			break;
		}
		case ESitePiece::Brewery:
		{
			W.House(FVector2f(-2.f, 0.f), 12.f, 6.f, 6.f, 9.4f, Brick, Tile);
			W.Facade(FVector2f(-2.f, 0.f), 12.f, 6.f, 6.f, 2, Window, Srgb(70, 50, 36));
			W.House(FVector2f(6.f, 0.f), 4.f, 4.f, 8.f, 11.f, Brick, RoofSlate);               // malt kiln
			W.Box(FVector3f(6.6f, -0.5f, 10.f), FVector3f(7.4f, 0.5f, 13.f), Brick);
			W.Box(FVector3f(-7.f, -4.f, 0.f), FVector3f(-4.f, -3.2f, 1.6f), DarkTimber);        // barrels
			break;
		}
		case ESitePiece::Brickworks:
		{
			W.House(FVector2f(-3.f, 0.f), 8.f, 6.f, 3.2f, 5.4f, Brick, Tile);                  // ring kiln
			W.Box(FVector3f(-3.6f, -0.6f, 0.f), FVector3f(-2.4f, 0.6f, 16.f), Brick);           // tall chimney
			for (float Y : { -5.f, 5.f })   // open drying sheds
			{
				W.House(FVector2f(5.f, Y), 12.f, 3.f, 2.2f, 3.6f, Srgb(118, 100, 80), Thatch);
			}
			break;
		}
		case ESitePiece::Sawmill:
		{
			W.House(FVector2f(0.f, 0.f), 12.f, 5.f, 3.6f, 6.f, Srgb(128, 96, 64), Tile);
			for (int32 k = 0; k < 4; ++k)   // log piles
			{
				W.Box(FVector3f(-6.f + k * 3.f, 3.6f, 0.f), FVector3f(-4.f + k * 3.f, 6.f, 1.2f), Srgb(150, 112, 70));
			}
			W.Box(FVector3f(5.6f, -0.3f, 0.f), FVector3f(6.2f, 0.3f, 9.f), Brick);             // boiler stack
			break;
		}
		case ESitePiece::Workshop:
		{
			W.House(FVector2f(0.f, 0.f), 16.f, 8.f, 5.f, 8.f, Brick, RoofSlate);
			W.Facade(FVector2f(0.f, 0.f), 16.f, 8.f, 5.f, 1, Window, Srgb(60, 60, 64));
			for (float X : { -4.f, 0.f, 4.f })   // north lights
			{
				W.Box(FVector3f(X - 0.9f, -0.8f, 7.6f), FVector3f(X + 0.9f, 0.8f, 8.6f), Window);
			}
			W.Box(FVector3f(8.4f, -0.6f, 0.f), FVector3f(9.6f, 0.6f, 15.f), Brick);             // chimney
			break;
		}
		case ESitePiece::Factory:
		{
			W.House(FVector2f(0.f, 0.f), 24.f, 9.f, 11.f, 14.f, Brick, RoofSlate);             // four-storey mill
			W.Facade(FVector2f(0.f, 0.f), 24.f, 9.f, 11.f, 4, Window, Srgb(60, 50, 44));
			W.House(FVector2f(-10.f, 7.f), 5.f, 5.f, 5.f, 7.f, Brick, RoofSlate, true);          // engine house
			W.Box(FVector3f(-13.f, 5.4f, 0.f), FVector3f(-11.4f, 7.f, 22.f), Brick);             // mill chimney
			break;
		}
		case ESitePiece::Inn:
		{
			const FLinearColor White = Srgb(230, 222, 200);
			W.House(FVector2f(0.f, 0.f), 14.f, 6.f, 3.6f, 7.4f, White, Thatch);
			W.Facade(FVector2f(0.f, 0.f), 14.f, 6.f, 3.6f, 1, Window, Srgb(90, 60, 40));
			W.House(FVector2f(-8.f, -4.f), 8.f, 4.f, 2.8f, 5.4f, Srgb(150, 120, 92), Thatch, true);   // stable for the post horses
			W.Box(FVector3f(3.f, 3.2f, 0.f), FVector3f(3.2f, 3.4f, 3.4f), DarkTimber);           // sign post
			W.Box(FVector3f(3.2f, 3.2f, 2.6f), FVector3f(4.4f, 3.3f, 3.3f), Srgb(176, 150, 90));
			break;
		}
		}
		static const TCHAR* Names[] = { TEXT("Ground"), TEXT("Barracks"), TEXT("Scaffold"), TEXT("CraneMast"), TEXT("CraneJib"), TEXT("Wagon"), TEXT("Flagpole"), TEXT("Flag"),
			TEXT("Stables"), TEXT("Depot"), TEXT("Infirmary"), TEXT("Arsenal"), TEXT("Lazaret"), TEXT("Battery"), TEXT("PowderMagazine"), TEXT("StarFort"),
			TEXT("Telegraph"), TEXT("Granary"), TEXT("Train"), TEXT("Station"),
			TEXT("FormationInfantry"), TEXT("FormationGuard"), TEXT("FormationJager"), TEXT("FormationCavalry"), TEXT("FormationArtillery"), TEXT("FormationHorseArtillery"),
			TEXT("TrainEngine"), TEXT("TrainCarBrown"), TEXT("TrainCarGreen"),
			TEXT("School"), TEXT("TownHall"), TEXT("PostOffice"), TEXT("Hospital"), TEXT("CustomsHouse"), TEXT("Lighthouse"), TEXT("MerchantYard"),
			TEXT("Brewery"), TEXT("Brickworks"), TEXT("Sawmill"), TEXT("Workshop"), TEXT("Factory"), TEXT("Inn"),
			TEXT("RedoubtSmall"), TEXT("RedoubtLarge"), TEXT("FortGun"), TEXT("PalisadeSmall"), TEXT("PalisadeLarge"), TEXT("Blockhouse"), TEXT("Traverse"), TEXT("TrenchSegment") };
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
