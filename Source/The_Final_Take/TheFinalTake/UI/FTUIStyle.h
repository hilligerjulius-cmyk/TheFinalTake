#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Layout/Margin.h"
#include "Widgets/Layout/Anchors.h"

class UWidgetTree;
class UWidget;
class UTextBlock;
class UBorder;
class UButton;
class UImage;
class UCanvasPanel;
class UCanvasPanelSlot;
class UVerticalBox;
class UHorizontalBox;
class UProgressBar;

/** Tactile "call sheet + clapperboard" UI kit shared by every widget. */
namespace FTUI
{
	extern const FLinearColor Paper, PaperDark, Ink, Teal, TealDark, Coral, Amber, Yellow, Cream, Red, Magenta, Cyan, White, Shade, Muted;

	FSlateFontInfo Font(int32 Size, const TCHAR* Face = TEXT("Bold"));
	FSlateBrush Round(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f);

	UTextBlock* Text(UWidgetTree* T, const FText& S, int32 Size, const FLinearColor& Color, const TCHAR* Face = TEXT("Bold"), bool bShadow = false);
	UBorder* Box(UWidgetTree* T, const FLinearColor& Fill, float Radius, const FMargin& Padding, const FLinearColor& Outline = Ink, float OutlineWidth = 3.f);
	UButton* Button(UWidgetTree* T, const FText& Label, const FLinearColor& Fill, int32 FontSize, TObjectPtr<UTextBlock>* OutLabel = nullptr, const FLinearColor& TextColor = Ink);
	UImage* Swatch(UWidgetTree* T, const FLinearColor& Color, const FVector2D& Size, float Radius = 0.f);
	UProgressBar* Bar(UWidgetTree* T, const FLinearColor& Fill, const FLinearColor& Back, float Height);

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* W, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size = FVector2D::ZeroVector);
	void AddV(UVerticalBox* Box, UWidget* W, const FMargin& Pad = FMargin(0.f, 4.f), int32 HAlign = 0);
	void AddH(UHorizontalBox* Box, UWidget* W, const FMargin& Pad = FMargin(4.f, 0.f), bool bFill = false, int32 VAlign = 1);

	inline FLinearColor CoralDarkText() { return FLinearColor(0.7f, 0.12f, 0.08f, 1.f); }

	/** 0..1 ease-out-back for bouncy entrances. */
	float Bounce(float T);
}
