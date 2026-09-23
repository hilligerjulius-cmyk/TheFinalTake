#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTPlayerState.generated.h"

UCLASS()
class THE_FINAL_TAKE_API AFTPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 0-3 crew look preset + marker colour. */
	UPROPERTY(Replicated, BlueprintReadOnly) int32 CrewIndex = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) EFTCostume Costume = EFTCostume::None;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bOnCamera = false;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bRunningEffect = false;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bCarrying = false;

	static FLinearColor CrewColor(int32 Index);
	static FString CrewRoleName(int32 Index);
	FString GetCrewName() const;
};
