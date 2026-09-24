#include "TheFinalTake/Editor/FTContentCommandlet.h"

#include "The_Final_Take.h"

#if WITH_EDITOR
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "IAssetTools.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "Sound/SoundWave.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	/** Flat-shaded triangle soup builder; every triangle is oriented away from an inside point. */
	struct FShapeBuilder
	{
		TArray<FVector3f> P;
		TArray<FVector3f> N;
		bool bSmoothUp = false;

		void Tri(FVector3f A, FVector3f B, FVector3f C, const FVector3f& Inside)
		{
			FVector3f Normal = FVector3f::CrossProduct(C - A, B - A);
			const FVector3f Centroid = (A + B + C) / 3.f;
			if (FVector3f::DotProduct(Normal, Centroid - Inside) < 0.f)
			{
				Swap(B, C);
				Normal = FVector3f::CrossProduct(C - A, B - A);
			}
			Normal = Normal.GetSafeNormal();
			if (bSmoothUp)
			{
				Normal = FVector3f(0.f, 0.f, 1.f);
			}
			P.Append({ A, B, C });
			N.Append({ Normal, Normal, Normal });
		}

		void Quad(const FVector3f& A, const FVector3f& B, const FVector3f& C, const FVector3f& D, const FVector3f& Inside)
		{
			Tri(A, B, C, Inside);
			Tri(A, C, D, Inside);
		}
	};

	FVector3f V(float X, float Y, float Z) { return FVector3f(X, Y, Z); }

	void BuildBox(FShapeBuilder& B, float Chamfer)
	{
		const float H = 50.f, I = 50.f - Chamfer;
		const FVector3f O = FVector3f::ZeroVector;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			for (float S : { -1.f, 1.f })
			{
				auto Pt = [&](float U, float W)
				{
					FVector3f R;
					R[Axis] = S * H;
					R[(Axis + 1) % 3] = U;
					R[(Axis + 2) % 3] = W;
					return R;
				};
				B.Quad(Pt(-I, -I), Pt(I, -I), Pt(I, I), Pt(-I, I), O);
			}
		}
		if (Chamfer <= 0.f)
		{
			return;
		}
		for (int32 K = 0; K < 3; ++K)
		{
			const int32 A = (K + 1) % 3, Bx = (K + 2) % 3;
			for (float SA : { -1.f, 1.f })
			{
				for (float SB : { -1.f, 1.f })
				{
					auto Pt = [&](float VA, float VB, float VK)
					{
						FVector3f R;
						R[A] = VA;
						R[Bx] = VB;
						R[K] = VK;
						return R;
					};
					B.Quad(Pt(SA * H, SB * I, -I), Pt(SA * H, SB * I, I), Pt(SA * I, SB * H, I), Pt(SA * I, SB * H, -I), O);
				}
			}
		}
		for (float SX : { -1.f, 1.f })
		{
			for (float SY : { -1.f, 1.f })
			{
				for (float SZ : { -1.f, 1.f })
				{
					B.Tri(V(SX * H, SY * I, SZ * I), V(SX * I, SY * H, SZ * I), V(SX * I, SY * I, SZ * H), O);
				}
			}
		}
	}

	void BuildIcosphere(FShapeBuilder& B, int32 Subdiv)
	{
		const float T = (1.f + FMath::Sqrt(5.f)) * 0.5f;
		TArray<FVector3f> Vs = {
			V(-1, T, 0), V(1, T, 0), V(-1, -T, 0), V(1, -T, 0), V(0, -1, T), V(0, 1, T),
			V(0, -1, -T), V(0, 1, -T), V(T, 0, -1), V(T, 0, 1), V(-T, 0, -1), V(-T, 0, 1) };
		TArray<FIntVector> F = {
			FIntVector(0, 11, 5), FIntVector(0, 5, 1), FIntVector(0, 1, 7), FIntVector(0, 7, 10), FIntVector(0, 10, 11),
			FIntVector(1, 5, 9), FIntVector(5, 11, 4), FIntVector(11, 10, 2), FIntVector(10, 7, 6), FIntVector(7, 1, 8),
			FIntVector(3, 9, 4), FIntVector(3, 4, 2), FIntVector(3, 2, 6), FIntVector(3, 6, 8), FIntVector(3, 8, 9),
			FIntVector(4, 9, 5), FIntVector(2, 4, 11), FIntVector(6, 2, 10), FIntVector(8, 6, 7), FIntVector(9, 8, 1) };
		for (FVector3f& X : Vs)
		{
			X = X.GetSafeNormal();
		}
		for (int32 s = 0; s < Subdiv; ++s)
		{
			TMap<uint64, int32> Mid;
			auto Middle = [&](int32 A, int32 C)
			{
				const uint64 Key = (uint64)FMath::Min(A, C) << 32 | (uint64)FMath::Max(A, C);
				if (const int32* Found = Mid.Find(Key))
				{
					return *Found;
				}
				const int32 Index = Vs.Add(((Vs[A] + Vs[C]) * 0.5f).GetSafeNormal());
				Mid.Add(Key, Index);
				return Index;
			};
			TArray<FIntVector> NF;
			for (const FIntVector& Fc : F)
			{
				const int32 A = Middle(Fc.X, Fc.Y), Bm = Middle(Fc.Y, Fc.Z), C = Middle(Fc.Z, Fc.X);
				NF.Append({ FIntVector(Fc.X, A, C), FIntVector(Fc.Y, Bm, A), FIntVector(Fc.Z, C, Bm), FIntVector(A, Bm, C) });
			}
			F = NF;
		}
		for (const FIntVector& Fc : F)
		{
			B.Tri(Vs[Fc.X] * 50.f, Vs[Fc.Y] * 50.f, Vs[Fc.Z] * 50.f, FVector3f::ZeroVector);
		}
	}

	/** Lathe around Z from a (radius, z) profile listed bottom to top. */
	void BuildLathe(FShapeBuilder& B, const TArray<FVector2f>& Profile, int32 Sides)
	{
		for (int32 i = 0; i + 1 < Profile.Num(); ++i)
		{
			const FVector2f P0 = Profile[i], P1 = Profile[i + 1];
			const FVector3f Inside(0.f, 0.f, (P0.Y + P1.Y) * 0.5f);
			for (int32 s = 0; s < Sides; ++s)
			{
				const float A0 = 2.f * PI * s / Sides, A1 = 2.f * PI * (s + 1) / Sides;
				const FVector3f Q0(FMath::Cos(A0) * P0.X, FMath::Sin(A0) * P0.X, P0.Y);
				const FVector3f Q1(FMath::Cos(A1) * P0.X, FMath::Sin(A1) * P0.X, P0.Y);
				const FVector3f Q2(FMath::Cos(A1) * P1.X, FMath::Sin(A1) * P1.X, P1.Y);
				const FVector3f Q3(FMath::Cos(A0) * P1.X, FMath::Sin(A0) * P1.X, P1.Y);
				if (P0.X > 0.01f && P1.X > 0.01f)
				{
					B.Quad(Q0, Q1, Q2, Q3, Inside);
				}
				else if (P0.X > 0.01f)
				{
					B.Tri(Q0, Q1, Q3, Inside);
				}
				else if (P1.X > 0.01f)
				{
					B.Tri(Q0, Q2, Q3, Inside);
				}
			}
		}
	}

	void BuildCylinder(FShapeBuilder& B, int32 Sides)
	{
		BuildLathe(B, { FVector2f(0.f, -50.f), FVector2f(50.f, -50.f), FVector2f(50.f, 50.f), FVector2f(0.f, 50.f) }, Sides);
	}

	void BuildCone(FShapeBuilder& B, int32 Sides)
	{
		BuildLathe(B, { FVector2f(0.f, -50.f), FVector2f(50.f, -50.f), FVector2f(0.f, 50.f) }, Sides);
	}

	void BuildCapsule(FShapeBuilder& B, int32 Sides)
	{
		TArray<FVector2f> Profile;
		const float R = 25.f;
		const int32 Rings = 3;
		for (int32 i = 0; i <= Rings; ++i)
		{
			const float A = -PI * 0.5f + (PI * 0.5f) * i / Rings;
			Profile.Add(FVector2f(FMath::Cos(A) * R, -25.f + FMath::Sin(A) * R));
		}
		for (int32 i = 0; i <= Rings; ++i)
		{
			const float A = (PI * 0.5f) * i / Rings;
			Profile.Add(FVector2f(FMath::Cos(A) * R, 25.f + FMath::Sin(A) * R));
		}
		BuildLathe(B, Profile, Sides);
	}

	void BuildPrism(FShapeBuilder& B, bool bRight)
	{
		const FVector3f T0(0.f, -50.f, -50.f), T1(0.f, 50.f, -50.f), T2(0.f, bRight ? -50.f : 0.f, 50.f);
		const FVector3f In((T0 + T1 + T2) / 3.f);
		const FVector3f DX(50.f, 0.f, 0.f);
		B.Tri(T0 - DX, T1 - DX, T2 - DX, In);
		B.Tri(T0 + DX, T1 + DX, T2 + DX, In);
		B.Quad(T0 - DX, T1 - DX, T1 + DX, T0 + DX, In);
		B.Quad(T1 - DX, T2 - DX, T2 + DX, T1 + DX, In);
		B.Quad(T2 - DX, T0 - DX, T0 + DX, T2 + DX, In);
	}

	void BuildTorus(FShapeBuilder& B, int32 Major, int32 Minor)
	{
		const float R = 35.f, r = 15.f;
		auto Pt = [&](int32 i, int32 j)
		{
			const float U = 2.f * PI * i / Major, W = 2.f * PI * j / Minor;
			return FVector3f((R + r * FMath::Cos(W)) * FMath::Cos(U), (R + r * FMath::Cos(W)) * FMath::Sin(U), r * FMath::Sin(W));
		};
		for (int32 i = 0; i < Major; ++i)
		{
			const float Uc = 2.f * PI * (i + 0.5f) / Major;
			const FVector3f Center(R * FMath::Cos(Uc), R * FMath::Sin(Uc), 0.f);
			for (int32 j = 0; j < Minor; ++j)
			{
				B.Quad(Pt(i, j), Pt(i + 1, j), Pt(i + 1, j + 1), Pt(i, j + 1), Center);
			}
		}
	}

	void BuildGrid(FShapeBuilder& B, int32 Cells)
	{
		B.bSmoothUp = true;
		const float Step = 100.f / Cells;
		const FVector3f Below(0.f, 0.f, -100.f);
		for (int32 x = 0; x < Cells; ++x)
		{
			for (int32 y = 0; y < Cells; ++y)
			{
				const float X0 = -50.f + x * Step, Y0 = -50.f + y * Step;
				const FVector3f A(X0, Y0, 0.f), Bp(X0 + Step, Y0, 0.f), C(X0 + Step, Y0 + Step, 0.f), D(X0, Y0 + Step, 0.f);
				B.Tri(A, Bp, C, A + Below);
				B.Tri(A, C, D, A + Below);
			}
		}
	}

	bool SavePackageFor(UObject* Asset)
	{
		UPackage* Pkg = Asset->GetOutermost();
		const FString File = FPackageName::LongPackageNameToFilename(Pkg->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Pkg, Asset, *File, Args);
	}

	bool AssetExists(const FString& PackagePath)
	{
		return FPackageName::DoesPackageExist(PackagePath);
	}

	bool MakeMesh(const FString& Name, const TFunction<void(FShapeBuilder&)>& Fill, bool bForce)
	{
		const FString PackagePath = TEXT("/Game/TheFinalTake/Meshes/") + Name;
		if (!bForce && AssetExists(PackagePath))
		{
			UE_LOG(LogFinalTake, Display, TEXT("[FTContent] mesh %s exists, skipping"), *Name);
			return true;
		}
		FShapeBuilder B;
		Fill(B);

		FMeshDescription MD;
		FStaticMeshAttributes Attr(MD);
		Attr.Register();
		TVertexAttributesRef<FVector3f> Positions = Attr.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = Attr.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> UVs = Attr.GetVertexInstanceUVs();
		const FPolygonGroupID Group = MD.CreatePolygonGroup();
		Attr.GetPolygonGroupMaterialSlotNames()[Group] = FName(TEXT("Mat"));
		MD.ReserveNewVertices(B.P.Num());
		MD.ReserveNewVertexInstances(B.P.Num());
		MD.ReserveNewTriangles(B.P.Num() / 3);
		for (int32 t = 0; t + 2 < B.P.Num(); t += 3)
		{
			TArray<FVertexInstanceID, TInlineAllocator<3>> Inst;
			for (int32 k = 0; k < 3; ++k)
			{
				const FVector3f& Pos = B.P[t + k];
				const FVector3f& Nrm = B.N[t + k];
				const FVertexID Vid = MD.CreateVertex();
				Positions[Vid] = Pos;
				const FVertexInstanceID Iid = MD.CreateVertexInstance(Vid);
				Normals[Iid] = Nrm;
				const FVector3f A = Nrm.GetAbs();
				FVector2f UV = A.Z >= A.X && A.Z >= A.Y ? FVector2f(Pos.X, Pos.Y) : (A.X >= A.Y ? FVector2f(Pos.Y, Pos.Z) : FVector2f(Pos.X, Pos.Z));
				UVs[Iid] = UV / 100.f + FVector2f(0.5f, 0.5f);
				Inst.Add(Iid);
			}
			// UE uses counter-clockwise winding with (P2-P0)x(P1-P0) as the face normal; the builder already ordered them.
			MD.CreateTriangle(Group, Inst);
		}

		UPackage* Pkg = CreatePackage(*PackagePath);
		Pkg->FullyLoad();
		UStaticMesh* Mesh = NewObject<UStaticMesh>(Pkg, *Name, RF_Public | RF_Standalone);
		Mesh->GetStaticMaterials().Add(FStaticMaterial(FTVis::Matte(), FName(TEXT("Mat")), FName(TEXT("Mat"))));
		UStaticMesh::FBuildMeshDescriptionsParams Params;
		Params.bBuildSimpleCollision = true;
		Params.bCommitMeshDescription = true;
		Params.bMarkPackageDirty = true;
		Params.bFastBuild = false;
		TArray<const FMeshDescription*> List = { &MD };
		if (!Mesh->BuildFromMeshDescriptions(List, Params))
		{
			UE_LOG(LogFinalTake, Error, TEXT("[FTContent] failed to build %s"), *Name);
			return false;
		}
		for (int32 LOD = 0; LOD < Mesh->GetNumSourceModels(); ++LOD)
		{
			FStaticMeshSourceModel& SM = Mesh->GetSourceModel(LOD);
			SM.BuildSettings.bRecomputeNormals = false;
			SM.BuildSettings.bRecomputeTangents = true;
			SM.BuildSettings.bGenerateLightmapUVs = false;
		}
		Mesh->PostEditChange();
		FAssetRegistryModule::AssetCreated(Mesh);
		const bool bSaved = SavePackageFor(Mesh);
		UE_LOG(LogFinalTake, Display, TEXT("[FTContent] mesh %s: %d triangles, saved=%d"), *Name, B.P.Num() / 3, bSaved ? 1 : 0);
		return bSaved;
	}

	int32 ImportAudio(bool bForce)
	{
		const FString Dir = FPaths::ProjectDir() / TEXT("RawAudio");
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.wav")), true, false);
		TArray<FString> ToImport;
		for (const FString& F : Files)
		{
			const FString Name = FPaths::GetBaseFilename(F);
			if (bForce || !AssetExists(TEXT("/Game/TheFinalTake/Audio/") + Name))
			{
				ToImport.Add(Dir / F);
			}
		}
		if (ToImport.Num() == 0)
		{
			UE_LOG(LogFinalTake, Display, TEXT("[FTContent] audio up to date (%d wavs)"), Files.Num());
			return 0;
		}
		UAutomatedAssetImportData* Data = NewObject<UAutomatedAssetImportData>();
		Data->Filenames = ToImport;
		Data->DestinationPath = TEXT("/Game/TheFinalTake/Audio");
		Data->bReplaceExisting = true;
		IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TArray<UObject*> Imported = Tools.ImportAssetsAutomated(Data);
		int32 Count = 0;
		for (UObject* Obj : Imported)
		{
			if (USoundWave* W = Cast<USoundWave>(Obj))
			{
				W->bLooping = W->GetName().Contains(TEXT("Loop"));
				W->MarkPackageDirty();
				SavePackageFor(W);
				++Count;
			}
		}
		UE_LOG(LogFinalTake, Display, TEXT("[FTContent] imported %d sound(s)"), Count);
		return Count;
	}

	/** One-sided copy of the engine text material: labels must not read mirrored from behind. */
	bool MakeTextMaterial(bool bForce)
	{
		const FString PackagePath = TEXT("/Game/TheFinalTake/Materials/M_FT_Text");
		if (!bForce && AssetExists(PackagePath))
		{
			return true;
		}
		UMaterial* Src = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultTextMaterialOpaque.DefaultTextMaterialOpaque"));
		if (!Src)
		{
			UE_LOG(LogFinalTake, Error, TEXT("[FTContent] engine text material not found"));
			return false;
		}
		UPackage* Pkg = CreatePackage(*PackagePath);
		UMaterial* M = DuplicateObject<UMaterial>(Src, Pkg, TEXT("M_FT_Text"));
		M->SetFlags(RF_Public | RF_Standalone);
		M->TwoSided = false;
		M->PostEditChange();
		FAssetRegistryModule::AssetCreated(M);
		M->MarkPackageDirty();
		const bool bSaved = SavePackageFor(M);
		UE_LOG(LogFinalTake, Display, TEXT("[FTContent] M_FT_Text (one-sided) saved=%d"), bSaved ? 1 : 0);
		return bSaved;
	}

	/** Every FT material may end up on instanced meshes (studio shell, particles) - flag them once and resave. */
	void EnsureMaterialUsage()
	{
		const TCHAR* Names[] = { TEXT("M_FT_Matte"), TEXT("M_FT_Glow"), TEXT("M_FT_Translucent"), TEXT("M_FT_Water"), TEXT("M_FT_Highlight"), TEXT("M_FT_Screen"), TEXT("M_FT_MatteISM") };
		for (const TCHAR* Name : Names)
		{
			const FString Path = FString::Printf(TEXT("/Game/TheFinalTake/Materials/%s.%s"), Name, Name);
			UMaterial* M = LoadObject<UMaterial>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!M || M->bUsedWithInstancedStaticMeshes)
			{
				continue;
			}
			M->bUsedWithInstancedStaticMeshes = true;
			M->PostEditChange();
			M->MarkPackageDirty();
			UE_LOG(LogFinalTake, Display, TEXT("[FTContent] %s: instanced-mesh usage enabled, saved=%d"), Name, SavePackageFor(M) ? 1 : 0);
		}
	}

	template <typename T>
	void MakeFilmAsset(FName FilmId, bool bForce)
	{
		const FString Name = FString::Printf(TEXT("DA_Film_%s"), *FilmId.ToString());
		const FString PackagePath = TEXT("/Game/TheFinalTake/Data/") + Name;
		if (!bForce && AssetExists(PackagePath))
		{
			UE_LOG(LogFinalTake, Display, TEXT("[FTContent] %s exists, skipping"), *Name);
			return;
		}
		UPackage* Pkg = CreatePackage(*PackagePath);
		T* Asset = NewObject<T>(Pkg, *Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		UE_LOG(LogFinalTake, Display, TEXT("[FTContent] %s saved=%d"), *Name, SavePackageFor(Asset) ? 1 : 0);
	}
}
#endif

