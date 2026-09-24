#pragma once

#include "CoreMinimal.h"
#include "TheFinalTake/Interaction/FTStudioActor.h"
#include "FTStudioShell.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class USpotLightComponent;

/**
 * The whole studio building (street, facade, lobby, director's office, wardrobe, Stage 4 with
 * the water tank beach set, camera deck, catwalk stairs, projection room, prop warehouse),
 * built reproducibly from instanced low-poly blocks, text signs and a light rig.
 * Place one at the world origin; gameplay actors are placed on top of it in the level.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTStudioShell : public AFTStudioActor
{
	GENERATED_BODY()

public:
	AFTStudioShell();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void OnShootPhaseChanged(EFTShootPhase Phase) override;
	virtual void OnStagePowerChanged(bool bPowered) override;

	enum EGroup : int32
	{
		CubeSolid, CubeDeco, BoxSolid, BoxDeco, CylSolid, CylDeco, SphereDeco, BallDeco, ConeDeco,
		PrismDeco, RampSolid, TorusDeco, CapsuleDeco, GlassSolid, Blocker, ShorelineDeco, NumGroups
	};

protected:
	void Build();
	void ClearAll();
	int32 Add(int32 Group, const FVector& Center, const FVector& Size, const FLinearColor& Color, float Emissive = 0.f, const FRotator& Rot = FRotator::ZeroRotator, float Gloss = 0.f);
	void Text(const FString& S, const FVector& Loc, float Yaw, float Size, const FColor& Color, float Pitch = 0.f);
	void Point(const FVector& Loc, const FLinearColor& Color, float Intensity, float Radius, int32 Zone, bool bShadows = false);
	void Spot(const FVector& Loc, const FRotator& Rot, const FLinearColor& Color, float Intensity, float Radius, float Cone, int32 Zone);
	void Arrow(const FVector& From, const FVector& To, const FLinearColor& Color, int32 Count);
	void Wall(const FVector& Min, const FVector& Max, const FLinearColor& Color, bool bSolid = true);
	void Stairs(const FVector& Start, float Run, float Width, float Rise, int32 Steps, bool bAlongX, const FLinearColor& Tread, const FLinearColor& Edge);
	void Poster(const FVector& Loc, float Yaw, int32 Film, float Scale);
	void Plant(const FVector& Loc, float Scale);
	void Crate(const FVector& Loc, float Size, float Yaw);
	void FlightCase(const FVector& Loc, const FVector& Size, float Yaw);
	void Cone(const FVector& Loc);
	void Palm(const FVector& Loc, float Height);
	void Truss(float Y, float Z);
	void ApplyLighting();

	void BuildExterior();
	void BuildLobby();
	void BuildOffice();
	void BuildWardrobe();
	void BuildStage();
	void BuildTankSet();
	void BuildUpperLevel();
	void BuildWarehouse();

	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UInstancedStaticMeshComponent>> Groups;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UTextRenderComponent>> Texts;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UPointLightComponent>> Points;
	UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<USpotLightComponent>> Spots;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TankWater;
	UPROPERTY() TArray<float> LightBase;
	UPROPERTY() TArray<int32> LightZone;

	int32 NextText = 0;
	int32 NextPoint = 0;
	int32 NextSpot = 0;
	float PhaseDim = 1.f;
	float PowerDim = 1.f;
};
