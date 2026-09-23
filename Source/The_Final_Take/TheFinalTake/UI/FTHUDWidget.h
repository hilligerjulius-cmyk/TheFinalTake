#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTHUDWidget.generated.h"

class UCanvasPanel;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UBorder;
class UProgressBar;
class UImage;
class UWidget;

/** In-game HUD built entirely in C++ (call sheet, clock, prompt, camera viewfinder, banners). */
UCLASS()
class THE_FINAL_TAKE_API UFTHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowToast(const FText& Text, bool bError);
	void ShowAnnouncement(const FText& Text, EFTAnnounceStyle Style);
	void ShowTakeResult(const FFTTakeResult& Result);
	void AddPing(const FVector& Location, int32 ColorIndex, const FString& Name);
	void SetHelpVisible(bool bVisible);
	bool IsHelpVisible() const { return bHelpVisible; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Build();
	void UpdateCallSheet();
	void UpdateClock();
	void UpdateCrew();
	void UpdatePrompt();
	void UpdateCamera(float Dt);
	void UpdateTransient(float Dt);
	void UpdatePings();

	UPROPERTY() TObjectPtr<UCanvasPanel> Root;
	// call sheet
	UPROPERTY() TObjectPtr<UBorder> CallSheet;
	UPROPERTY() TObjectPtr<UTextBlock> FilmLine;
	UPROPERTY() TObjectPtr<UTextBlock> SceneTitle;
	UPROPERTY() TObjectPtr<UBorder> StatePill;
	UPROPERTY() TObjectPtr<UTextBlock> StateText;
	UPROPERTY() TObjectPtr<UVerticalBox> ObjectiveList;
	UPROPERTY() TObjectPtr<UProgressBar> CaptureBar;
	UPROPERTY() TObjectPtr<UTextBlock> CaptureText;
	// banner / clock / crew
	UPROPERTY() TObjectPtr<UBorder> CalloutBanner;
	UPROPERTY() TObjectPtr<UTextBlock> CalloutText;
	UPROPERTY() TObjectPtr<UTextBlock> ClockText;
	UPROPERTY() TObjectPtr<UTextBlock> ClockSub;
	UPROPERTY() TObjectPtr<UProgressBar> DawnBar;
	UPROPERTY() TObjectPtr<UProgressBar> ConditionBar;
	UPROPERTY() TObjectPtr<UTextBlock> ConditionText;
	UPROPERTY() TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY() TObjectPtr<UVerticalBox> CrewList;
	// prompt
	UPROPERTY() TObjectPtr<UWidget> Crosshair;
	UPROPERTY() TObjectPtr<UBorder> PromptBox;
	UPROPERTY() TObjectPtr<UTextBlock> PromptKey;
	UPROPERTY() TObjectPtr<UTextBlock> PromptText;
	UPROPERTY() TObjectPtr<UTextBlock> PromptReason;
	UPROPERTY() TObjectPtr<UProgressBar> HoldBar;
	UPROPERTY() TObjectPtr<UTextBlock> CarryHint;
	// toasts / announcements / stamp
	UPROPERTY() TObjectPtr<UVerticalBox> ToastBox;
	UPROPERTY() TObjectPtr<UBorder> AnnounceBox;
	UPROPERTY() TObjectPtr<UTextBlock> AnnounceText;
	UPROPERTY() TObjectPtr<UWidget> AnnounceRoot;
	UPROPERTY() TObjectPtr<UWidget> StampRoot;
	UPROPERTY() TObjectPtr<UBorder> StampBox;
	UPROPERTY() TObjectPtr<UTextBlock> StampText;
	UPROPERTY() TObjectPtr<UTextBlock> StampScore;
	UPROPERTY() TObjectPtr<UVerticalBox> StampReasons;
	// camera viewfinder
	UPROPERTY() TObjectPtr<UCanvasPanel> CameraLayer;
	UPROPERTY() TObjectPtr<UImage> RecDot;
	UPROPERTY() TObjectPtr<UTextBlock> RecText;
	UPROPERTY() TObjectPtr<UTextBlock> Timecode;
	UPROPERTY() TObjectPtr<UProgressBar> FrameBar;
	UPROPERTY() TObjectPtr<UTextBlock> FrameText;
	UPROPERTY() TObjectPtr<UVerticalBox> SubjectList;
	UPROPERTY() TObjectPtr<UTextBlock> CameraHint;
	UPROPERTY() TObjectPtr<UTextBlock> CameraStatus;
	// help / danger / pings
	UPROPERTY() TObjectPtr<UWidget> HelpCard;
	UPROPERTY() TArray<TObjectPtr<UImage>> DangerEdges;
	UPROPERTY() TArray<TObjectPtr<UWidget>> PingWidgets;

	struct FToast { TWeakObjectPtr<UWidget> Widget; float Age = 0.f; };
	struct FAnnounce { FText Text; EFTAnnounceStyle Style; };
	struct FPing { FVector Location; float Age = 0.f; int32 Color = 0; FString Name; };
	TArray<FToast> Toasts;
	TArray<FAnnounce> AnnounceQueue;
	TArray<FPing> Pings;
	float AnnounceAge = 100.f;
	float StampAge = 100.f;
	float HelpAge = 0.f;
	bool bHelpVisible = true;
	bool bHelpAutoHide = true;
	FString ObjectiveSignature;
	float CrewTimer = 0.f;
	float Time = 0.f;
};
