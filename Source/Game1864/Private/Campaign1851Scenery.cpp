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
				Colours[Corners[i]] = FVector4f(Lit.R, Lit.G, Lit.B, 1.f);
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
		void Lathe(const TArray<FVector2f>& Profile, int32 Sides, const FLinearColor& Base, float Twist = 0.f)
		{
			const FVector3f In(0.f, 0.f, (Profile[0].Y + Profile.Last().Y) * 0.5f);
			auto P = [&](int32 Ring, int32 Side)
			{
				const float A = (Side + Twist * Ring) * UE_TWO_PI / Sides;
				return FVector3f(Profile[Ring].X * FMath::Cos(A), Profile[Ring].X * FMath::Sin(A), Profile[Ring].Y);
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
		switch (Piece)
		{
		case EPiece::TownHouse:
		case EPiece::TownHouseOchre:
			W.DropShadow(4.6f, 3.4f, 1.6f);
			W.House(FVector2f(0.f, 0.f), 7.f, 4.4f, 3.4f, 6.2f, Piece == EPiece::TownHouse ? Plaster : Ochre, Tile);
			break;
		case EPiece::Cottage:
			W.DropShadow(3.6f, 2.6f, 1.2f);
			W.House(FVector2f(0.f, 0.f), 5.5f, 3.4f, 2.2f, 4.8f, Plaster, Thatch);
			break;
		case EPiece::Farm:
			W.DropShadow(7.f, 7.f, 1.8f);
			W.House(FVector2f(0.f, -4.2f), 10.f, 3.4f, 2.4f, 5.f, Plaster, Thatch);
			W.House(FVector2f(0.f, 4.2f), 10.f, 3.4f, 2.4f, 5.f, Srgb(150, 120, 92), Thatch);
			W.House(FVector2f(-4.8f, 0.f), 5.f, 3.2f, 2.4f, 4.8f, Plaster, Thatch, true);
			W.House(FVector2f(4.8f, 0.f), 5.f, 3.2f, 2.4f, 4.8f, Srgb(150, 120, 92), Thatch, true);
			break;
		case EPiece::Church:
			W.DropShadow(10.f, 5.f, 3.f);
			W.House(FVector2f(1.5f, 0.f), 12.f, 5.4f, 4.8f, 8.6f, Plaster, Tile);
			W.Box(FVector3f(-7.9f, -2.2f, 0.f), FVector3f(-3.5f, 2.2f, 12.5f), Plaster);
			W.Spire(FVector2f(-5.7f, 0.f), 2.5f, 12.5f, 20.f, Slate);
			break;
		case EPiece::Broadleaf:
			W.DropShadow(4.8f, 3.8f, 2.4f);
			W.Box(FVector3f(-0.45f, -0.45f, 0.f), FVector3f(0.45f, 0.45f, 3.5f), Bark);
			W.Lathe({ { 0.f, 2.4f }, { 3.4f, 3.6f }, { 4.4f, 6.4f }, { 3.2f, 9.6f }, { 0.f, 11.f } }, 7, Beech, 0.5f);
			break;
		case EPiece::Conifer:
			W.DropShadow(3.2f, 2.6f, 2.6f);
			W.Box(FVector3f(-0.35f, -0.35f, 0.f), FVector3f(0.35f, 0.35f, 2.f), Bark);
			W.Lathe({ { 3.2f, 1.6f }, { 1.4f, 7.f }, { 2.4f, 6.2f }, { 0.f, 14.f } }, 6, Spruce);
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
		static const TCHAR* Names[] = { TEXT("TownHouse"), TEXT("TownHouseOchre"), TEXT("Cottage"), TEXT("Farm"), TEXT("Church"), TEXT("Broadleaf"), TEXT("Conifer") };
		return Names[FMath::Clamp(int32(Piece), 0, int32(EPiece::Count) - 1)];
	}

	static UStaticMesh* Finish(FWriter& Writer, UMaterialInterface* Material, const FString& MeshName)
	{
		// Unique names: several sites build the same pieces, and reusing a name would re-create a mesh
		// that is already on screen (the renderer's ray tracing geometry asserts on that).
		UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UStaticMesh::StaticClass(), FName(*MeshName)), RF_Transient);
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, TEXT("Scenery")));
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
		}
		static const TCHAR* Names[] = { TEXT("Ground"), TEXT("Barracks"), TEXT("Scaffold"), TEXT("CraneMast"), TEXT("CraneJib"), TEXT("Wagon"), TEXT("Flagpole"), TEXT("Flag"),
			TEXT("Stables"), TEXT("Depot"), TEXT("Infirmary"), TEXT("Arsenal"), TEXT("Lazaret"), TEXT("Battery"), TEXT("PowderMagazine"), TEXT("StarFort"),
			TEXT("Telegraph"), TEXT("Granary"), TEXT("Train"), TEXT("Station") };
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
