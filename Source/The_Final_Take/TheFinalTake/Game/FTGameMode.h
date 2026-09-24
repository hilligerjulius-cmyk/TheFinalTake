#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FTGameMode.generated.h"

class AFTSceneManager;

/**
 * Listen-server game mode. Standalone launches without "?ftplay" show the title screen;
 * "Play Demo" reopens the map as a listen server. Late joiners are only accepted
 * before a script is greenlit.
 */
UCLASS()
class THE_FINAL_TAKE_API AFTGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFTGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void StartPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditAnywhere, Category = "Shoot") float DawnSeconds = 1800.f;

	bool IsTitleMode() const { return bTitleMode; }
	int32 GetCareerSlot() const { return CareerSlot; }

private:
	bool bTitleMode = false;
	/** "?career=N" from the title screen; automated tests default to a reserved slot. */
	int32 CareerSlot = 1;
	UPROPERTY() TObjectPtr<AFTSceneManager> SceneManager;
	UPROPERTY() TObjectPtr<class AFTCareerManager> CareerManager;
};
