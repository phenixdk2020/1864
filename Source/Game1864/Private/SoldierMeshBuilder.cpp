#include "SoldierMeshBuilder.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"

namespace
{
	/** Colour + livery slot. Slot is written to vertex alpha as Slot / 4. */
	struct FPaint
	{
		FLinearColor Colour;
		int32 Slot = 0;
	};

	// Livery slots, resolved per instance by the material.
	const FPaint Coat{ FLinearColor::Black, 1 };
	const FPaint Trousers{ FLinearColor::Black, 2 };
	const FPaint Facings{ FLinearColor::Black, 3 };
	const FPaint Piping{ FLinearColor::Black, 4 };

	// Fixed colours (linear).
	const FPaint Skin{ FLinearColor(0.42f, 0.26f, 0.18f) };
	const FPaint Leather{ FLinearColor(0.018f, 0.015f, 0.012f) };
	const FPaint Hat{ FLinearColor(0.012f, 0.012f, 0.014f) };
	const FPaint Belts{ FLinearColor(0.62f, 0.60f, 0.54f) };
	const FPaint Brass{ FLinearColor(0.45f, 0.33f, 0.12f) };
	const FPaint Pack{ FLinearColor(0.045f, 0.032f, 0.024f) };
	const FPaint Blanket{ FLinearColor(0.10f, 0.085f, 0.065f) };
	const FPaint Wood{ FLinearColor(0.11f, 0.05f, 0.022f) };
	const FPaint Steel{ FLinearColor(0.30f, 0.31f, 0.33f) };
	const FPaint Pompon{ FLinearColor(0.015f, 0.02f, 0.06f) };
	const FPaint Cockade{ FLinearColor(0.5f, 0.02f, 0.02f) };

	/** Writes flat-shaded convex parts into a mesh description. */
	class FPartWriter
	{
	public:
		FPartWriter()
			: Attributes(Mesh)
		{
			Attributes.Register();
			Attributes.GetVertexInstanceUVs().SetNumChannels(1);
			Group = Mesh.CreatePolygonGroup();
		}

		/**
		 * A box tapered from ring A to ring B. Sizes are (depth along the part's forward, width
		 * along its right). Forward is world +X unless the part itself runs along X.
		 */
		void Segment(const FVector3f& A, const FVector3f& B, const FVector2f& SizeA, const FVector2f& SizeB, const FPaint& Paint)
		{
			const FVector3f Axis = (B - A).GetSafeNormal();
			FVector3f Right = FVector3f::CrossProduct(Axis, FVector3f::ForwardVector);
			if (Right.SizeSquared() < 1e-4f)
			{
				Right = FVector3f::CrossProduct(Axis, FVector3f::UpVector);
			}
			Right.Normalize();
			const FVector3f Fwd = FVector3f::CrossProduct(Right, Axis).GetSafeNormal();

			auto Ring = [&](const FVector3f& C, const FVector2f& S, FVector3f Out[4])
			{
				const FVector3f F = Fwd * (S.X * 0.5f);
				const FVector3f R = Right * (S.Y * 0.5f);
				Out[0] = C - F - R;
				Out[1] = C + F - R;
				Out[2] = C + F + R;
				Out[3] = C - F + R;
			};
			FVector3f Lo[4], Hi[4];
			Ring(A, SizeA, Lo);
			Ring(B, SizeB, Hi);

			const FVector3f Centre = (A + B) * 0.5f;
			Quad(Lo[0], Lo[1], Lo[2], Lo[3], Centre, Paint);
			Quad(Hi[0], Hi[1], Hi[2], Hi[3], Centre, Paint);
			for (int32 i = 0; i < 4; ++i)
			{
				const int32 j = (i + 1) % 4;
				Quad(Lo[i], Lo[j], Hi[j], Hi[i], Centre, Paint);
			}
		}

		void Box(const FVector3f& Min, const FVector3f& Max, const FPaint& Paint)
		{
			const FVector2f Size(Max.X - Min.X, Max.Y - Min.Y);
			const float X = (Min.X + Max.X) * 0.5f;
			const float Y = (Min.Y + Max.Y) * 0.5f;
			Segment(FVector3f(X, Y, Min.Z), FVector3f(X, Y, Max.Z), Size, Size, Paint);
		}

		/** Mirrors a part across the body's centre plane (Y = 0). */
		void MirroredSegment(const FVector3f& A, const FVector3f& B, const FVector2f& SizeA, const FVector2f& SizeB, const FPaint& Paint)
		{
			Segment(A, B, SizeA, SizeB, Paint);
			Segment(FVector3f(A.X, -A.Y, A.Z), FVector3f(B.X, -B.Y, B.Z), SizeA, SizeB, Paint);
		}

