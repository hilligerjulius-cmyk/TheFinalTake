#include "TheFinalTake/UI/FTUIStyle.h"

#include "TheFinalTake/Core/FTVisuals.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace FTUI
{
	const FLinearColor Paper = FTColors::Hex(0xFBF1DC);
	const FLinearColor PaperDark = FTColors::Hex(0xEBD9B4);
	const FLinearColor Ink = FTColors::Hex(0x1B2140);
	const FLinearColor Teal = FTColors::Hex(0x1F9E96);
	const FLinearColor TealDark = FTColors::Hex(0x13706B);
	const FLinearColor Coral = FTColors::Hex(0xFF6F59);
	const FLinearColor Amber = FTColors::Hex(0xFFA62B);
	const FLinearColor Yellow = FTColors::Hex(0xFFC93C);
	const FLinearColor Cream = FTColors::Hex(0xF6E7C8);
	const FLinearColor Red = FTColors::Hex(0xE8322E);
	const FLinearColor Magenta = FTColors::Hex(0xE0409A);
	const FLinearColor Cyan = FTColors::Hex(0x33D6E8);
	const FLinearColor White = FLinearColor::White;
	const FLinearColor Shade = FLinearColor(0.01f, 0.012f, 0.03f, 0.72f);
	const FLinearColor Muted = FTColors::Hex(0x8A7F6A);

	FSlateFontInfo Font(int32 Size, const TCHAR* Face)
	{
		return FCoreStyle::GetDefaultFontStyle(Face, Size);
	}

	FSlateBrush Round(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		B.OutlineSettings.Color = FSlateColor(Outline);
		B.OutlineSettings.Width = OutlineWidth;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	UTextBlock* Text(UWidgetTree* T, const FText& S, int32 Size, const FLinearColor& Color, const TCHAR* Face, bool bShadow)
	{
		UTextBlock* W = T->ConstructWidget<UTextBlock>();
		W->SetText(S);
		W->SetFont(Font(Size, Face));
		W->SetColorAndOpacity(FSlateColor(Color));
		if (bShadow)
		{
			W->SetShadowOffset(FVector2D(2.f, 3.f));
			W->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.02f, 0.75f));
		}
		return W;
	}

	UBorder* Box(UWidgetTree* T, const FLinearColor& Fill, float Radius, const FMargin& Padding, const FLinearColor& Outline, float OutlineWidth)
	{
		UBorder* B = T->ConstructWidget<UBorder>();
		B->SetBrush(Round(Fill, Radius, Outline, OutlineWidth));
		B->SetPadding(Padding);
		return B;
	}

	UButton* Button(UWidgetTree* T, const FText& Label, const FLinearColor& Fill, int32 FontSize, TObjectPtr<UTextBlock>* OutLabel, const FLinearColor& TextColor)
	{
		UButton* Btn = T->ConstructWidget<UButton>();
		FButtonStyle Style;
		Style.SetNormal(Round(Fill, 14.f, Ink, 3.f));
		Style.SetHovered(Round(Fill * 1.15f + FLinearColor(0.05f, 0.05f, 0.05f, 0.f), 14.f, Ink, 4.f));
		Style.SetPressed(Round(Fill * 0.8f, 14.f, Ink, 4.f));
		Style.SetDisabled(Round(FLinearColor(0.35f, 0.34f, 0.32f, 0.9f), 14.f, Ink, 2.f));
		Style.SetNormalPadding(FMargin(22.f, 10.f));
		Style.SetPressedPadding(FMargin(22.f, 12.f, 22.f, 8.f));
		Btn->SetStyle(Style);
		UTextBlock* L = Text(T, Label, FontSize, TextColor, TEXT("Black"));
		L->SetJustification(ETextJustify::Center);
		Btn->SetContent(L);
		if (OutLabel)
		{
			*OutLabel = L;
		}
		return Btn;
	}

	UImage* Swatch(UWidgetTree* T, const FLinearColor& Color, const FVector2D& Size, float Radius)
	{
		UImage* I = T->ConstructWidget<UImage>();
		if (Radius > 0.f)
		{
			I->SetBrush(Round(FLinearColor::White, Radius));
		}
		I->SetColorAndOpacity(Color);
		I->SetDesiredSizeOverride(Size);
		return I;
	}

	UProgressBar* Bar(UWidgetTree* T, const FLinearColor& Fill, const FLinearColor& Back, float Height)
	{
		UProgressBar* P = T->ConstructWidget<UProgressBar>();
		FProgressBarStyle S;
		S.SetBackgroundImage(Round(Back, Height * 0.5f, Ink, 2.f));
		S.SetFillImage(Round(FLinearColor::White, Height * 0.5f));
		S.SetMarqueeImage(Round(FLinearColor::White, Height * 0.5f));
		P->SetWidgetStyle(S);
		P->SetFillColorAndOpacity(Fill);
		P->SetPercent(0.f);
		return P;
	}

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* W, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(Anchors);
		S->SetAlignment(Alignment);
		S->SetPosition(Position);
		if (Size.IsNearlyZero())
		{
			S->SetAutoSize(true);
		}
		else
		{
			S->SetSize(Size);
		}
		return S;
	}

	void AddV(UVerticalBox* Box, UWidget* W, const FMargin& Pad, int32 HAlign)
	{
		UVerticalBoxSlot* S = Box->AddChildToVerticalBox(W);
		S->SetPadding(Pad);
		S->SetHorizontalAlignment(HAlign == 1 ? HAlign_Center : (HAlign == 2 ? HAlign_Right : (HAlign == 3 ? HAlign_Fill : HAlign_Left)));
	}

	void AddH(UHorizontalBox* Box, UWidget* W, const FMargin& Pad, bool bFill, int32 VAlign)
	{
		UHorizontalBoxSlot* S = Box->AddChildToHorizontalBox(W);
		S->SetPadding(Pad);
		S->SetVerticalAlignment(VAlign == 0 ? VAlign_Top : (VAlign == 2 ? VAlign_Bottom : VAlign_Center));
		if (bFill)
		{
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	float Bounce(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		const float C1 = 1.70158f;
		const float C3 = C1 + 1.f;
		return 1.f + C3 * FMath::Pow(T - 1.f, 3.f) + C1 * FMath::Pow(T - 1.f, 2.f);
	}
}