UFTContentCommandlet::UFTContentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UFTContentCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	const bool bForce = Params.Contains(TEXT("-force"));
	int32 Failed = 0;
	auto M = [&](const TCHAR* Name, TFunction<void(FShapeBuilder&)> F) { Failed += MakeMesh(Name, F, bForce) ? 0 : 1; };
	M(TEXT("SM_FT_Box"), [](FShapeBuilder& B) { BuildBox(B, 7.f); });
	M(TEXT("SM_FT_Sphere"), [](FShapeBuilder& B) { BuildIcosphere(B, 1); });
	M(TEXT("SM_FT_Ball"), [](FShapeBuilder& B) { BuildIcosphere(B, 2); });
	M(TEXT("SM_FT_Cylinder"), [](FShapeBuilder& B) { BuildCylinder(B, 10); });
	M(TEXT("SM_FT_Cone"), [](FShapeBuilder& B) { BuildCone(B, 10); });
	M(TEXT("SM_FT_Prism"), [](FShapeBuilder& B) { BuildPrism(B, false); });
	M(TEXT("SM_FT_Ramp"), [](FShapeBuilder& B) { BuildPrism(B, true); });
	M(TEXT("SM_FT_Torus"), [](FShapeBuilder& B) { BuildTorus(B, 14, 7); });
	M(TEXT("SM_FT_Capsule"), [](FShapeBuilder& B) { BuildCapsule(B, 10); });
	M(TEXT("SM_FT_WaterGrid"), [](FShapeBuilder& B) { BuildGrid(B, 40); });
	ImportAudio(bForce);
	Failed += MakeTextMaterial(bForce) ? 0 : 1;
	EnsureMaterialUsage();
	MakeFilmAsset<UFTFilm_JawsOfTheStudio>(FTTags::FilmJaws, bForce);
	MakeFilmAsset<UFTFilm_MoonfallMotel>(FTTags::FilmMoonfall, bForce);
	MakeFilmAsset<UFTFilm_CastleOnFire>(FTTags::FilmCastle, bForce);
	if (Params.Contains(TEXT("-map")))
	{
		extern bool FTBuildStudioMap();
		Failed += FTBuildStudioMap() ? 0 : 1;
	}
	UE_LOG(LogFinalTake, Display, TEXT("[FTContent] done, %d failure(s)"), Failed);
	return Failed;
#else
	return 1;
#endif
}
