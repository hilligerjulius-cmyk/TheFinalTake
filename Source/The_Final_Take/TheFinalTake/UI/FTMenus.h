#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheFinalTake/Core/FTTypes.h"
#include "FTMenus.generated.h"

class UCanvasPanel;
class UVerticalBox;
class UHorizontalBox;
class UTextBlock;
class UBorder;
class UButton;
class USlider;
class UCheckBox;
class UEditableTextBox;
class UImage;
class UWidget;
class AFTPlayerController;
class UFTFilmDefinition;

/** Volume / sensitivity / reduced shake. Embedded in the title and pause menus. */
UCLASS()
class THE_FINAL_TAKE_API UFTSettingsPanel : public UUserWidget
{
	GENERATED_BODY()
public:
	FSimpleDelegate OnBack;
protected:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void HandleVolume(float V);
	UFUNCTION() void HandleSensitivity(float V);
	UFUNCTION() void HandleShake(bool bOn);
	UFUNCTION() void HandleBack();
	void Refresh();
	UPROPERTY() TObjectPtr<USlider> Volume;
	UPROPERTY() TObjectPtr<USlider> Sensitivity;
	UPROPERTY() TObjectPtr<UCheckBox> Shake;
	UPROPERTY() TObjectPtr<UTextBlock> VolumeValue;
	UPROPERTY() TObjectPtr<UTextBlock> SensValue;
	UPROPERTY() TObjectPtr<UButton> BackButton;
};

UCLASS()
class THE_FINAL_TAKE_API UFTTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UFUNCTION() void HandlePlay();
	UFUNCTION() void HandleHost();
	UFUNCTION() void HandleJoinToggle();
	UFUNCTION() void HandleConnect();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleQuit();
	void ShowSettings(bool bShow);
	AFTPlayerController* PC() const;

	// ---- career slots ("worlds")
	void ShowCareer(bool bShow, bool bHost);
	void RefreshSlots();
	void PlaySlot(int32 SlotIndex);
	void DeleteSlot(int32 SlotIndex);
	UFUNCTION() void HandleSlot1();
	UFUNCTION() void HandleSlot2();
	UFUNCTION() void HandleSlot3();
	UFUNCTION() void HandleDelete1();
	UFUNCTION() void HandleDelete2();
	UFUNCTION() void HandleDelete3();
	UFUNCTION() void HandleCareerBack();
	UPROPERTY() TObjectPtr<UWidget> CareerPanel;
	UPROPERTY() TObjectPtr<UTextBlock> CareerHeader;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotTitles;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotDetails;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotPlayLabels;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> SlotDeleteLabels;
	UPROPERTY() TArray<TObjectPtr<UButton>> SlotDeleteButtons;
	bool bCareerHost = false;
	int32 DeleteArmed = 0;

	UPROPERTY() TObjectPtr<UWidget> MainColumn;
	UPROPERTY() TObjectPtr<UWidget> JoinRow;
	UPROPERTY() TObjectPtr<UEditableTextBox> Address;
	UPROPERTY() TObjectPtr<UFTSettingsPanel> Settings;
	UPROPERTY() TObjectPtr<UWidget> TitleText;
	UPROPERTY() TObjectPtr<UButton> PlayButton;
	float Age = 0.f;
};

UCLASS()
class THE_FINAL_TAKE_API UFTPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Refresh();
protected:
	virtual void NativeOnInitialized() override;
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleControls();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandleLeave();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleResetCareer();
	AFTPlayerController* PC() const;
	UPROPERTY() TObjectPtr<UWidget> MainColumn;
	UPROPERTY() TObjectPtr<UFTSettingsPanel> Settings;
	UPROPERTY() TObjectPtr<UButton> RestartButton;
	UPROPERTY() TObjectPtr<UTextBlock> RestartLabel;
	UPROPERTY() TObjectPtr<UButton> ResumeButton;
	UPROPERTY() TObjectPtr<UTextBlock> CareerText;
	UPROPERTY() TObjectPtr<UButton> ResetCareerButton;
	UPROPERTY() TObjectPtr<UTextBlock> ResetCareerLabel;
	bool bResetArmed = false;
};

/** The giant physical script book: pick a film (only playable scripts can be greenlit). */
UCLASS()
class THE_FINAL_TAKE_API UFTScriptBookWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Refresh();
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UFUNCTION() void HandleFilm0();
	UFUNCTION() void HandleFilm1();
	UFUNCTION() void HandleFilm2();
	UFUNCTION() void HandleGreenlight();
	UFUNCTION() void HandleClose();
	void SelectFilm(int32 Index);
	AFTPlayerController* PC() const;

	UPROPERTY() TArray<TObjectPtr<UButton>> FilmButtons;
	UPROPERTY() TObjectPtr<UVerticalBox> DetailBox;
	UPROPERTY() TObjectPtr<UButton> GreenlightButton;
	UPROPERTY() TObjectPtr<UTextBlock> GreenlightLabel;
	UPROPERTY() TObjectPtr<UTextBlock> StatusText;
	UPROPERTY() TObjectPtr<UWidget> BookRoot;
	/** Strong references: the book outlives GC passes while it sits in the viewport. */
	UPROPERTY() TArray<TObjectPtr<UFTFilmDefinition>> Films;
	int32 Selected = 0;
	float Age = 0.f;
};

UCLASS()
class THE_FINAL_TAKE_API UFTResultsWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Refresh();
	static FText RatingForTotal(int32 Total, bool bFailed);
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleLeave();
	AFTPlayerController* PC() const;
	UPROPERTY() TObjectPtr<UVerticalBox> Content;
	UPROPERTY() TObjectPtr<UVerticalBox> LeftColumn;
	UPROPERTY() TObjectPtr<UVerticalBox> RightColumn;
	UPROPERTY() TObjectPtr<UWidget> Card;
	UPROPERTY() TObjectPtr<UButton> RetryButton;
	float Age = 0.f;
};

/** Montage shown on the in-world cinema screen during the premiere. */
UCLASS()
class THE_FINAL_TAKE_API UFTPremiereWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Studio screen: plays the optional test screening instead of the premiere. */
	bool bTestScreening = false;
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY() TObjectPtr<UBorder> Backdrop;
	UPROPERTY() TObjectPtr<UTextBlock> BigText;
	UPROPERTY() TObjectPtr<UTextBlock> SubText;
	UPROPERTY() TObjectPtr<UImage> Still;
	UPROPERTY() TObjectPtr<UBorder> StillFrame;
	UPROPERTY() TObjectPtr<UBorder> Stamp;
	UPROPERTY() TObjectPtr<UTextBlock> StampText;
	int32 LastCard = -2;
};