		FMeshDescription Mesh;

	private:
		void Quad(const FVector3f& P0, const FVector3f& P1, const FVector3f& P2, const FVector3f& P3, const FVector3f& Centre, const FPaint& Paint)
		{
			Triangle(P0, P1, P2, Centre, Paint);
			Triangle(P0, P2, P3, Centre, Paint);
		}

		void Triangle(FVector3f P0, FVector3f P1, FVector3f P2, const FVector3f& Centre, const FPaint& Paint)
		{
			// Unreal's front-face normal is (P2 - P0) x (P1 - P0); wind every face away from the part centre.
			FVector3f Normal = FVector3f::CrossProduct(P2 - P0, P1 - P0);
			if (Normal.SizeSquared() < 1e-8f)
			{
				return;
			}
			if (FVector3f::DotProduct(Normal, (P0 + P1 + P2) / 3.f - Centre) < 0.f)
			{
				Swap(P1, P2);
				Normal = -Normal;
			}
			Normal.Normalize();
			const FVector3f Tangent = FVector3f::CrossProduct(Normal, FMath::Abs(Normal.Z) < 0.9f ? FVector3f::UpVector : FVector3f::ForwardVector).GetSafeNormal();
			const FVector4f Colour(Paint.Colour.R, Paint.Colour.G, Paint.Colour.B, Paint.Slot / 4.f);

			TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
			TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
			TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
			TVertexInstanceAttributesRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
			TVertexInstanceAttributesRef<FVector4f> Colours = Attributes.GetVertexInstanceColors();
			TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();

			FVertexInstanceID Corners[3];
			const FVector3f Points[3] = { P0, P1, P2 };
			for (int32 i = 0; i < 3; ++i)
			{
				const FVertexID Vertex = Mesh.CreateVertex();
				Positions[Vertex] = Points[i];
				const FVertexInstanceID Instance = Mesh.CreateVertexInstance(Vertex);
				Normals[Instance] = Normal;
				Tangents[Instance] = Tangent;
				Signs[Instance] = 1.f;
				Colours[Instance] = Colour;
				UVs.Set(Instance, 0, FVector2f::ZeroVector);
				Corners[i] = Instance;
			}
			Mesh.CreateTriangle(Group, Corners);
		}

		FStaticMeshAttributes Attributes;
		FPolygonGroupID Group;
	};

	using V = FVector3f;
	using S = FVector2f;

