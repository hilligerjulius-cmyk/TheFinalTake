#include "TheFinalTake/UI/FTHUDWidget.h"

#include "TheFinalTake/UI/FTUIStyle.h"
#include "TheFinalTake/Core/FTVisuals.h"
#include "TheFinalTake/Characters/FTCharacter.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"
#include "TheFinalTake/Game/FTPlayerState.h"
#include "TheFinalTake/Interaction/FTInteractableComponent.h"
#include "TheFinalTake/Production/FTFilmCamera.h"
#include "TheFinalTake/Props/FTProp.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FinalTakeHUD"

using namespace FTUI;

namespace
{
	FText StateLabel(EFTSceneState S)
	{
		switch (S)
		{
		case EFTSceneState::Preparation: return LOCTEXT("SPrep", "PREP");
		case EFTSceneState::ReadyToRoll: return LOCTEXT("SReady", "READY TO ROLL");
		case EFTSceneState::Recording: return LOCTEXT("SRec", "REC");
		case EFTSceneState::TakeComplete: return LOCTEXT("STake", "TAKE COMPLETE");
		case EFTSceneState::Transition: return LOCTEXT("STrans", "RESETTING SET");
		}
		return FText::GetEmpty();
	}

	FLinearColor StateColor(EFTSceneState S)
	{
		switch (S)
		{
		case EFTSceneState::ReadyToRoll: return Teal;
		case EFTSceneState::Recording: return Red;
		case EFTSceneState::TakeComplete: return Yellow;
		case EFTSceneState::Transition: return Amber;
		default: return PaperDark;
		}
	}

	FString CostumeName(EFTCostume C)
	{
		switch (C)
		{
		case EFTCostume::Lifeguard: return TEXT("LIFEGUARD");
		case EFTCostume::Shark: return TEXT("SHARK");
		case EFTCostume::Raincoat: return TEXT("RAINCOAT");
		case EFTCostume::FoamKnight: return TEXT("KNIGHT");
		default: return FString();
		}
	}
}

void UFTHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Build();
}

