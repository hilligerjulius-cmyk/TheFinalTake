#include "TheFinalTake/UI/FTMenus.h"

#include "TheFinalTake/UI/FTUIStyle.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/UI/FTHUDWidget.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "TheFinalTake/Core/FTSettings.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FinalTakeMenus"

using namespace FTUI;

namespace
{
	UCanvasPanel* MakeRoot(UWidgetTree* T)
	{
		UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		T->RootWidget = Root;
		return Root;
	}

	UWidget* Sized(UWidgetTree* T, UWidget* W, float Width, float Height = 0.f)
	{
		USizeBox* S = T->ConstructWidget<USizeBox>();
		if (Width > 0.f) S->SetWidthOverride(Width);
		if (Height > 0.f) S->SetHeightOverride(Height);
		S->AddChild(W);
		return S;
	}

	UWidget* Fullscreen(UCanvasPanel* Root, UWidget* W)
	{
		UCanvasPanelSlot* S = Root->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
		return W;
	}

	void Click(const UObject* Ctx)
	{
		FTAudio::Play2D(Ctx, EFTSound::UIClick, 0.8f);
	}

	FLinearColor RatingColor(EFTTakeRating R)
	{
		switch (R)
		{
		case EFTTakeRating::Perfect: return Magenta;
		case EFTTakeRating::Great: return Teal;
		case EFTTakeRating::Usable: return Amber;
		default: return Coral;
		}
	}

	FText RatingName(EFTTakeRating R)
	{
		switch (R)
		{
		case EFTTakeRating::Perfect: return LOCTEXT("RPerfect", "PERFECT");
		case EFTTakeRating::Great: return LOCTEXT("RGreat", "GREAT");
		case EFTTakeRating::Usable: return LOCTEXT("RUsable", "USABLE");
		default: return LOCTEXT("RRetake", "RETAKE");
		}
	}
}

// ============================================================================ settings panel

void UFTSettingsPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UBorder* Card = Box(T, Paper, 14.f, FMargin(26.f, 20.f), Ink, 3.f);
	T->RootWidget = Card;
	UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
	Card->SetContent(V);
	AddV(V, Text(T, LOCTEXT("Settings", "SETTINGS"), 28, Ink, TEXT("Black")), FMargin(0.f, 0.f, 0.f, 12.f));

	auto SliderRow = [&](const FText& Label, TObjectPtr<USlider>& OutSlider, TObjectPtr<UTextBlock>& OutValue)
	{
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		AddH(Row, Sized(T, Text(T, Label, 18, Ink, TEXT("Bold")), 270.f));
		OutSlider = T->ConstructWidget<USlider>();
		// chunky track + pill thumb to match the paper cards
		FSliderStyle SliderStyle;
		const FSlateBrush Track = Round(PaperDark, 5.f, Ink, 2.f);
		FSlateBrush Thumb = Round(Coral, 7.f, Ink, 2.f);
		Thumb.ImageSize = FVector2D(20.f, 26.f);
		FSlateBrush ThumbHover = Round(Amber, 7.f, Ink, 2.f);
		ThumbHover.ImageSize = Thumb.ImageSize;
		SliderStyle.SetNormalBarImage(Track);
		SliderStyle.SetHoveredBarImage(Track);
		SliderStyle.SetDisabledBarImage(Track);
		SliderStyle.SetNormalThumbImage(Thumb);
		SliderStyle.SetHoveredThumbImage(ThumbHover);
		SliderStyle.SetDisabledThumbImage(Thumb);
		SliderStyle.SetBarThickness(10.f);
		OutSlider->SetWidgetStyle(SliderStyle);
		OutSlider->SetSliderBarColor(FLinearColor::White);
		OutSlider->SetSliderHandleColor(FLinearColor::White);
		AddH(Row, Sized(T, OutSlider, 240.f, 32.f));
		OutValue = Text(T, FText::GetEmpty(), 18, TealDark, TEXT("Black"));
		AddH(Row, Sized(T, OutValue, 70.f), FMargin(12.f, 0.f, 0.f, 0.f));
		AddV(V, Row, FMargin(0.f, 6.f));
	};
	SliderRow(LOCTEXT("Volume", "Master volume"), Volume, VolumeValue);
	SliderRow(LOCTEXT("Sens", "Mouse sensitivity"), Sensitivity, SensValue);
	UHorizontalBox* ShakeRow = T->ConstructWidget<UHorizontalBox>();
	AddH(ShakeRow, Sized(T, Text(T, LOCTEXT("Shake", "Reduced screen shake"), 18, Ink, TEXT("Bold")), 270.f));
	Shake = T->ConstructWidget<UCheckBox>();
	{
		FCheckBoxStyle BoxStyle;
		auto BoxBrush = [](const FLinearColor& Fill, float Outline)
		{
			FSlateBrush B = Round(Fill, 6.f, Ink, Outline);
			B.ImageSize = FVector2D(28.f, 28.f);
			return B;
		};
		BoxStyle.SetUncheckedImage(BoxBrush(Cream, 2.f));
		BoxStyle.SetUncheckedHoveredImage(BoxBrush(Cream, 3.f));
		BoxStyle.SetUncheckedPressedImage(BoxBrush(PaperDark, 3.f));
		BoxStyle.SetCheckedImage(BoxBrush(Teal, 2.f));
		BoxStyle.SetCheckedHoveredImage(BoxBrush(Teal, 3.f));
		BoxStyle.SetCheckedPressedImage(BoxBrush(TealDark, 3.f));
		Shake->SetWidgetStyle(BoxStyle);
	}
	AddH(ShakeRow, Shake, FMargin(0.f, 0.f, 0.f, 0.f));
	AddV(V, ShakeRow, FMargin(0.f, 10.f));
	BackButton = Button(T, LOCTEXT("Back", "BACK"), Cream, 20);
	AddV(V, BackButton, FMargin(0.f, 16.f, 0.f, 0.f), 1);

	Volume->OnValueChanged.AddDynamic(this, &UFTSettingsPanel::HandleVolume);
	Sensitivity->OnValueChanged.AddDynamic(this, &UFTSettingsPanel::HandleSensitivity);
	Shake->OnCheckStateChanged.AddDynamic(this, &UFTSettingsPanel::HandleShake);
	BackButton->OnClicked.AddDynamic(this, &UFTSettingsPanel::HandleBack);
	Refresh();
}

