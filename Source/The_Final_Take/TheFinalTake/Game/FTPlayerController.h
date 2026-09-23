#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FTPlayerController.generated.h"

class UFTHUDWidget;
class UFTTitleMenuWidget;
class UFTScriptBookWidget;
class UFTPauseMenuWidget;
class UFTResultsWidget;
class UUserWidget;

UCLASS()
class THE_FINAL_TAKE_API AFTPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFTPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

	// ---- local UI
	void ShowToast(const FText& Text, bool bError);
	void ToggleHelp();
	void TogglePause();
	void ClosePause();
	bool IsUIBlockingGameplay() const;
	UFTHUDWidget* GetHUDWidget() const { return HUD; }

	// ---- script book
	UFUNCTION(Client, Reliable) void ClientOpenScriptBook();
	UFUNCTION(Client, Reliable) void ClientCloseScriptBook();
	UFUNCTION(Server, Reliable) void ServerSelectFilm(FName FilmId);
	UFUNCTION(Server, Reliable) void ServerCloseScriptBook();
	void LocalCloseScriptBook(bool bNotifyServer);

	// ---- shoot control
	UFUNCTION(Server, Reliable) void ServerRequestRestart();
	UFUNCTION(Server, Reliable) void ServerPing(FVector_NetQuantize Location);
	UFUNCTION(Client, Reliable) void ClientToast(const FText& Text, bool bError);

	// ---- title / session
	void StartDemo(bool bHost);
	void JoinStudio(const FString& Address);
	void LeaveToTitle();
	void QuitGame();

private:
	void EnsureWidgets();
	void ApplyInputMode();
	void OnStateChanged();
	void SetupInputContext();
	void UpdateTitleCamera();

	UPROPERTY(Transient) TObjectPtr<UFTHUDWidget> HUD;
	UPROPERTY(Transient) TObjectPtr<UFTTitleMenuWidget> TitleMenu;
	UPROPERTY(Transient) TObjectPtr<UFTScriptBookWidget> ScriptBook;
	UPROPERTY(Transient) TObjectPtr<UFTPauseMenuWidget> PauseMenu;
	UPROPERTY(Transient) TObjectPtr<UFTResultsWidget> Results;
	UPROPERTY(Transient) TObjectPtr<class UAudioComponent> Music;

	bool bTitleShown = false;
	bool bBookOpen = false;
	bool bPaused = false;
	FDelegateHandle StateHandle;
};