void UFTHUDWidget::Build()
{
	UWidgetTree* T = WidgetTree;
	Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;

	// ------------------------------------------------ danger edges (power out / flood)
	{
		const FAnchors Edges[4] = { FAnchors(0.f, 0.f, 1.f, 0.f), FAnchors(0.f, 1.f, 1.f, 1.f), FAnchors(0.f, 0.f, 0.f, 1.f), FAnchors(1.f, 0.f, 1.f, 1.f) };
		const FVector2D Align[4] = { FVector2D(0.f, 0.f), FVector2D(0.f, 1.f), FVector2D(0.f, 0.f), FVector2D(1.f, 0.f) };
		const FVector2D Size[4] = { FVector2D(0.f, 14.f), FVector2D(0.f, 14.f), FVector2D(14.f, 0.f), FVector2D(14.f, 0.f) };
		for (int32 i = 0; i < 4; ++i)
		{
			UImage* E = T->ConstructWidget<UImage>();
			E->SetColorAndOpacity(FLinearColor(Red.R, Red.G, Red.B, 0.f));
			UCanvasPanelSlot* S = Root->AddChildToCanvas(E);
			S->SetAnchors(Edges[i]);
			S->SetAlignment(Align[i]);
			S->SetOffsets(FMargin(0.f, 0.f, Size[i].X, Size[i].Y));
			E->SetVisibility(ESlateVisibility::HitTestInvisible);
			DangerEdges.Add(E);
		}
	}

	// ------------------------------------------------ camera viewfinder layer
	{
		CameraLayer = T->ConstructWidget<UCanvasPanel>();
		UCanvasPanelSlot* S = Root->AddChildToCanvas(CameraLayer);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
		// 2.39:1 letterbox on a 16:9 screen; the REC line and hints sit on the bars like on a real monitor
		for (int32 i = 0; i < 2; ++i)
		{
			UCanvasPanelSlot* BarSlot = CameraLayer->AddChildToCanvas(Swatch(T, FLinearColor(0.f, 0.f, 0.f, 1.f), FVector2D(10.f)));
			BarSlot->SetAnchors(i == 0 ? FAnchors(0.f, 0.f, 1.f, 0.128f) : FAnchors(0.f, 0.872f, 1.f, 1.f));
			BarSlot->SetOffsets(FMargin(0.f));
		}
		const float L = 70.f, W = 6.f, M = 60.f;
		struct FCorner { FAnchors A; FVector2D Al; FVector2D P; };
		const FCorner Corners[4] = {
			{ FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(M, M) },
			{ FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-M, M) },
			{ FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(M, -M) },
			{ FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-M, -M) } };
		for (const FCorner& C : Corners)
		{
			Place(CameraLayer, Swatch(T, FLinearColor(1.f, 1.f, 1.f, 0.9f), FVector2D(L, W)), C.A, C.Al, C.P, FVector2D(L, W));
			Place(CameraLayer, Swatch(T, FLinearColor(1.f, 1.f, 1.f, 0.9f), FVector2D(W, L)), C.A, C.Al, C.P, FVector2D(W, L));
		}
		// thirds guides
		for (float X : { 1.f / 3.f, 2.f / 3.f })
		{
			UImage* G = Swatch(T, FLinearColor(1.f, 1.f, 1.f, 0.12f), FVector2D(2.f, 10.f));
			UCanvasPanelSlot* GS = CameraLayer->AddChildToCanvas(G);
			GS->SetAnchors(FAnchors(X, 0.1f, X, 0.9f));
			GS->SetOffsets(FMargin(-1.f, 0.f, 2.f, 0.f));
		}
		UHorizontalBox* RecRow = T->ConstructWidget<UHorizontalBox>();
		RecDot = Swatch(T, Red, FVector2D(22.f, 22.f), 11.f);
		AddH(RecRow, RecDot, FMargin(0.f, 0.f, 10.f, 0.f));
		RecText = Text(T, LOCTEXT("Rec", "REC"), 26, White, TEXT("Black"), true);
		AddH(RecRow, RecText, FMargin(0.f, 0.f, 22.f, 0.f));
		Timecode = Text(T, FText::FromString(TEXT("00:00:00")), 24, White, TEXT("Bold"), true);
		AddH(RecRow, Timecode);
		Place(CameraLayer, RecRow, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(M + 24.f, M + 20.f));

		CameraStatus = Text(T, FText::GetEmpty(), 20, Yellow, TEXT("Black"), true);
		Place(CameraLayer, CameraStatus, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, M + 18.f));

		UVerticalBox* Panel = T->ConstructWidget<UVerticalBox>();
		UHorizontalBox* FrameRow = T->ConstructWidget<UHorizontalBox>();
		FrameText = Text(T, LOCTEXT("Frame", "FRAME"), 16, White, TEXT("Black"), true);
		AddH(FrameRow, FrameText, FMargin(0.f, 0.f, 10.f, 0.f));
		USizeBox* FrameSize = T->ConstructWidget<USizeBox>();
		FrameSize->SetWidthOverride(260.f);
		FrameSize->SetHeightOverride(16.f);
		FrameBar = Bar(T, Teal, FLinearColor(0.f, 0.f, 0.f, 0.5f), 16.f);
		FrameSize->AddChild(FrameBar);
		AddH(FrameRow, FrameSize);
		AddV(Panel, FrameRow);
		SubjectList = T->ConstructWidget<UVerticalBox>();
		for (int32 i = 0; i < 6; ++i)
		{
			UTextBlock* Line = Text(T, FText::GetEmpty(), 16, White, TEXT("Black"), true);
			AddV(SubjectList, Line, FMargin(0.f, 1.f));
			Line->SetVisibility(ESlateVisibility::Collapsed);
			SubjectLines.Add(Line);
		}
		AddV(Panel, SubjectList, FMargin(0.f, 6.f));
		CameraHint = Text(T, LOCTEXT("CamHint", "LMB record/cut   Mouse pan/tilt   Wheel zoom   A/D dolly   R recenter   Q step off (keeps rolling)"), 14, Cream, TEXT("Bold"), true);
		AddV(Panel, CameraHint, FMargin(0.f, 8.f));
		Place(CameraLayer, Panel, FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(M + 24.f, -M - 16.f));
		CameraLayer->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ------------------------------------------------ call sheet (top-left)
	{
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
		FilmLine = Text(T, LOCTEXT("NoFilm", "NO SCRIPT GREENLIT"), 14, TealDark, TEXT("BoldCondensed"));
		AddV(V, FilmLine, FMargin(0.f, 0.f, 0.f, 2.f));
		SceneTitle = Text(T, LOCTEXT("Lobby", "Night Shift: Lobby"), 26, Ink, TEXT("Black"));
		AddV(V, SceneTitle, FMargin(0.f, 0.f, 0.f, 6.f));
		StatePill = Box(T, PaperDark, 10.f, FMargin(10.f, 3.f), Ink, 2.f);
		StateText = Text(T, LOCTEXT("Prep", "PREP"), 13, Ink, TEXT("Black"));
		StatePill->SetContent(StateText);
		AddV(V, StatePill, FMargin(0.f, 0.f, 0.f, 8.f));
		ObjectiveList = T->ConstructWidget<UVerticalBox>();
		AddV(V, ObjectiveList);
		UHorizontalBox* CapRow = T->ConstructWidget<UHorizontalBox>();
		USizeBox* CapSize = T->ConstructWidget<USizeBox>();
		CapSize->SetWidthOverride(220.f);
		CapSize->SetHeightOverride(12.f);
		CaptureBar = Bar(T, Red, PaperDark, 12.f);
		CapSize->AddChild(CaptureBar);
		AddH(CapRow, CapSize, FMargin(0.f, 0.f, 8.f, 0.f));
		CaptureText = Text(T, FText::GetEmpty(), 13, Ink, TEXT("Bold"));
		AddH(CapRow, CaptureText);
		AddV(V, CapRow, FMargin(0.f, 8.f, 0.f, 0.f));
		CallSheet = Box(T, Paper, 12.f, FMargin(18.f, 14.f, 20.f, 16.f), Ink, 3.f);
		CallSheet->SetContent(V);
		CallSheet->SetRenderTransformAngle(-1.2f);
		USizeBox* SheetSize = T->ConstructWidget<USizeBox>();
		SheetSize->SetWidthOverride(400.f);
		SheetSize->AddChild(CallSheet);
		Place(Root, SheetSize, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(30.f, 34.f));
		UImage* Tape = Swatch(T, FLinearColor(1.f, 0.93f, 0.7f, 0.75f), FVector2D(110.f, 26.f));
		Tape->SetRenderTransformAngle(-4.f);
		Place(Root, Tape, FAnchors(0.f, 0.f), FVector2D(0.5f, 0.5f), FVector2D(230.f, 34.f), FVector2D(110.f, 26.f));
	}

	// ------------------------------------------------ callout banner (top-center)
	{
		CalloutBanner = Box(T, Amber, 18.f, FMargin(22.f, 8.f), Ink, 3.f);
		CalloutText = Text(T, FText::GetEmpty(), 18, Ink, TEXT("Bold"));
		// fixed wrap width: auto-wrap inside an auto-sized canvas slot collapses to one word per line
		CalloutText->SetAutoWrapText(false);
		CalloutText->SetWrapTextAt(760.f);
		CalloutText->SetJustification(ETextJustify::Center);
		CalloutBanner->SetContent(CalloutText);
		Place(Root, CalloutBanner, FAnchors(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 26.f));
	}

	// ------------------------------------------------ clock / condition / score (top-right)
	{
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
		ClockText = Text(T, FText::FromString(TEXT("00:00 AM")), 32, Cream, TEXT("Black"));
		AddV(V, ClockText, FMargin(0.f), 2);
		ClockSub = Text(T, LOCTEXT("Premiere", "PREMIERE AT 06:00"), 13, Amber, TEXT("BoldCondensed"));
		AddV(V, ClockSub, FMargin(0.f, 0.f, 0.f, 4.f), 2);
		USizeBox* DS = T->ConstructWidget<USizeBox>();
		DS->SetWidthOverride(230.f);
		DS->SetHeightOverride(10.f);
		DawnBar = Bar(T, Amber, FLinearColor(0.1f, 0.1f, 0.2f, 1.f), 10.f);
		DS->AddChild(DawnBar);
		AddV(V, DS, FMargin(0.f, 0.f, 0.f, 10.f), 3);
		ConditionText = Text(T, LOCTEXT("Studio", "STUDIO 100%"), 13, Cream, TEXT("BoldCondensed"));
		AddV(V, ConditionText, FMargin(0.f), 0);
		USizeBox* CS = T->ConstructWidget<USizeBox>();
		CS->SetWidthOverride(230.f);
		CS->SetHeightOverride(12.f);
		ConditionBar = Bar(T, Teal, FLinearColor(0.1f, 0.1f, 0.2f, 1.f), 12.f);
		CS->AddChild(ConditionBar);
		AddV(V, CS, FMargin(0.f, 2.f, 0.f, 10.f), 3);
		ScoreText = Text(T, LOCTEXT("Score0", "SCORE  0"), 24, Yellow, TEXT("Black"));
		AddV(V, ScoreText, FMargin(0.f), 2);
		UBorder* Card = Box(T, Ink, 14.f, FMargin(18.f, 12.f), Cream, 3.f);
		Card->SetContent(V);
		Card->SetRenderTransformAngle(1.f);
		Place(Root, Card, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-30.f, 30.f));

		CrewList = T->ConstructWidget<UVerticalBox>();
		Place(Root, CrewList, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-34.f, 250.f));
	}

	// ------------------------------------------------ crosshair + prompt (center)
	{
		Crosshair = Swatch(T, FLinearColor(1.f, 0.97f, 0.9f, 0.9f), FVector2D(8.f, 8.f), 4.f);
		Place(Root, Crosshair, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector, FVector2D(8.f, 8.f));

		UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		UBorder* Key = Box(T, Ink, 8.f, FMargin(12.f, 4.f), Cream, 2.f);
		PromptKey = Text(T, FText::FromString(TEXT("E")), 20, Cream, TEXT("Black"));
		Key->SetContent(PromptKey);
		AddH(Row, Key, FMargin(0.f, 0.f, 12.f, 0.f));
		PromptText = Text(T, FText::GetEmpty(), 20, Ink, TEXT("Bold"));
		AddH(Row, PromptText);
		AddV(V, Row, FMargin(0.f), 1);
		PromptReason = Text(T, FText::GetEmpty(), 15, Red, TEXT("Bold"));
		AddV(V, PromptReason, FMargin(0.f, 4.f, 0.f, 0.f), 1);
		USizeBox* HS = T->ConstructWidget<USizeBox>();
		HS->SetWidthOverride(200.f);
		HS->SetHeightOverride(10.f);
		HoldBar = Bar(T, Coral, PaperDark, 10.f);
		HS->AddChild(HoldBar);
		AddV(V, HS, FMargin(0.f, 6.f, 0.f, 0.f), 1);
		PromptBox = Box(T, Paper, 14.f, FMargin(14.f, 8.f), Ink, 3.f);
		PromptBox->SetContent(V);
		Place(Root, PromptBox, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.f), FVector2D(0.f, 60.f));

		CarryHint = Text(T, FText::GetEmpty(), 15, Cream, TEXT("Bold"), true);
		Place(Root, CarryHint, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -150.f));
	}

	// ------------------------------------------------ toasts (bottom-center)
	{
		ToastBox = T->ConstructWidget<UVerticalBox>();
		Place(Root, ToastBox, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -40.f));
	}

	// ------------------------------------------------ announcement (clapperboard banner)
	{
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
		UHorizontalBox* Stripes = T->ConstructWidget<UHorizontalBox>();
		for (int32 i = 0; i < 10; ++i)
		{
			UImage* S = Swatch(T, i % 2 ? Cream : Ink, FVector2D(52.f, 22.f));
			FWidgetTransform Sheared;
			Sheared.Shear = FVector2D(-25.f, 0.f);
			S->SetRenderTransform(Sheared);
			AddH(Stripes, S, FMargin(0.f));
		}
		AddV(V, Stripes, FMargin(0.f), 1);
		AnnounceBox = Box(T, Ink, 10.f, FMargin(28.f, 12.f), Cream, 3.f);
		AnnounceText = Text(T, FText::GetEmpty(), 34, Cream, TEXT("Black"));
		AnnounceText->SetJustification(ETextJustify::Center);
		AnnounceText->SetAutoWrapText(false);
		AnnounceText->SetWrapTextAt(900.f);
		AnnounceBox->SetContent(AnnounceText);
		AddV(V, AnnounceBox, FMargin(0.f), 1);
		AnnounceRoot = V;
		V->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Place(Root, V, FAnchors(0.5f, 0.24f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		V->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ------------------------------------------------ take stamp
	{
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
		StampBox = Box(T, Teal, 16.f, FMargin(30.f, 10.f), Ink, 5.f);
		UVerticalBox* SV = T->ConstructWidget<UVerticalBox>();
		StampText = Text(T, FText::GetEmpty(), 58, Cream, TEXT("Black"), true);
		AddV(SV, StampText, FMargin(0.f), 1);
		StampScore = Text(T, FText::GetEmpty(), 22, Cream, TEXT("Black"));
		AddV(SV, StampScore, FMargin(0.f), 1);
		StampBox->SetContent(SV);
		StampBox->SetRenderTransformAngle(-6.f);
		AddV(V, StampBox, FMargin(0.f, 0.f, 0.f, 10.f), 1);
		UBorder* ReasonCard = Box(T, Paper, 10.f, FMargin(16.f, 10.f), Ink, 3.f);
		StampReasons = T->ConstructWidget<UVerticalBox>();
		ReasonCard->SetContent(StampReasons);
		AddV(V, ReasonCard, FMargin(0.f), 1);
		StampRoot = V;
		V->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Place(Root, V, FAnchors(0.5f, 0.52f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		V->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ------------------------------------------------ help card
	{
		UVerticalBox* V = T->ConstructWidget<UVerticalBox>();
		AddV(V, Text(T, LOCTEXT("Controls", "CONTROLS"), 20, Ink, TEXT("Black")), FMargin(0.f, 0.f, 0.f, 6.f));
		const TCHAR* Lines[] = {
			TEXT("WASD  move      Shift  sprint      Space  jump"),
			TEXT("E  use / carry (hold for big switches)"),
			TEXT("Q  drop / step off      LMB  throw / hero thrust"),
			TEXT("F  costume move (shark suit: lunge!)"),
			TEXT("1-4  wave, point, cheer, panic      G  ping"),
			TEXT("Camera: LMB record, mouse aim, wheel zoom, A/D dolly"),
			TEXT("H  toggle this card      Esc / P  pause"),
		};
		for (const TCHAR* L : Lines)
		{
			AddV(V, Text(T, FText::FromString(L), 14, Ink, TEXT("Bold")), FMargin(0.f, 2.f));
		}
		UBorder* Card = Box(T, Paper, 12.f, FMargin(18.f, 14.f), Ink, 3.f);
		Card->SetContent(V);
		Card->SetRenderTransformAngle(1.5f);
		HelpCard = Card;
		Place(Root, Card, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-30.f, -40.f));
	}

	// ------------------------------------------------ ping markers
	for (int32 i = 0; i < 6; ++i)
	{
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		UImage* Dia = Swatch(T, Coral, FVector2D(16.f, 16.f), 3.f);
		Dia->SetRenderTransformAngle(45.f);
		AddH(Row, Dia, FMargin(0.f, 0.f, 6.f, 0.f));
		AddH(Row, Text(T, FText::GetEmpty(), 15, White, TEXT("Black"), true));
		Place(Root, Row, FAnchors(0.f, 0.f), FVector2D(0.1f, 0.5f), FVector2D::ZeroVector);
		Row->SetVisibility(ESlateVisibility::Collapsed);
		PingWidgets.Add(Row);
	}
}

// ============================================================================ public API

void UFTHUDWidget::ShowToast(const FText& InText, bool bError)
{
	UBorder* B = Box(WidgetTree, bError ? Coral : Ink, 12.f, FMargin(16.f, 6.f), bError ? Ink : Cream, 2.f);
	B->SetContent(Text(WidgetTree, InText, 17, bError ? Ink : Cream, TEXT("Bold")));
	AddV(ToastBox, B, FMargin(0.f, 3.f), 1);
	Toasts.Add({ B, 0.f });
	while (Toasts.Num() > 4)
	{
		if (UWidget* Old = Toasts[0].Widget.Get())
		{
			Old->RemoveFromParent();
		}
		Toasts.RemoveAt(0);
	}
}

void UFTHUDWidget::ShowAnnouncement(const FText& InText, EFTAnnounceStyle Style)
{
	if (InText.IsEmpty())
	{
		return;
	}
	AnnounceQueue.Add({ InText, Style });
	if (AnnounceQueue.Num() > 4)
	{
		AnnounceQueue.RemoveAt(0);
	}
}

void UFTHUDWidget::ShowTakeResult(const FFTTakeResult& R)
{
	FLinearColor C = Coral;
	FText Label = LOCTEXT("StampRetake", "RETAKE!");
	switch (R.Rating)
	{
	case EFTTakeRating::Usable: C = Amber; Label = LOCTEXT("StampUsable", "USABLE!"); break;
	case EFTTakeRating::Great: C = Teal; Label = LOCTEXT("StampGreat", "GREAT!"); break;
	case EFTTakeRating::Perfect: C = Magenta; Label = LOCTEXT("StampPerfect", "PERFECT!"); break;
	default: break;
	}
	StampBox->SetBrush(Round(C, 16.f, Ink, 5.f));
	StampText->SetText(Label);
	StampScore->SetText(FText::Format(LOCTEXT("StampScoreFmt", "TAKE {0}  -  {1} / 100"), FText::AsNumber(R.TakeNumber), FText::AsNumber(R.Score)));
	StampReasons->ClearChildren();
	for (int32 i = 0; i < R.Reasons.Num() && i < 8; ++i)
	{
		const FString& Line = R.Reasons[i];
		const FLinearColor LC = Line.StartsWith(TEXT("-")) || Line.StartsWith(TEXT("+0")) ? FTUI::CoralDarkText() : Ink;
		AddV(StampReasons, Text(WidgetTree, FText::FromString(Line), 15, LC, TEXT("Bold")), FMargin(0.f, 1.f));
	}
	StampAge = 0.f;
	StampRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UFTHUDWidget::AddPing(const FVector& Location, int32 ColorIndex, const FString& Name)
{
	Pings.Add({ Location, 0.f, ColorIndex, Name });
	if (Pings.Num() > PingWidgets.Num())
	{
		Pings.RemoveAt(0);
	}
}

void UFTHUDWidget::SetHelpVisible(bool bVisible)
{
	bHelpVisible = bVisible;
	bHelpAutoHide = false;
	HelpCard->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

// ============================================================================ tick

void UFTHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Time += InDeltaTime;
	UpdateCallSheet();
	UpdateClock();
	CrewTimer -= InDeltaTime;
	if (CrewTimer <= 0.f)
	{
		CrewTimer = 0.5f;
		UpdateCrew();
	}
	UpdatePrompt();
	UpdateCamera(InDeltaTime);
	UpdateTransient(InDeltaTime);
	UpdatePings();
}

void UFTHUDWidget::UpdateCallSheet()
{
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS)
	{
		return;
	}
	const UFTFilmDefinition* Film = GS->GetFilm();
	const FFTSceneDefinition* Scene = GS->GetCurrentScene();

	CalloutText->SetText(GS->Callout);

	FString Sig = FString::Printf(TEXT("%d|%d|%d|%s|%d|"), (int32)GS->ShootPhase, GS->SceneIndex, (int32)GS->SceneState, *GS->FilmId.ToString(), GS->ReelsLoaded);
	for (const FFTObjectiveStatus& S : GS->Objectives)
	{
		Sig += FString::Printf(TEXT("%d"), (int32)S.State);
	}
	if (Sig == ObjectiveSignature)
	{
		const bool bRec = GS->SceneState == EFTSceneState::Recording;
		CaptureBar->GetParent()->GetParent()->SetVisibility(bRec ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bRec)
		{
			CaptureBar->SetPercent(GS->CaptureProgress);
			CaptureText->SetText(GS->bKeyActionDone
				? FText::Format(LOCTEXT("HoldFmt", "HOLD  {0}%"), FText::AsNumber(FMath::RoundToInt(GS->CaptureProgress * 100.f)))
				: LOCTEXT("WaitAction", "waiting for the action"));
			const float Pulse = 0.6f + 0.4f * FMath::Abs(FMath::Sin(Time * 4.f));
			StatePill->SetBrush(Round(Red * Pulse + FLinearColor(0.f, 0.f, 0.f, 1.f), 10.f, Ink, 2.f));
		}
		return;
	}
	ObjectiveSignature = Sig;

	ObjectiveList->ClearChildren();
	auto AddRow = [this](const FText& Label, EFTObjectiveState State, bool bCritical)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		FLinearColor Fill = Paper, TextC = Ink;
		FText Mark = FText::GetEmpty();
		switch (State)
		{
		case EFTObjectiveState::Complete: Fill = Teal; TextC = Muted; Mark = FText::FromString(TEXT("OK")); break;
		case EFTObjectiveState::InProgress: Fill = Coral; Mark = FText::FromString(TEXT("..")); break;
		case EFTObjectiveState::FailedOptional: Fill = PaperDark; TextC = Muted; Mark = FText::FromString(TEXT("-")); break;
		case EFTObjectiveState::NotStarted: TextC = Muted; break;
		default: break;
		}
		UBorder* Check = Box(WidgetTree, Fill, 5.f, FMargin(3.f, 0.f), Ink, 2.f);
		USizeBox* CS = WidgetTree->ConstructWidget<USizeBox>();
		CS->SetWidthOverride(28.f);
		CS->SetHeightOverride(22.f);
		UTextBlock* MT = Text(WidgetTree, Mark, 10, Cream, TEXT("Black"));
		MT->SetJustification(ETextJustify::Center);
		Check->SetContent(MT);
		Check->SetHorizontalAlignment(HAlign_Center);
		Check->SetVerticalAlignment(VAlign_Center);
		CS->AddChild(Check);
		AddH(Row, CS, FMargin(0.f, 0.f, 8.f, 0.f));
		UTextBlock* L = Text(WidgetTree, bCritical ? Label : FText::Format(LOCTEXT("Opt", "{0}"), Label), bCritical ? 16 : 14, TextC, bCritical ? TEXT("Bold") : TEXT("Italic"));
		L->SetAutoWrapText(false);
		L->SetWrapTextAt(310.f);
		if (State == EFTObjectiveState::Complete)
		{
			L->SetStrikeBrush(Round(FLinearColor(0.1f, 0.12f, 0.25f, 0.8f), 1.f));
		}
		AddH(Row, L, FMargin(0.f), true);
		AddV(ObjectiveList, Row, FMargin(0.f, 3.f));
	};

	StatePill->SetBrush(Round(StateColor(GS->SceneState), 10.f, Ink, 2.f));
	StateText->SetText(StateLabel(GS->SceneState));
	switch (GS->ShootPhase)
	{
	case EFTShootPhase::Title:
	case EFTShootPhase::Lobby:
		FilmLine->SetText(LOCTEXT("NoFilmYet", "NO SCRIPT GREENLIT"));
		SceneTitle->SetText(LOCTEXT("NightShift", "Night Shift"));
		StateText->SetText(LOCTEXT("Lobby2", "LOBBY"));
		AddRow(LOCTEXT("L1", "Walk to the Director's Office"), EFTObjectiveState::Available, true);
		AddRow(LOCTEXT("L2", "Hold E on the big script book"), EFTObjectiveState::Available, true);
		AddRow(LOCTEXT("L3", "Greenlight a script"), EFTObjectiveState::NotStarted, true);
		break;
	case EFTShootPhase::Shooting:
		if (Film && Scene)
		{
			FilmLine->SetText(FText::Format(LOCTEXT("FilmFmt", "{0}  -  TAKE {1}"), Film->Title, FText::AsNumber(GS->TakeNumber)));
			SceneTitle->SetText(Scene->Title);
			for (int32 i = 0; i < Scene->Objectives.Num() && i < GS->Objectives.Num(); ++i)
			{
				AddRow(Scene->Objectives[i].Text, GS->Objectives[i].State, Scene->Objectives[i].bCritical);
			}
		}
		break;
	case EFTShootPhase::Finale:
		FilmLine->SetText(Film ? Film->Title : FText::GetEmpty());
		SceneTitle->SetText(LOCTEXT("Finale", "Finale: The Premiere"));
		StateText->SetText(LOCTEXT("FinaleState", "FINALE"));
		AddRow(LOCTEXT("F1", "Projection room unlocked"), GS->bProjectionUnlocked ? EFTObjectiveState::Complete : EFTObjectiveState::Available, true);
		AddRow(FText::Format(LOCTEXT("F2", "Load the reels ({0}/{1})"), FText::AsNumber(GS->ReelsLoaded), FText::AsNumber(GS->GetCompletedTakeCount())),
			GS->ReelsLoaded >= GS->GetCompletedTakeCount() ? EFTObjectiveState::Complete : EFTObjectiveState::Available, true);
		AddRow(LOCTEXT("F3", "Restore projector power"), GS->bProjectorPower ? EFTObjectiveState::Complete : EFTObjectiveState::Available, true);
		AddRow(LOCTEXT("F4", "Hold E on the projector: start the premiere"), EFTObjectiveState::Available, true);
		break;
	case EFTShootPhase::Premiere:
		SceneTitle->SetText(LOCTEXT("PremiereNow", "Premiere!"));
		StateText->SetText(LOCTEXT("PremiereState", "NOW SHOWING"));
		break;
	case EFTShootPhase::Results:
		SceneTitle->SetText(LOCTEXT("Wrap", "That's a wrap!"));
		StateText->SetText(LOCTEXT("ResultsState", "RESULTS"));
		break;
	case EFTShootPhase::Failed:
		SceneTitle->SetText(LOCTEXT("Blackout", "Blackout!"));
		StateText->SetText(LOCTEXT("FailedState", "SHOOT FAILED"));
		break;
	}
	CallSheet->SetVisibility(GS->ShootPhase == EFTShootPhase::Title ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UFTHUDWidget::UpdateClock()
{
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS)
	{
		return;
	}
	ClockText->SetText(GS->GetStudioClockText());
	const float Remaining = GS->GetDawnRemaining();
	ClockSub->SetText(FText::Format(LOCTEXT("ClockSub", "PREMIERE AT 06:00  -  {0}:{1} LEFT"),
		FText::AsNumber(FMath::FloorToInt(Remaining / 60.f)), FText::FromString(FString::Printf(TEXT("%02d"), FMath::FloorToInt(FMath::Fmod(Remaining, 60.f))))));
	DawnBar->SetPercent(1.f - Remaining / FMath::Max(GS->DawnDuration, 1.f));
	const float Cond = GS->StudioCondition / 100.f;
	ConditionBar->SetPercent(Cond);
	ConditionBar->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(Red, Teal, Cond));
	ConditionText->SetText(FText::Format(LOCTEXT("StudioFmt", "STUDIO CONDITION  {0}%"), FText::AsNumber(FMath::RoundToInt(GS->StudioCondition))));
	ScoreText->SetText(FText::Format(LOCTEXT("ScoreFmt", "SCORE  {0}"), FText::AsNumber(GS->TeamScore)));

	const bool bDanger = GS->IsShootActive() && (!GS->bStagePower || GS->FloodStage != EFTFloodStage::Dry);
	const float A = bDanger ? 0.25f + 0.2f * FMath::Sin(Time * 5.f) : 0.f;
	for (UImage* E : DangerEdges)
	{
		E->SetColorAndOpacity(FLinearColor(Red.R, Red.G, Red.B, A));
	}
}

void UFTHUDWidget::UpdateCrew()
{
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!GS)
	{
		return;
	}
	CrewList->ClearChildren();
	for (APlayerState* P : GS->PlayerArray)
	{
		const AFTPlayerState* PS = Cast<AFTPlayerState>(P);
		if (!PS)
		{
			continue;
		}
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		AddH(Row, Swatch(WidgetTree, AFTPlayerState::CrewColor(PS->CrewIndex), FVector2D(14.f, 14.f), 7.f), FMargin(0.f, 0.f, 8.f, 0.f));
		AddH(Row, Text(WidgetTree, FText::FromString(PS->GetCrewName()), 15, White, TEXT("Bold"), true), FMargin(0.f, 0.f, 8.f, 0.f));
		FString Tags;
		if (PS->bOnCamera) Tags += TEXT("CAM  ");
		if (PS->bCarrying) Tags += TEXT("CARRY  ");
		Tags += CostumeName(PS->Costume);
		if (!Tags.IsEmpty())
		{
			UBorder* Pill = Box(WidgetTree, Amber, 8.f, FMargin(6.f, 1.f), Ink, 2.f);
			Pill->SetContent(Text(WidgetTree, FText::FromString(Tags.TrimEnd()), 11, Ink, TEXT("Black")));
			AddH(Row, Pill);
		}
		AddV(CrewList, Row, FMargin(0.f, 2.f), 2);
	}
}

void UFTHUDWidget::UpdatePrompt()
{
	APlayerController* PC = GetOwningPlayer();
	AFTCharacter* C = PC ? Cast<AFTCharacter>(PC->GetPawn()) : nullptr;
	bool bAvailable = false;
	FText Reason;
	const FText Prompt = C ? C->GetPromptText(bAvailable, Reason) : FText::GetEmpty();
	const bool bOperating = C && C->IsOperatingCamera();
	Crosshair->SetVisibility(C && !bOperating ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	// keep the viewfinder clean: the lens layer has its own status line and key hints
	const AFTPlayerController* FTPC = Cast<AFTPlayerController>(PC);
	const bool bModalUI = FTPC && FTPC->IsUIBlockingGameplay();
	HelpCard->SetVisibility(bHelpVisible && !bOperating && !bModalUI ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	CalloutBanner->SetVisibility(!GS || bOperating || GS->Callout.IsEmpty() || GS->ShootPhase == EFTShootPhase::Title ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	if (!Prompt.IsEmpty() && !bOperating)
	{
		PromptBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		PromptText->SetText(Prompt);
		PromptText->SetColorAndOpacity(FSlateColor(bAvailable ? Ink : Muted));
		PromptReason->SetText(Reason);
		PromptReason->SetVisibility(!bAvailable && !Reason.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		const UFTInteractableComponent* F = C->GetFocusedInteractable();
		const bool bHold = F && F->Type == EFTInteractType::Hold;
		HoldBar->GetParent()->SetVisibility(bHold ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		HoldBar->SetPercent(C->GetHoldProgress());
		PromptKey->SetText(bHold ? LOCTEXT("HoldE", "HOLD E") : FText::FromString(TEXT("E")));
	}
	else
	{
		PromptBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (C && C->HeldProp && !bOperating)
	{
		const bool bThrow = C->HeldProp->bThrowable;
		CarryHint->SetText(FText::Format(bThrow ? LOCTEXT("CarryThrow", "Carrying {0}   Q drop   LMB throw") : LOCTEXT("Carry", "Carrying {0}   Q drop"), C->HeldProp->PropName));
		CarryHint->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		CarryHint->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHUDWidget::UpdateCamera(float Dt)
{
	APlayerController* PC = GetOwningPlayer();
	AFTCharacter* C = PC ? Cast<AFTCharacter>(PC->GetPawn()) : nullptr;
	AFTFilmCamera* Cam = C ? C->GetOperatedCamera() : nullptr;
	const AFTGameState* GS = GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
	if (!Cam || !GS)
	{
		CameraLayer->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	CameraLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	const bool bRec = Cam->IsRecording();
	RecDot->SetColorAndOpacity(bRec && FMath::Fmod(Time, 1.f) < 0.6f ? Red : FLinearColor(0.4f, 0.4f, 0.4f, 0.8f));
	RecText->SetText(bRec ? LOCTEXT("RecOn", "REC") : LOCTEXT("Stby", "STBY"));
	const float T = bRec ? GS->RecordingTime : 0.f;
	const int32 Frames = FMath::FloorToInt(FMath::Fmod(T, 1.f) * 24.f);
	Timecode->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d:%02d"), FMath::FloorToInt(T / 60.f), FMath::FloorToInt(FMath::Fmod(T, 60.f)), Frames)));

	FText Status;
	switch (GS->SceneState)
	{
	case EFTSceneState::Preparation: Status = LOCTEXT("CamPrep", "SET NOT READY - check the call sheet"); break;
	case EFTSceneState::ReadyToRoll: Status = LOCTEXT("CamReady", "READY - press LMB to roll"); break;
	case EFTSceneState::Recording: Status = GS->bKeyActionDone ? LOCTEXT("CamHold", "HOLD THE SHOT!") : LOCTEXT("CamAction", "ROLLING - waiting for the action"); break;
	default: Status = FText::GetEmpty(); break;
	}
	CameraStatus->SetText(Status);

	const FFTFrameReport& F = GS->LiveFrame;
	FrameBar->SetPercent(bRec ? F.Quality : 0.f);
	FrameBar->SetFillColorAndOpacity(F.bCriticalValid ? Teal : Coral);
	FrameText->SetText(bRec ? FText::Format(LOCTEXT("FrameFmt", "FRAME {0}%"), FText::AsNumber(FMath::RoundToInt(F.Quality * 100.f))) : LOCTEXT("FramePre", "FRAME --"));

	int32 Used = 0;
	auto SetLine = [this, &Used](const FText& InText, const FLinearColor& Color)
	{
		if (SubjectLines.IsValidIndex(Used))
		{
			UTextBlock* L = SubjectLines[Used];
			L->SetText(InText);
			L->SetColorAndOpacity(FSlateColor(Color));
			L->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		++Used;
	};
	if (const FFTSceneDefinition* Scene = GS->GetCurrentScene())
	{
		for (int32 i = 0; i < Scene->RequiredSubjects.Num(); ++i)
		{
			const float S = bRec && F.SubjectScores.IsValidIndex(i) ? F.SubjectScores[i] : -1.f;
			const FString Name = Scene->RequiredSubjects[i].ToString().RightChop(8);
			const FString Line = S < 0.f ? FString::Printf(TEXT("[ ] %s"), *Name) : FString::Printf(TEXT("%s %s  %d%%"), S >= 0.3f ? TEXT("[x]") : TEXT("[ ]"), *Name, FMath::RoundToInt(S * 100.f));
			SetLine(FText::FromString(Line), S >= 0.3f ? Cyan : White);
		}
		if (bRec)
		{
			SetLine(FText::Format(LOCTEXT("HoldSec", "HOLD {0} / {1} s"), FText::AsNumber(FMath::RoundToFloat(GS->CaptureProgress * Scene->CaptureDuration * 10.f) / 10.f), FText::AsNumber(Scene->CaptureDuration)), Yellow);
		}
	}
	for (int32 i = Used; i < SubjectLines.Num(); ++i)
	{
		SubjectLines[i]->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHUDWidget::UpdateTransient(float Dt)
{
	// toasts
	for (int32 i = Toasts.Num() - 1; i >= 0; --i)
	{
		FToast& T = Toasts[i];
		T.Age += Dt;
		UWidget* W = T.Widget.Get();
		if (!W || T.Age > 3.6f)
		{
			if (W)
			{
				W->RemoveFromParent();
			}
			Toasts.RemoveAt(i);
			continue;
		}
		W->SetRenderOpacity(FMath::Clamp((3.6f - T.Age) / 0.5f, 0.f, 1.f));
	}

	// announcements
	const float Hold = AnnounceQueue.Num() > 1 ? 2.2f : 3.2f;
	if (AnnounceAge > Hold && AnnounceQueue.Num() > 0)
	{
		const FAnnounce A = AnnounceQueue[0];
		AnnounceQueue.RemoveAt(0);
		AnnounceText->SetText(A.Text);
		FLinearColor Fill = Ink, TextC = Cream;
		switch (A.Style)
		{
		case EFTAnnounceStyle::Success: Fill = Teal; break;
		case EFTAnnounceStyle::Danger: Fill = Red; break;
		case EFTAnnounceStyle::Hint: Fill = Amber; TextC = Ink; break;
		case EFTAnnounceStyle::Info: Fill = FTColors::Hex(0x2E3766); break;
		default: break;
		}
		AnnounceBox->SetBrush(Round(Fill, 10.f, Cream, 3.f));
		AnnounceText->SetColorAndOpacity(FSlateColor(TextC));
		AnnounceText->SetFont(Font(A.Style == EFTAnnounceStyle::Info ? 24 : 34, TEXT("Black")));
		AnnounceAge = 0.f;
	}
	AnnounceAge += Dt;
	if (AnnounceAge < Hold + 0.4f)
	{
		AnnounceRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		const float In = Bounce(AnnounceAge / 0.35f);
		const float Out = FMath::Clamp((Hold + 0.4f - AnnounceAge) / 0.4f, 0.f, 1.f);
		AnnounceRoot->SetRenderScale(FVector2D(FMath::Lerp(0.6f, 1.f, In)));
		AnnounceRoot->SetRenderOpacity(FMath::Min(Out, FMath::Clamp(AnnounceAge / 0.12f, 0.f, 1.f)));
	}
	else
	{
		AnnounceRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	// take stamp
	StampAge += Dt;
	if (StampAge < 6.5f)
	{
		const float In = Bounce(StampAge / 0.3f);
		StampRoot->SetRenderScale(FVector2D(FMath::Lerp(1.8f, 1.f, In)));
		StampRoot->SetRenderOpacity(FMath::Clamp((6.5f - StampAge) / 0.6f, 0.f, 1.f));
	}
	else if (StampRoot->IsVisible())
	{
		StampRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	// controls card auto-hides after the first half minute
	HelpAge += Dt;
	if (bHelpAutoHide && HelpAge > 30.f && bHelpVisible)
	{
		bHelpVisible = false;
		HelpCard->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFTHUDWidget::UpdatePings()
{
	APlayerController* PC = GetOwningPlayer();
	for (int32 i = Pings.Num() - 1; i >= 0; --i)
	{
		Pings[i].Age += GetWorld()->GetDeltaSeconds();
		if (Pings[i].Age > 5.f)
		{
			Pings.RemoveAt(i);
		}
	}
	for (int32 i = 0; i < PingWidgets.Num(); ++i)
	{
		UWidget* W = PingWidgets[i];
		if (!Pings.IsValidIndex(i) || !PC)
		{
			W->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		FVector2D Screen;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Pings[i].Location, Screen, false))
		{
			W->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(W->Slot))
			{
				S->SetPosition(Screen);
			}
			if (UHorizontalBox* Row = Cast<UHorizontalBox>(W))
			{
				if (UImage* Dia = Cast<UImage>(Row->GetChildAt(0)))
				{
					Dia->SetColorAndOpacity(AFTPlayerState::CrewColor(Pings[i].Color));
				}
				if (UTextBlock* Label = Cast<UTextBlock>(Row->GetChildAt(1)))
				{
					Label->SetText(FText::FromString(Pings[i].Name));
				}
			}
			W->SetRenderOpacity(FMath::Clamp((5.f - Pings[i].Age) / 0.8f, 0.f, 1.f));
		}
		else
		{
			W->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#undef LOCTEXT_NAMESPACE
