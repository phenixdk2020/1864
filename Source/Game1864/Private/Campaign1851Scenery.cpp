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
		UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), *MeshName, RF_Transient);
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
		const FLinearColor Edge = Colour * 0.68f;
		for (const TArray<FVector>& Line : Lines)
		{
			if (Line.Num() < 2)
			{
				continue;
			}
			TArray<FVector3f> L, C, R;
			for (int32 i = 0; i < Line.Num(); ++i)
			{
				// Direction from the neighbours; the side vector is horizontal.
				const FVector Dir = (Line[FMath::Min(i + 1, Line.Num() - 1)] - Line[FMath::Max(i - 1, 0)]).GetSafeNormal2D();
				const FVector Side = FVector(-Dir.Y, Dir.X, 0.0) * HalfWidth;
				C.Add(FVector3f(Line[i] + FVector(0.0, 0.0, 0.25)));
				L.Add(FVector3f(Line[i] - Side));
				R.Add(FVector3f(Line[i] + Side));
			}
			for (int32 i = 0; i + 1 < Line.Num(); ++i)
			{
				const FVector3f Below = C[i] - FVector3f(0.f, 0.f, 50.f);
				Writer.Tri(L[i], C[i], C[i + 1], Colour, Below);
				Writer.Tri(L[i], C[i + 1], L[i + 1], Edge, Below);
				Writer.Tri(C[i], R[i], R[i + 1], Colour, Below);
				Writer.Tri(C[i], R[i + 1], C[i + 1], Edge, Below);
			}
		}
		return Finish(Writer, Material, FString(MeshName));
	}
}