	/** Line infantryman at shoulder arms (musket on the left shoulder, bayonet fixed). */
	void WriteInfantryman(FPartWriter& W)
	{
		// Boots and legs. Left side is -Y.
		W.Box(V(-9.f, -14.f, 0.f), V(17.f, -4.f, 9.f), Leather);
		W.Box(V(-9.f, 4.f, 0.f), V(17.f, 14.f, 9.f), Leather);
		W.MirroredSegment(V(0.5f, 9.f, 8.f), V(0.5f, 9.5f, 92.f), S(12.f, 11.f), S(16.f, 15.f), Trousers);
		W.MirroredSegment(V(0.5f, 14.8f, 10.f), V(0.5f, 17.3f, 90.f), S(1.6f, 0.6f), S(1.6f, 0.6f), Piping);

		// Coat: skirt, torso, shoulders, collar.
		W.Segment(V(0.f, 0.f, 80.f), V(0.f, 0.f, 100.f), S(26.f, 40.f), S(23.f, 37.f), Coat);
		W.Segment(V(0.f, 0.f, 100.f), V(0.f, 0.f, 143.f), S(23.f, 37.f), S(24.f, 42.f), Coat);
		W.Segment(V(0.f, 0.f, 143.f), V(0.f, 0.f, 148.f), S(24.f, 42.f), S(18.f, 30.f), Coat);
		W.Segment(V(0.5f, 0.f, 146.f), V(0.5f, 0.f, 153.f), S(15.f, 16.f), S(14.f, 15.f), Facings);

		// Waist belt, white cross belts with brass plate, cartridge box.
		W.Segment(V(0.f, 0.f, 98.f), V(0.f, 0.f, 103.f), S(24.5f, 38.5f), S(24.5f, 38.5f), Leather);
		W.Segment(V(12.4f, -17.f, 142.f), V(12.4f, 14.f, 101.f), S(0.8f, 5.f), S(0.8f, 5.f), Belts);
		W.Segment(V(12.4f, 17.f, 142.f), V(12.4f, -14.f, 101.f), S(0.8f, 5.f), S(0.8f, 5.f), Belts);
		W.Box(V(12.5f, -2.5f, 117.f), V(13.5f, 2.5f, 122.f), Brass);
		W.Box(V(11.5f, -8.f, 98.f), V(15.f, 8.f, 107.f), Leather);
		W.Box(V(15.f, -2.5f, 101.f), V(15.5f, 2.5f, 104.f), Brass);

		// Knapsack with rolled blanket.
		W.Box(V(-25.f, -15.f, 108.f), V(-12.f, 15.f, 140.f), Pack);
		W.Segment(V(-18.5f, -17.f, 144.f), V(-18.5f, 17.f, 144.f), S(9.f, 8.f), S(9.f, 8.f), Blanket);

		// Head and shako.
		W.Segment(V(1.f, 0.f, 150.f), V(1.f, 0.f, 157.f), S(10.f, 10.f), S(10.f, 10.f), Skin);
		W.Segment(V(1.5f, 0.f, 156.f), V(1.5f, 0.f, 177.f), S(19.f, 16.f), S(18.f, 15.f), Skin);
		W.Segment(V(1.f, 0.f, 172.f), V(1.f, 0.f, 194.f), S(21.f, 19.f), S(23.f, 21.f), Hat);
		W.Box(V(10.f, -9.f, 171.f), V(17.f, 9.f, 173.5f), Hat);
		W.Box(V(-1.5f, -2.5f, 194.f), V(3.5f, 2.5f, 200.f), Pompon);
		W.Box(V(11.8f, -2.5f, 183.f), V(13.2f, 2.5f, 188.f), Cockade);

		// Right arm hangs; cuff in facing colour.
		W.Segment(V(0.f, 22.f, 143.f), V(1.f, 24.f, 117.f), S(10.f, 10.f), S(9.f, 9.f), Coat);
		W.Segment(V(1.f, 24.f, 117.f), V(3.f, 24.f, 95.f), S(9.f, 9.f), S(8.f, 8.f), Coat);
		W.Segment(V(3.f, 24.f, 98.f), V(3.3f, 24.f, 92.f), S(9.4f, 9.4f), S(9.4f, 9.4f), Facings);
		W.Segment(V(3.3f, 24.f, 92.f), V(3.5f, 24.f, 83.f), S(8.f, 6.f), S(7.f, 5.f), Skin);

		// Left arm holds the musket butt; musket rests against the left shoulder.
		W.Segment(V(0.f, -22.f, 143.f), V(0.f, -25.f, 120.f), S(10.f, 10.f), S(9.f, 9.f), Coat);
		W.Segment(V(0.f, -25.f, 120.f), V(4.f, -27.f, 106.f), S(9.f, 9.f), S(8.f, 8.f), Coat);
		W.Segment(V(4.f, -27.f, 107.f), V(5.f, -27.f, 100.f), S(8.f, 6.f), S(7.f, 5.f), Skin);
		W.Segment(V(4.f, -27.f, 96.f), V(4.f, -27.f, 150.f), S(5.f, 3.5f), S(3.5f, 3.f), Wood);
		W.Segment(V(4.f, -27.f, 150.f), V(4.f, -27.f, 205.f), S(3.f, 2.6f), S(2.4f, 2.2f), Wood);
		W.Segment(V(4.f, -27.f, 205.f), V(4.f, -27.f, 213.f), S(2.2f, 2.2f), S(2.2f, 2.2f), Steel);
		W.Segment(V(4.f, -27.f, 213.f), V(4.f, -27.f, 238.f), S(1.4f, 1.2f), S(0.4f, 0.4f), Steel);
	}

	UStaticMesh* BuildMesh(UMaterialInterface* Material)
	{
		FPartWriter Writer;
		WriteInfantryman(Writer);

		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UStaticMesh::StaticClass(), TEXT("SM_Infantry_Crowd"));
		UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), Name, RF_Transient);
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, TEXT("Soldier")));

		UStaticMesh::FBuildMeshDescriptionsParams Params;
		Params.bBuildSimpleCollision = true;
		Params.bFastBuild = true;
		Params.bMarkPackageDirty = false;
		Params.bCommitMeshDescription = false;
		Mesh->BuildFromMeshDescriptions({ &Writer.Mesh }, Params);
		return Mesh;
	}
}

UStaticMesh* Game1864::SoldierMesh::GetInfantry(UMaterialInterface* Material)
{
	// One shared mesh; it stays alive while any component references it.
	static TWeakObjectPtr<UStaticMesh> Cached;
	UStaticMesh* Mesh = Cached.Get();
	if (!Mesh)
	{
		Mesh = BuildMesh(Material);
		Cached = Mesh;
	}
	else if (Mesh->GetMaterial(0) != Material)
	{
		Mesh->SetMaterial(0, Material);
	}
	return Mesh;
}