void UFTSettingsPanel::Refresh()
{
	UFTSettings* S = UFTSettings::Get();
	Volume->SetValue(S->MasterVolume);
	Sensitivity->SetValue((S->MouseSensitivity - 0.2f) / 2.8f);
	Shake->SetIsChecked(S->bReducedScreenShake);
	VolumeValue->SetText(FText::AsPercent(S->MasterVolume));
	SensValue->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), S->MouseSensitivity)));
}

void UFTSettingsPanel::HandleVolume(float V)
{
	UFTSettings* S = UFTSettings::Get();
	S->MasterVolume = V;
	S->Save();
	VolumeValue->SetText(FText::AsPercent(V));
}

void UFTSettingsPanel::HandleSensitivity(float V)
{
	UFTSettings* S = UFTSettings::Get();
	S->MouseSensitivity = 0.2f + V * 2.8f;
	S->Save();
	SensValue->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), S->MouseSensitivity)));
}

void UFTSettingsPanel::HandleShake(bool bOn)
{
	UFTSettings* S = UFTSettings::Get();
	S->bReducedScreenShake = bOn;
	S->Save();
	Click(this);
}

void UFTSettingsPanel::HandleBack()
{
	Click(this);
	OnBack.ExecuteIfBound();
}

// ============================================================================ title

AFTPlayerController* UFTTitleMenuWidget::PC() const
{
	return Cast<AFTPlayerController>(GetOwningPlayer());
}

void UFTTitleMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = MakeRoot(T);

	UImage* Side = Swatch(T, FLinearColor(0.02f, 0.025f, 0.07f, 0.72f), FVector2D(10.f));
	UCanvasPanelSlot* SideSlot = Root->AddChildToCanvas(Side);
	SideSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 1.f));
	SideSlot->SetOffsets(FMargin(0.f, 0.f, 700.f, 0.f));

	UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
	UTextBlock* Title = Text(T, LOCTEXT("GameTitle", "THE FINAL TAKE"), 84, Cream, TEXT("Black"));
	Title->SetShadowOffset(FVector2D(5.f, 6.f));
	Title->SetShadowColorAndOpacity(Coral);
	TitleText = Title;
	AddV(V, Title, FMargin(0.f));
	AddV(V, Text(T, LOCTEXT("Tagline", "Make the movie. Survive the premiere."), 24, Amber, TEXT("Bold")), FMargin(4.f, 0.f, 0.f, 34.f));

	UVerticalBox* Buttons = T->ConstructWidget<UVerticalBox>();
	PlayButton = Button(T, LOCTEXT("PlayDemo", "PLAY DEMO"), Coral, 30);
	UButton* Host = Button(T, LOCTEXT("Host", "HOST STUDIO  (LAN prototype)"), Teal, 20, nullptr, Cream);
	UButton* Join = Button(T, LOCTEXT("Join", "JOIN STUDIO  (LAN prototype)"), Teal, 20, nullptr, Cream);
	UButton* Sett = Button(T, LOCTEXT("SettingsBtn", "SETTINGS"), Cream, 20);
	UButton* Quit = Button(T, LOCTEXT("Quit", "QUIT"), PaperDark, 20);
	for (UButton* B : { PlayButton.Get(), Host, Join, Sett, Quit })
	{
		AddV(Buttons, Sized(T, B, 460.f), FMargin(0.f, 7.f));
	}
	PlayButton->OnClicked.AddDynamic(this, &UFTTitleMenuWidget::HandlePlay);
	Host->OnClicked.AddDynamic(this, &UFTTitleMenuWidget::HandleHost);
	Join->OnClicked.AddDynamic(this, &UFTTitleMenuWidget::HandleJoinToggle);
	Sett->OnClicked.AddDynamic(this, &UFTTitleMenuWidget::HandleSettings);
	Quit->OnClicked.AddDynamic(this, &UFTTitleMenuWidget::HandleQuit);
	AddV(V, Buttons);

	UHorizontalBox* JoinBox = T->ConstructWidget<UHorizontalBox>();
	Address = T->ConstructWidget<UEditableTextBox>();
	Address->SetText(FText::FromString(UFTSettings::Get()->LastJoinAddress));
	Address->SetHintText(LOCTEXT("IPHint", "Host IP address"));
	AddH(JoinBox, Sized(T, Address, 280.f, 44.f));
	UButton* Connect = Button(T, LOCTEXT("Connect", "CONNECT"), Amber, 18);
	Connect->OnClicked.AddDynamic(this, &UFTTitleMenuWidget::HandleConnect);
	AddH(JoinBox, Connect, FMargin(10.f, 0.f));
	JoinRow = JoinBox;
	JoinRow->SetVisibility(ESlateVisibility::Collapsed);
	AddV(V, JoinBox, FMargin(0.f, 10.f));

	AddV(V, Text(T, LOCTEXT("Footer", "Co-op comedy for 1-4 crew  -  Play Demo starts a local listen-server session.\nHost/Join are LAN prototypes (direct IP)."), 14, FLinearColor(0.8f, 0.78f, 0.7f, 1.f), TEXT("Regular")), FMargin(0.f, 30.f, 0.f, 0.f));
	MainColumn = V;
	Place(Root, V, FAnchors(0.f, 0.5f), FVector2D(0.f, 0.5f), FVector2D(90.f, 0.f));

	Settings = CreateWidget<UFTSettingsPanel>(GetOwningPlayer(), UFTSettingsPanel::StaticClass());
	Settings->OnBack.BindLambda([this]() { ShowSettings(false); });
	Place(Root, Settings, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(160.f, 0.f));
	Settings->SetVisibility(ESlateVisibility::Collapsed);
}

void UFTTitleMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	if (TitleText)
	{
		TitleText->SetRenderTransformAngle(FMath::Sin(Age * 1.3f) * 1.2f);
		TitleText->SetRenderScale(FVector2D(FMath::Lerp(0.8f, 1.f, Bounce(Age / 0.6f))));
	}
}

void UFTTitleMenuWidget::ShowSettings(bool bShow)
{
	Settings->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	MainColumn->SetVisibility(bShow ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UFTTitleMenuWidget::HandlePlay()
{
	FTAudio::Play2D(this, EFTSound::Clapper, 1.f);
	if (AFTPlayerController* P = PC())
	{
		P->StartDemo(false);
	}
}

void UFTTitleMenuWidget::HandleHost()
{
	FTAudio::Play2D(this, EFTSound::Clapper, 1.f);
	if (AFTPlayerController* P = PC())
	{
		P->StartDemo(true);
	}
}

void UFTTitleMenuWidget::HandleJoinToggle()
{
	Click(this);
	JoinRow->SetVisibility(JoinRow->IsVisible() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UFTTitleMenuWidget::HandleConnect()
{
	Click(this);
	const FString Addr = Address->GetText().ToString().TrimStartAndEnd();
	if (Addr.IsEmpty())
	{
		return;
	}
	UFTSettings::Get()->LastJoinAddress = Addr;
	UFTSettings::Get()->Save();
	if (AFTPlayerController* P = PC())
	{
		P->JoinStudio(Addr);
	}
}

void UFTTitleMenuWidget::HandleSettings()
{
	Click(this);
	ShowSettings(true);
}

void UFTTitleMenuWidget::HandleQuit()
{
	Click(this);
	if (AFTPlayerController* P = PC())
	{
		P->QuitGame();
	}
}

// ============================================================================ pause

AFTPlayerController* UFTPauseMenuWidget::PC() const
{
	return Cast<AFTPlayerController>(GetOwningPlayer());
}

void UFTPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = MakeRoot(T);
	Fullscreen(Root, Swatch(T, Shade, FVector2D(10.f)));

	UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
	AddV(V, Text(T, LOCTEXT("Paused", "PAUSED"), 56, Cream, TEXT("Black"), true), FMargin(0.f, 0.f, 0.f, 4.f), 1);
	AddV(V, Text(T, LOCTEXT("StillRunning", "The shoot keeps running for everyone else!"), 16, Amber, TEXT("Bold")), FMargin(0.f, 0.f, 0.f, 18.f), 1);
	ResumeButton = Button(T, LOCTEXT("Resume", "RESUME"), Coral, 24);
	UButton* Controls = Button(T, LOCTEXT("ControlsBtn", "SHOW / HIDE CONTROLS"), Cream, 18);
	UButton* Sett = Button(T, LOCTEXT("SettingsPause", "SETTINGS"), Cream, 18);
	RestartButton = Button(T, LOCTEXT("Restart", "RESTART SHOOT (BACK TO LOBBY)"), Teal, 18, &RestartLabel, Cream);
	UButton* Leave = Button(T, LOCTEXT("Leave", "LEAVE TO TITLE"), PaperDark, 18);
	UButton* Quit = Button(T, LOCTEXT("QuitPause", "QUIT GAME"), PaperDark, 18);
	for (UButton* B : { ResumeButton.Get(), Controls, Sett, RestartButton.Get(), Leave, Quit })
	{
		AddV(V, Sized(T, B, 460.f), FMargin(0.f, 6.f), 1);
	}
	ResumeButton->OnClicked.AddDynamic(this, &UFTPauseMenuWidget::HandleResume);
	Controls->OnClicked.AddDynamic(this, &UFTPauseMenuWidget::HandleControls);
	Sett->OnClicked.AddDynamic(this, &UFTPauseMenuWidget::HandleSettings);
	RestartButton->OnClicked.AddDynamic(this, &UFTPauseMenuWidget::HandleRestart);
	Leave->OnClicked.AddDynamic(this, &UFTPauseMenuWidget::HandleLeave);
	Quit->OnClicked.AddDynamic(this, &UFTPauseMenuWidget::HandleQuit);
	MainColumn = V;
	Place(Root, V, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	Settings = CreateWidget<UFTSettingsPanel>(GetOwningPlayer(), UFTSettingsPanel::StaticClass());
	Settings->OnBack.BindLambda([this]()
	{
		Settings->SetVisibility(ESlateVisibility::Collapsed);
		MainColumn->SetVisibility(ESlateVisibility::Visible);
	});
	Place(Root, Settings, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	Settings->SetVisibility(ESlateVisibility::Collapsed);
}

void UFTPauseMenuWidget::Refresh()
{
	APlayerController* P = GetOwningPlayer();
	const bool bHost = P && P->HasAuthority();
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	const bool bEnded = GS && (GS->ShootPhase == EFTShootPhase::Results || GS->ShootPhase == EFTShootPhase::Failed);
	RestartButton->SetIsEnabled(bHost || bEnded);
	RestartLabel->SetText(bHost || bEnded ? LOCTEXT("RestartOk", "RESTART SHOOT (BACK TO LOBBY)") : LOCTEXT("RestartHost", "RESTART SHOOT (HOST ONLY)"));
	Settings->SetVisibility(ESlateVisibility::Collapsed);
	MainColumn->SetVisibility(ESlateVisibility::Visible);
	ResumeButton->SetKeyboardFocus();
}

void UFTPauseMenuWidget::HandleResume()
{
	Click(this);
	if (AFTPlayerController* P = PC()) { P->ClosePause(); }
}

void UFTPauseMenuWidget::HandleControls()
{
	Click(this);
	if (AFTPlayerController* P = PC()) { P->ToggleHelp(); }
}

void UFTPauseMenuWidget::HandleSettings()
{
	Click(this);
	Settings->SetVisibility(ESlateVisibility::Visible);
	MainColumn->SetVisibility(ESlateVisibility::Collapsed);
}

void UFTPauseMenuWidget::HandleRestart()
{
	Click(this);
	if (AFTPlayerController* P = PC())
	{
		P->ServerRequestRestart();
		P->ClosePause();
	}
}

void UFTPauseMenuWidget::HandleLeave()
{
	Click(this);
	if (AFTPlayerController* P = PC()) { P->LeaveToTitle(); }
}

void UFTPauseMenuWidget::HandleQuit()
{
	Click(this);
	if (AFTPlayerController* P = PC()) { P->QuitGame(); }
}

// ============================================================================ script book

AFTPlayerController* UFTScriptBookWidget::PC() const
{
	return Cast<AFTPlayerController>(GetOwningPlayer());
}

void UFTScriptBookWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = MakeRoot(T);
	Fullscreen(Root, Swatch(T, FLinearColor(0.01f, 0.01f, 0.03f, 0.6f), FVector2D(10.f)));
	Films.Reset();
	for (const UFTFilmDefinition* F : UFTFilmDefinition::All())
	{
		Films.Add(const_cast<UFTFilmDefinition*>(F));
	}

	// book cover behind the pages
	UBorder* Cover = Box(T, Teal, 22.f, FMargin(20.f, 18.f), Ink, 4.f);
	UHorizontalBox* Spread = T->ConstructWidget<UHorizontalBox>();
	Cover->SetContent(Spread);

	// left page: film tabs
	UVerticalBox* Left = T->ConstructWidget<UVerticalBox>();
	AddV(Left, Text(T, LOCTEXT("Scripts", "THE SCRIPTS"), 30, Ink, TEXT("Black")), FMargin(0.f, 0.f, 0.f, 4.f));
	AddV(Left, Text(T, LOCTEXT("Pick", "Pick tonight's picture. The premiere is at 06:00."), 15, Muted, TEXT("Italic")), FMargin(0.f, 0.f, 0.f, 16.f));
	for (int32 i = 0; i < Films.Num() && i < 3; ++i)
	{
		const UFTFilmDefinition* F = Films[i];
		UButton* B = T->ConstructWidget<UButton>();
		FButtonStyle Style;
		Style.SetNormal(Round(PaperDark, 12.f, Ink, 2.f));
		Style.SetHovered(Round(Cream, 12.f, Ink, 3.f));
		Style.SetPressed(Round(Amber, 12.f, Ink, 3.f));
		Style.SetNormalPadding(FMargin(12.f, 10.f));
		Style.SetPressedPadding(FMargin(12.f, 10.f));
		B->SetStyle(Style);
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		AddH(Row, Swatch(T, F->PosterPrimary, FVector2D(40.f, 56.f), 6.f), FMargin(0.f, 0.f, 12.f, 0.f));
		UVerticalBox* Info = T->ConstructWidget<UVerticalBox>();
		AddV(Info, Text(T, F->Title, 20, Ink, TEXT("Black")), FMargin(0.f));
		AddV(Info, Text(T, F->Genre, 13, Muted, TEXT("Bold")), FMargin(0.f));
		AddH(Row, Info, FMargin(0.f), true);
		if (!F->bPlayable)
		{
			UBorder* Lock = Box(T, Coral, 6.f, FMargin(8.f, 2.f), Ink, 2.f);
			Lock->SetContent(Text(T, LOCTEXT("LockedTag", "LOCKED"), 12, Ink, TEXT("Black")));
			Lock->SetRenderTransformAngle(-8.f);
			AddH(Row, Lock);
		}
		B->SetContent(Row);
		AddV(Left, Sized(T, B, 420.f), FMargin(0.f, 6.f));
		FilmButtons.Add(B);
	}
	if (FilmButtons.IsValidIndex(0)) FilmButtons[0]->OnClicked.AddDynamic(this, &UFTScriptBookWidget::HandleFilm0);
	if (FilmButtons.IsValidIndex(1)) FilmButtons[1]->OnClicked.AddDynamic(this, &UFTScriptBookWidget::HandleFilm1);
	if (FilmButtons.IsValidIndex(2)) FilmButtons[2]->OnClicked.AddDynamic(this, &UFTScriptBookWidget::HandleFilm2);
	UBorder* LeftPage = Box(T, Paper, 14.f, FMargin(26.f, 24.f), Ink, 2.f);
	LeftPage->SetContent(Left);
	AddH(Spread, Sized(T, LeftPage, 480.f, 640.f), FMargin(0.f, 0.f, 6.f, 0.f));

	// right page: details
	UVerticalBox* Right = T->ConstructWidget<UVerticalBox>();
	DetailBox = T->ConstructWidget<UVerticalBox>();
	// details scroll inside the fixed-height page so the buttons always stay on the page
	UScrollBox* DetailScroll = T->ConstructWidget<UScrollBox>();
	DetailScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	DetailScroll->SetScrollbarThickness(FVector2D(6.f, 6.f));
	DetailScroll->AddChild(DetailBox);
	if (UVerticalBoxSlot* ScrollSlot = Right->AddChildToVerticalBox(DetailScroll))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	StatusText = Text(T, FText::GetEmpty(), 15, CoralDarkText(), TEXT("Bold"));
	StatusText->SetAutoWrapText(false);
	StatusText->SetWrapTextAt(520.f);
	AddV(Right, StatusText, FMargin(0.f, 4.f));
	UHorizontalBox* Buttons = T->ConstructWidget<UHorizontalBox>();
	GreenlightButton = Button(T, LOCTEXT("Greenlight", "GREENLIGHT THIS SCRIPT"), Coral, 22, &GreenlightLabel);
	GreenlightButton->OnClicked.AddDynamic(this, &UFTScriptBookWidget::HandleGreenlight);
	AddH(Buttons, GreenlightButton, FMargin(0.f, 0.f, 12.f, 0.f));
	UButton* Close = Button(T, LOCTEXT("Close", "CLOSE BOOK"), PaperDark, 18);
	Close->OnClicked.AddDynamic(this, &UFTScriptBookWidget::HandleClose);
	AddH(Buttons, Close);
	AddV(Right, Buttons, FMargin(0.f, 10.f, 0.f, 0.f));
	UBorder* RightPage = Box(T, Paper, 14.f, FMargin(26.f, 24.f), Ink, 2.f);
	RightPage->SetContent(Right);
	AddH(Spread, Sized(T, RightPage, 600.f, 640.f), FMargin(6.f, 0.f, 0.f, 0.f));

	BookRoot = Cover;
	Cover->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Place(Root, Cover, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	SelectFilm(0);
}

void UFTScriptBookWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	if (BookRoot)
	{
		BookRoot->SetRenderScale(FVector2D(FMath::Lerp(0.85f, 1.f, Bounce(Age / 0.4f))));
		BookRoot->SetRenderOpacity(FMath::Clamp(Age / 0.15f, 0.f, 1.f));
	}
}

void UFTScriptBookWidget::Refresh()
{
	Age = 0.f;
	SelectFilm(Selected);
}

void UFTScriptBookWidget::HandleFilm0() { Click(this); SelectFilm(0); }
void UFTScriptBookWidget::HandleFilm1() { Click(this); SelectFilm(1); }
void UFTScriptBookWidget::HandleFilm2() { Click(this); SelectFilm(2); }

void UFTScriptBookWidget::SelectFilm(int32 Index)
{
	if (!Films.IsValidIndex(Index))
	{
		return;
	}
	Selected = Index;
	const UFTFilmDefinition* F = Films[Index];
	UWidgetTree* T = WidgetTree;
	DetailBox->ClearChildren();

	UBorder* Poster = Box(T, F->PosterPrimary, 10.f, FMargin(18.f, 12.f), Ink, 3.f);
	UVerticalBox* PV = T->ConstructWidget<UVerticalBox>();
	AddV(PV, Text(T, F->Title, 32, Cream, TEXT("Black"), true), FMargin(0.f));
	AddV(PV, Text(T, F->PosterTagline, 15, F->PosterSecondary, TEXT("Black")), FMargin(0.f));
	Poster->SetContent(PV);
	Poster->SetRenderTransformAngle(-1.f);
	AddV(DetailBox, Poster, FMargin(0.f, 0.f, 0.f, 10.f), 3);

	auto Line = [&](const FText& Label, const FText& Value)
	{
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		AddH(Row, Sized(T, Text(T, Label, 14, TealDark, TEXT("Black")), 110.f), FMargin(0.f), false, 0);
		UTextBlock* VT = Text(T, Value, 15, Ink, TEXT("Regular"));
		VT->SetAutoWrapText(false);
		VT->SetWrapTextAt(420.f);
		AddH(Row, VT, FMargin(0.f), true, 0);
		AddV(DetailBox, Row, FMargin(0.f, 2.f));
	};
	Line(LOCTEXT("Genre", "GENRE"), F->Genre);
	Line(LOCTEXT("Logline", "LOGLINE"), F->Logline);
	Line(LOCTEXT("Runtime", "RUNTIME"), F->Runtime);
	Line(LOCTEXT("Hazard", "HAZARD"), F->SignatureHazard);
	Line(LOCTEXT("Effect", "EFFECTS"), F->SignatureEffect);
	UHorizontalBox* Diff = T->ConstructWidget<UHorizontalBox>();
	AddH(Diff, Sized(T, Text(T, LOCTEXT("Difficulty", "DIFFICULTY"), 14, TealDark, TEXT("Black")), 110.f));
	for (int32 i = 0; i < 5; ++i)
	{
		AddH(Diff, Swatch(T, i < F->Difficulty ? Coral : PaperDark, FVector2D(18.f, 18.f), 4.f), FMargin(2.f, 0.f));
	}
	AddV(DetailBox, Diff, FMargin(0.f, 4.f, 0.f, 10.f));

	AddV(DetailBox, Text(T, LOCTEXT("SceneList", "SCENES"), 16, Ink, TEXT("Black")), FMargin(0.f, 0.f, 0.f, 2.f));
	for (const FFTSceneDefinition& S : F->Scenes)
	{
		AddV(DetailBox, Text(T, S.Title, 16, Ink, TEXT("Bold")), FMargin(8.f, 2.f, 0.f, 0.f));
		UTextBlock* D = Text(T, S.ScriptDescription, 13, Muted, TEXT("Italic"));
		D->SetAutoWrapText(false);
		D->SetWrapTextAt(500.f);
		AddV(DetailBox, D, FMargin(20.f, 0.f, 0.f, 2.f));
	}

	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	const bool bSelectablePhase = !GS || GS->ShootPhase == EFTShootPhase::Lobby || GS->ShootPhase == EFTShootPhase::Results || GS->ShootPhase == EFTShootPhase::Failed;
	FText Status;
	bool bCan = F->bPlayable && bSelectablePhase;
	if (!F->bPlayable)
	{
		Status = F->LockedReason;
	}
	else if (!bSelectablePhase && GS)
	{
		const FFTSceneDefinition* Cur = GS->GetCurrentScene();
		Status = FText::Format(LOCTEXT("InProgress", "Shoot in progress: {0}. Restart from the pause menu to pick again."), Cur ? Cur->Title : LOCTEXT("Finale", "the finale"));
	}
	StatusText->SetText(Status);
	GreenlightButton->SetIsEnabled(bCan);
	GreenlightLabel->SetText(F->bPlayable ? LOCTEXT("Greenlight2", "GREENLIGHT THIS SCRIPT") : LOCTEXT("Locked", "IN PRE-PRODUCTION"));
	for (int32 i = 0; i < FilmButtons.Num(); ++i)
	{
		FilmButtons[i]->SetRenderTranslation(FVector2D(i == Selected ? 14.f : 0.f, 0.f));
	}
}

void UFTScriptBookWidget::HandleGreenlight()
{
	if (!Films.IsValidIndex(Selected) || !Films[Selected]->bPlayable)
	{
		return;
	}
	FTAudio::Play2D(this, EFTSound::Stamp, 1.f);
	if (AFTPlayerController* P = PC())
	{
		P->ServerSelectFilm(Films[Selected]->FilmId);
	}
}

void UFTScriptBookWidget::HandleClose()
{
	Click(this);
	if (AFTPlayerController* P = PC())
	{
		P->LocalCloseScriptBook(true);
	}
}

// ============================================================================ results

AFTPlayerController* UFTResultsWidget::PC() const
{
	return Cast<AFTPlayerController>(GetOwningPlayer());
}

FText UFTResultsWidget::RatingForTotal(int32 Total, bool bFailed)
{
	if (bFailed || Total < 90) return LOCTEXT("Disasterpiece", "DISASTERPIECE");
	if (Total < 150) return LOCTEXT("BMovie", "B-MOVIE");
	if (Total < 200) return LOCTEXT("Cult", "CULT CLASSIC");
	if (Total < 250) return LOCTEXT("Blockbuster", "BLOCKBUSTER");
	return LOCTEXT("Legend", "STUDIO LEGEND");
}

void UFTResultsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = MakeRoot(T);
	Fullscreen(Root, Swatch(T, Shade, FVector2D(10.f)));
	UBorder* C = Box(T, Paper, 18.f, FMargin(34.f, 26.f), Ink, 4.f);
	Content = T->ConstructWidget<UVerticalBox>();
	UVerticalBox* Outer = T->ConstructWidget<UVerticalBox>();
	AddV(Outer, Content, FMargin(0.f));
	UHorizontalBox* Buttons = T->ConstructWidget<UHorizontalBox>();
	RetryButton = Button(T, LOCTEXT("Retry", "RETRY SHOOT"), Coral, 24);
	RetryButton->OnClicked.AddDynamic(this, &UFTResultsWidget::HandleRetry);
	AddH(Buttons, RetryButton, FMargin(0.f, 0.f, 12.f, 0.f));
	UButton* Leave = Button(T, LOCTEXT("LeaveTitle", "LEAVE TO TITLE"), PaperDark, 20);
	Leave->OnClicked.AddDynamic(this, &UFTResultsWidget::HandleLeave);
	AddH(Buttons, Leave);
	AddV(Outer, Buttons, FMargin(0.f, 18.f, 0.f, 0.f), 1);
	C->SetContent(Outer);
	C->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Card = C;
	Place(Root, Sized(T, C, 820.f), FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

void UFTResultsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	Card->SetRenderScale(FVector2D(FMath::Lerp(0.7f, 1.f, Bounce(Age / 0.45f))));
}

void UFTResultsWidget::Refresh()
{
	Age = 0.f;
	UWidgetTree* T = WidgetTree;
	Content->ClearChildren();
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS)
	{
		return;
	}
	const bool bFailed = GS->ShootPhase == EFTShootPhase::Failed;
	const UFTFilmDefinition* Film = GS->GetFilm();
	AddV(Content, Text(T, bFailed ? LOCTEXT("BlackoutT", "BLACKOUT!") : LOCTEXT("Reviews", "THE REVIEWS ARE IN"), 44, bFailed ? Red : Ink, TEXT("Black")), FMargin(0.f), 1);
	if (bFailed)
	{
		AddV(Content, Text(T, GS->FailReason, 18, CoralDarkText(), TEXT("Bold")), FMargin(0.f, 0.f, 0.f, 10.f), 1);
	}
	if (Film)
	{
		AddV(Content, Text(T, Film->Title, 22, TealDark, TEXT("Black")), FMargin(0.f, 0.f, 0.f, 12.f), 1);
		for (int32 i = 0; i < Film->Scenes.Num(); ++i)
		{
			UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
			AddH(Row, Text(T, Film->Scenes[i].Title, 18, Ink, TEXT("Bold")), FMargin(0.f), true);
			const FFTTakeResult* Best = GS->GetBestTake(i);
			UBorder* Stamp = Box(T, Best ? RatingColor(Best->Rating) : PaperDark, 8.f, FMargin(10.f, 2.f), Ink, 2.f);
			Stamp->SetContent(Text(T, Best ? RatingName(Best->Rating) : LOCTEXT("NotShot", "NOT SHOT"), 14, Ink, TEXT("Black")));
			Stamp->SetRenderTransformAngle(-4.f);
			AddH(Row, Stamp, FMargin(10.f, 0.f));
			AddH(Row, Sized(T, Text(T, Best ? FText::Format(LOCTEXT("Pts", "{0} pts"), FText::AsNumber(Best->Score)) : FText::FromString(TEXT("-")), 18, Ink, TEXT("Black")), 90.f));
			AddV(Content, Row, FMargin(0.f, 4.f), 3);
		}
	}
	const int32 Total = GS->TeamScore + GS->DisasterBonus;
	auto Stat = [&](const FText& L, int32 V)
	{
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		AddH(Row, Text(T, L, 16, Muted, TEXT("Bold")), FMargin(0.f), true);
		AddH(Row, Text(T, FText::AsNumber(V), 16, Ink, TEXT("Black")));
		AddV(Content, Row, FMargin(0.f, 1.f), 3);
	};
	AddV(Content, Swatch(T, PaperDark, FVector2D(10.f, 3.f)), FMargin(0.f, 10.f), 3);
	Stat(LOCTEXT("Takes", "Completed takes"), GS->GetCompletedTakeCount());
	Stat(LOCTEXT("TakeScore", "Take score (incl. style)"), GS->TeamScore);
	Stat(LOCTEXT("Style", "Style bonus earned"), GS->StyleBonus);
	Stat(LOCTEXT("Disaster", "Disaster bonus"), GS->DisasterBonus);
	Stat(LOCTEXT("Total", "TOTAL"), Total);
	UBorder* Rating = Box(T, bFailed ? Coral : Teal, 12.f, FMargin(20.f, 6.f), Ink, 4.f);
	Rating->SetContent(Text(T, RatingForTotal(Total, bFailed), 36, Cream, TEXT("Black"), true));
	Rating->SetRenderTransformAngle(-3.f);
	AddV(Content, Rating, FMargin(0.f, 14.f), 1);

	FString Names;
	for (APlayerState* P : GS->PlayerArray)
	{
		if (const AFTPlayerState* PS = Cast<AFTPlayerState>(P))
		{
			Names += (Names.IsEmpty() ? TEXT("") : TEXT("  -  ")) + PS->GetCrewName();
		}
	}
	AddV(Content, Text(T, LOCTEXT("Credits", "A Film by The Final Take Crew"), 16, Ink, TEXT("Black")), FMargin(0.f, 6.f, 0.f, 0.f), 1);
	AddV(Content, Text(T, FText::FromString(Names), 15, TealDark, TEXT("Bold")), FMargin(0.f), 1);
	RetryButton->SetKeyboardFocus();
}

void UFTResultsWidget::HandleRetry()
{
	FTAudio::Play2D(this, EFTSound::Clapper, 1.f);
	if (AFTPlayerController* P = PC())
	{
		P->ServerRequestRestart();
	}
}

void UFTResultsWidget::HandleLeave()
{
	Click(this);
	if (AFTPlayerController* P = PC())
	{
		P->LeaveToTitle();
	}
}

// ============================================================================ premiere montage

void UFTPremiereWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = MakeRoot(T);
	Backdrop = Box(T, Ink, 0.f, FMargin(0.f), Ink, 0.f);
	Fullscreen(Root, Backdrop);
	StillFrame = Box(T, Cream, 8.f, FMargin(10.f), Cream, 0.f);
	Still = T->ConstructWidget<UImage>();
	StillFrame->SetContent(Sized(T, Still, 960.f, 540.f));
	Place(Root, StillFrame, FAnchors(0.5f, 0.56f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	BigText = Text(T, FText::GetEmpty(), 96, Cream, TEXT("Black"), true);
	BigText->SetJustification(ETextJustify::Center);
	Place(Root, BigText, FAnchors(0.5f, 0.12f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	SubText = Text(T, FText::GetEmpty(), 44, Amber, TEXT("Bold"));
	SubText->SetJustification(ETextJustify::Center);
	SubText->SetAutoWrapText(false);
	SubText->SetWrapTextAt(1600.f);
	// bottom-anchored so a two-line description grows upwards instead of off the screen
	Place(Root, SubText, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -24.f));
	Stamp = Box(T, Teal, 16.f, FMargin(30.f, 10.f), Cream, 6.f);
	StampText = Text(T, FText::GetEmpty(), 72, Cream, TEXT("Black"), true);
	Stamp->SetContent(StampText);
	Stamp->SetRenderTransformAngle(-8.f);
	Place(Root, Stamp, FAnchors(0.78f, 0.7f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

void UFTPremiereWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS)
	{
		return;
	}
	const UFTFilmDefinition* Film = GS->GetFilm();
	const bool bShowing = GS->ShootPhase == EFTShootPhase::Premiere || GS->ShootPhase == EFTShootPhase::Results;
	if (!bShowing || !Film)
	{
		Backdrop->SetBrushColor(FLinearColor(0.93f, 0.93f, 0.9f, 1.f));
		BigText->SetText(FText::GetEmpty());
		SubText->SetText(FText::GetEmpty());
		StillFrame->SetVisibility(ESlateVisibility::Collapsed);
		Stamp->SetVisibility(ESlateVisibility::Collapsed);
		LastCard = -2;
		return;
	}
	const float T = GS->GetServerWorldTimeSeconds() - GS->PremiereStartTime;
	const int32 Scenes = Film->Scenes.Num();
	int32 Card = -1; // -1 title, 0..N-1 scenes, N credits
	if (T > 8.f)
	{
		Card = FMath::Min(FMath::FloorToInt((T - 8.f) / 6.f), Scenes);
	}
	if (GS->ShootPhase == EFTShootPhase::Results)
	{
		Card = Scenes;
	}
	if (Card != LastCard)
	{
		LastCard = Card;
		Stamp->SetVisibility(ESlateVisibility::Collapsed);
		StillFrame->SetVisibility(ESlateVisibility::Collapsed);
		if (Card < 0)
		{
			Backdrop->SetBrushColor(Film->PosterPrimary * 0.6f + FLinearColor(0.f, 0.f, 0.f, 1.f));
			BigText->SetText(Film->Title);
			SubText->SetText(Film->PosterTagline);
		}
		else if (Card < Scenes)
		{
			Backdrop->SetBrushColor(FTColors::Hex(0x151A33));
			BigText->SetText(Film->Scenes[Card].Title);
			const FFTTakeResult* Best = GS->GetBestTake(Card);
			SubText->SetText(Film->Scenes[Card].ScriptDescription);
			if (TObjectPtr<UTextureRenderTarget2D>* RT = GS->LocalStills.Find(Card))
			{
				Still->SetBrushResourceObject(*RT);
				Still->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				Still->SetBrushResourceObject(nullptr);
				Still->SetColorAndOpacity(Film->PosterPrimary);
			}
			StillFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (Best)
			{
				Stamp->SetVisibility(ESlateVisibility::HitTestInvisible);
				Stamp->SetBrush(Round(RatingColor(Best->Rating), 16.f, Cream, 6.f));
				StampText->SetText(FText::Format(LOCTEXT("PremStamp", "{0}  {1}"), RatingName(Best->Rating), FText::AsNumber(Best->Score)));
				const EFTSound Reaction = Best->Rating == EFTTakeRating::Perfect ? EFTSound::ApplauseBig
					: (Best->Rating == EFTTakeRating::Great ? EFTSound::ApplauseSmall : EFTSound::Laugh);
				FTAudio::Play2D(this, Reaction, 0.7f);
			}
			else
			{
				FTAudio::Play2D(this, EFTSound::Groan, 0.6f);
			}
		}
		else
		{
			Backdrop->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.f));
			BigText->SetText(LOCTEXT("PremCredits", "A Film by The Final Take Crew"));
			FString Names;
			for (APlayerState* P : GS->PlayerArray)
			{
				if (const AFTPlayerState* PS = Cast<AFTPlayerState>(P))
				{
					Names += (Names.IsEmpty() ? TEXT("") : TEXT("   ")) + PS->GetCrewName();
				}
			}
			SubText->SetText(FText::FromString(Names));
			FTAudio::Play2D(this, EFTSound::ApplauseBig, 0.8f);
		}
	}
	if (Card >= 0 && Card < Scenes)
	{
		const float Local = FMath::Fmod(T - 8.f, 6.f);
		Stamp->SetRenderScale(FVector2D(FMath::Lerp(2.f, 1.f, Bounce((Local - 1.5f) / 0.3f))));
		Stamp->SetRenderOpacity(Local > 1.5f ? 1.f : 0.f);
	}
}

#undef LOCTEXT_NAMESPACE
