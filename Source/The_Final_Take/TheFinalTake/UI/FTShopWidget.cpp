#include "TheFinalTake/UI/FTShopWidget.h"

#include "TheFinalTake/UI/FTUIStyle.h"
#include "TheFinalTake/Career/FTEconomy.h"
#include "TheFinalTake/Career/FTShopItems.h"
#include "TheFinalTake/Core/FTAudio.h"
#include "TheFinalTake/Data/FTFilmDefinition.h"
#include "TheFinalTake/Game/FTGameState.h"
#include "TheFinalTake/Game/FTPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FinalTakeShopUI"

using namespace FTUI;

namespace
{
	const EFTShopCategory TabCategories[UFTShopWidget::NumTabs] = { EFTShopCategory::Costume, EFTShopCategory::Prop, EFTShopCategory::Effect, EFTShopCategory::SharkUpgrade, EFTShopCategory::SetPiece };

	FText TabName(int32 Tab)
	{
		switch (Tab)
		{
		case 0: return LOCTEXT("TabCostume", "COSTUMES");
		case 1: return LOCTEXT("TabProp", "PROPS");
		case 2: return LOCTEXT("TabFx", "EFFECTS");
		case 3: return LOCTEXT("TabShark", "SHARK KITS");
		default: return LOCTEXT("TabSet", "SET PIECES");
		}
	}

	UWidget* Sized(UWidgetTree* T, UWidget* W, float Width, float Height = 0.f)
	{
		USizeBox* S = T->ConstructWidget<USizeBox>();
		if (Width > 0.f) S->SetWidthOverride(Width);
		if (Height > 0.f) S->SetHeightOverride(Height);
		S->AddChild(W);
		return S;
	}

	FButtonStyle RowStyle(const FLinearColor& Fill)
	{
		FButtonStyle Style;
		Style.SetNormal(Round(Fill, 10.f, Ink, 2.f));
		Style.SetHovered(Round(Cream, 10.f, Ink, 3.f));
		Style.SetPressed(Round(Amber, 10.f, Ink, 3.f));
		Style.SetDisabled(Round(Fill * 0.8f, 10.f, Ink, 2.f));
		Style.SetNormalPadding(FMargin(10.f, 6.f));
		Style.SetPressedPadding(FMargin(10.f, 6.f));
		return Style;
	}
}

AFTPlayerController* UFTShopWidget::PC() const
{
	return Cast<AFTPlayerController>(GetOwningPlayer());
}

const AFTGameState* UFTShopWidget::GS() const
{
	return GetWorld() ? GetWorld()->GetGameState<AFTGameState>() : nullptr;
}

void UFTShopWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UWidgetTree* T = WidgetTree;
	UCanvasPanel* Root = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	T->RootWidget = Root;
	{
		UImage* Dim = Swatch(T, FLinearColor(0.01f, 0.01f, 0.03f, 0.62f), FVector2D(10.f));
		UCanvasPanelSlot* S = Root->AddChildToCanvas(Dim);
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	UBorder* Frame = Box(T, Coral, 24.f, FMargin(18.f, 16.f), Ink, 4.f);
	UVerticalBox* Page = T->ConstructWidget<UVerticalBox>();
	Frame->SetContent(Page);

	// ------------------------------------------------ header: sign, team balance, close
	UHorizontalBox* Header = T->ConstructWidget<UHorizontalBox>();
	UVerticalBox* TitleCol = T->ConstructWidget<UVerticalBox>();
	AddV(TitleCol, Text(T, LOCTEXT("ShopTitle", "STUDIO SUPPLY CO."), 34, Cream, TEXT("Black"), true), FMargin(0.f));
	AddV(TitleCol, Text(T, LOCTEXT("ShopSub", "Everything a no-budget blockbuster needs. Paid from the team account."), 15, Ink, TEXT("Italic")), FMargin(0.f));
	AddH(Header, TitleCol, FMargin(4.f, 0.f, 12.f, 0.f), true, 1);
	UBorder* Balance = Box(T, TealDark, 12.f, FMargin(16.f, 6.f), Ink, 3.f);
	UVerticalBox* BalCol = T->ConstructWidget<UVerticalBox>();
	AddV(BalCol, Text(T, LOCTEXT("TeamBalance", "TEAM BALANCE"), 12, Cream, TEXT("Black")), FMargin(0.f), 2);
	BalanceText = Text(T, FText::GetEmpty(), 26, Yellow, TEXT("Black"));
	AddV(BalCol, BalanceText, FMargin(0.f), 2);
	Balance->SetContent(BalCol);
	AddH(Header, Balance, FMargin(0.f, 0.f, 12.f, 0.f));
	UButton* Close = Button(T, LOCTEXT("CloseShop", "CLOSE (ESC)"), PaperDark, 16);
	Close->OnClicked.AddDynamic(this, &UFTShopWidget::HandleClose);
	AddH(Header, Close);
	AddV(Page, Header, FMargin(0.f, 0.f, 0.f, 10.f));

	// ------------------------------------------------ category tabs
	UHorizontalBox* Tabs = T->ConstructWidget<UHorizontalBox>();
	for (int32 i = 0; i < NumTabs; ++i)
	{
		TObjectPtr<UTextBlock> Label;
		UButton* B = Button(T, TabName(i), PaperDark, 17, &Label);
		TabButtons.Add(B);
		TabLabels.Add(Label);
		AddH(Tabs, B, FMargin(0.f, 0.f, 8.f, 0.f));
	}
	TabButtons[0]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleTab0);
	TabButtons[1]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleTab1);
	TabButtons[2]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleTab2);
	TabButtons[3]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleTab3);
	TabButtons[4]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleTab4);
	AddV(Page, Tabs, FMargin(0.f, 0.f, 0.f, 10.f));

	// ------------------------------------------------ body: list | details
	UHorizontalBox* Body = T->ConstructWidget<UHorizontalBox>();
	UVerticalBox* List = T->ConstructWidget<UVerticalBox>();
	for (int32 i = 0; i < MaxRows; ++i)
	{
		UButton* B = T->ConstructWidget<UButton>();
		B->SetStyle(RowStyle(Paper));
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		UImage* Sw = Swatch(T, White, FVector2D(30.f, 30.f), 8.f);
		AddH(Row, Sw, FMargin(0.f, 0.f, 10.f, 0.f));
		UVerticalBox* Info = T->ConstructWidget<UVerticalBox>();
		UTextBlock* N = Text(T, FText::GetEmpty(), 17, Ink, TEXT("Black"));
		UTextBlock* Pr = Text(T, FText::GetEmpty(), 13, Muted, TEXT("Bold"));
		AddV(Info, N, FMargin(0.f));
		AddV(Info, Pr, FMargin(0.f));
		AddH(Row, Info, FMargin(0.f), true);
		UBorder* Chip = Box(T, Teal, 8.f, FMargin(8.f, 2.f), Ink, 2.f);
		UTextBlock* ChipText = Text(T, LOCTEXT("OwnedChip", "OWNED"), 12, Ink, TEXT("Black"));
		Chip->SetContent(ChipText);
		AddH(Row, Chip, FMargin(6.f, 0.f, 0.f, 0.f));
		B->SetContent(Row);
		AddV(List, Sized(T, B, 420.f, 58.f), FMargin(0.f, 3.f));
		RowButtons.Add(B);
		RowNames.Add(N);
		RowPrices.Add(Pr);
		RowSwatches.Add(Sw);
		RowChips.Add(Chip);
		RowChipTexts.Add(ChipText);
	}
	RowButtons[0]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem0);
	RowButtons[1]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem1);
	RowButtons[2]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem2);
	RowButtons[3]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem3);
	RowButtons[4]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem4);
	RowButtons[5]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem5);
	RowButtons[6]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem6);
	RowButtons[7]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem7);
	RowButtons[8]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem8);
	RowButtons[9]->OnClicked.AddDynamic(this, &UFTShopWidget::HandleItem9);
	UBorder* ListPage = Box(T, PaperDark, 14.f, FMargin(12.f, 10.f), Ink, 2.f);
	ListPage->SetContent(List);
	AddH(Body, Sized(T, ListPage, 456.f, 560.f), FMargin(0.f, 0.f, 10.f, 0.f), false, 0);

	UVerticalBox* Details = T->ConstructWidget<UVerticalBox>();
	// preview window: live capture from the counter's photo studio (swatch fallback without a renderer)
	UOverlay* PreviewStack = T->ConstructWidget<UOverlay>();
	PreviewFallback = Swatch(T, TealDark, FVector2D(600.f, 250.f), 10.f);
	PreviewStack->AddChildToOverlay(PreviewFallback);
	Preview = T->ConstructWidget<UImage>();
	if (UOverlaySlot* S = PreviewStack->AddChildToOverlay(Preview))
	{
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Fill);
	}
	SoldStamp = Box(T, Coral, 10.f, FMargin(22.f, 6.f), Ink, 4.f);
	SoldText = Text(T, LOCTEXT("Sold", "SOLD!"), 44, Cream, TEXT("Black"), true);
	SoldStamp->SetContent(SoldText);
	SoldStamp->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	SoldStamp->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* S = PreviewStack->AddChildToOverlay(SoldStamp))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
	}
	UBorder* PreviewFrame = Box(T, Ink, 12.f, FMargin(4.f), Ink, 3.f);
	PreviewFrame->SetContent(Sized(T, PreviewStack, 600.f, 250.f));
	AddV(Details, PreviewFrame, FMargin(0.f, 0.f, 0.f, 8.f));

	UBorder* InfoPage = Box(T, Paper, 14.f, FMargin(18.f, 12.f), Ink, 2.f);
	UVerticalBox* Info = T->ConstructWidget<UVerticalBox>();
	CategoryText = Text(T, FText::GetEmpty(), 13, TealDark, TEXT("Black"));
	AddV(Info, CategoryText, FMargin(0.f));
	NameText = Text(T, FText::GetEmpty(), 28, Ink, TEXT("Black"));
	AddV(Info, NameText, FMargin(0.f, 0.f, 0.f, 2.f));
	DescText = Text(T, FText::GetEmpty(), 15, Muted, TEXT("Italic"));
	DescText->SetAutoWrapText(false);
	DescText->SetWrapTextAt(560.f);
	AddV(Info, DescText, FMargin(0.f, 0.f, 0.f, 6.f));
	ValueText = Text(T, FText::GetEmpty(), 15, CoralDarkText(), TEXT("Black"));
	ValueText->SetAutoWrapText(false);
	ValueText->SetWrapTextAt(560.f);
	AddV(Info, ValueText, FMargin(0.f, 2.f));
	FitsText = Text(T, FText::GetEmpty(), 14, Ink, TEXT("Bold"));
	AddV(Info, FitsText, FMargin(0.f, 2.f));
	UsageText = Text(T, FText::GetEmpty(), 13, TealDark, TEXT("Bold"));
	UsageText->SetAutoWrapText(false);
	UsageText->SetWrapTextAt(560.f);
	AddV(Info, UsageText, FMargin(0.f, 2.f, 0.f, 6.f));
	UHorizontalBox* BuyRow = T->ConstructWidget<UHorizontalBox>();
	BuyButton = Button(T, LOCTEXT("Buy", "BUY"), Coral, 22, &BuyLabel);
	BuyButton->OnClicked.AddDynamic(this, &UFTShopWidget::HandleBuy);
	AddH(BuyRow, BuyButton, FMargin(0.f, 0.f, 12.f, 0.f));
	StatusText = Text(T, FText::GetEmpty(), 14, CoralDarkText(), TEXT("Bold"));
	StatusText->SetAutoWrapText(false);
	StatusText->SetWrapTextAt(300.f);
	AddH(BuyRow, StatusText, FMargin(0.f), true, 1);
	AddV(Info, BuyRow, FMargin(0.f, 4.f, 0.f, 0.f));
	InfoPage->SetContent(Info);
	AddV(Details, Sized(T, InfoPage, 616.f, 302.f));
	AddH(Body, Details, FMargin(0.f), false, 0);
	AddV(Page, Body);

	Card = Frame;
	Frame->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Place(Root, Frame, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	SelectTab(0);
}

void UFTShopWidget::SetTerminal(AFTShopTerminal* InTerminal)
{
	if (Terminal.Get() == InTerminal && bPreviewActive)
	{
		return;
	}
	Shutdown();
	Terminal = InTerminal;
	UTextureRenderTarget2D* RT = InTerminal ? InTerminal->BeginPreview() : nullptr;
	bPreviewActive = InTerminal != nullptr;
	if (RT)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(RT);
		Brush.ImageSize = FVector2D(600.f, 250.f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		// the capture is 640x400: show the middle band at the window's 2.4:1 aspect
		Brush.SetUVRegion(FBox2f(FVector2f(0.f, 0.1667f), FVector2f(1.f, 0.8333f)));
		Preview->SetBrush(Brush);
		Preview->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Preview->SetVisibility(ESlateVisibility::Collapsed);
	}
	Age = 0.f;
	StampAge = 100.f;
	SoldStamp->SetVisibility(ESlateVisibility::Collapsed);
	StatusText->SetText(FText::GetEmpty());
	SelectItem(Selected);
}

void UFTShopWidget::Shutdown()
{
	if (bPreviewActive && Terminal.IsValid())
	{
		Terminal->EndPreview();
	}
	bPreviewActive = false;
}

void UFTShopWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	StampAge += InDeltaTime;
	if (Card)
	{
		Card->SetRenderScale(FVector2D(FMath::Lerp(0.88f, 1.f, Bounce(Age / 0.35f))));
		Card->SetRenderOpacity(FMath::Clamp(Age / 0.12f, 0.f, 1.f));
	}
	if (SoldStamp->GetVisibility() != ESlateVisibility::Collapsed)
	{
		const float S = FMath::Lerp(2.2f, 1.f, Bounce(StampAge / 0.35f));
		SoldStamp->SetRenderScale(FVector2D(S));
		SoldStamp->SetRenderTransformAngle(-12.f);
		SoldStamp->SetRenderOpacity(FMath::Clamp(3.f - StampAge, 0.f, 1.f));
		if (StampAge > 3.f)
		{
			SoldStamp->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UFTShopWidget::HandleTab0() { FTAudio::Play2D(this, EFTSound::UIClick, 0.8f); SelectTab(0); }
void UFTShopWidget::HandleTab1() { FTAudio::Play2D(this, EFTSound::UIClick, 0.8f); SelectTab(1); }
void UFTShopWidget::HandleTab2() { FTAudio::Play2D(this, EFTSound::UIClick, 0.8f); SelectTab(2); }
void UFTShopWidget::HandleTab3() { FTAudio::Play2D(this, EFTSound::UIClick, 0.8f); SelectTab(3); }
void UFTShopWidget::HandleTab4() { FTAudio::Play2D(this, EFTSound::UIClick, 0.8f); SelectTab(4); }
void UFTShopWidget::HandleItem0() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(0); }
void UFTShopWidget::HandleItem1() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(1); }
void UFTShopWidget::HandleItem2() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(2); }
void UFTShopWidget::HandleItem3() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(3); }
void UFTShopWidget::HandleItem4() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(4); }
void UFTShopWidget::HandleItem5() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(5); }
void UFTShopWidget::HandleItem6() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(6); }
void UFTShopWidget::HandleItem7() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(7); }
void UFTShopWidget::HandleItem8() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(8); }
void UFTShopWidget::HandleItem9() { FTAudio::Play2D(this, EFTSound::UIClick, 0.7f); SelectItem(9); }

void UFTShopWidget::SelectTab(int32 NewTab)
{
	Tab = FMath::Clamp(NewTab, 0, NumTabs - 1);
	RowIds.Reset();
	for (const FFTShopItemDef& D : UFTEconomyConfig::Get()->Items)
	{
		if (D.Category == TabCategories[Tab] && RowIds.Num() < MaxRows)
		{
			RowIds.Add(D.ItemId);
		}
	}
	for (int32 i = 0; i < TabButtons.Num(); ++i)
	{
		TabButtons[i]->SetStyle(RowStyle(i == Tab ? Yellow : PaperDark));
		TabButtons[i]->SetRenderTranslation(FVector2D(0.f, i == Tab ? -3.f : 0.f));
	}
	Selected = 0;
	RefreshRows();
	SelectItem(0);
}

void UFTShopWidget::SelectItem(int32 Index)
{
	if (!RowIds.IsValidIndex(Index))
	{
		Index = 0;
	}
	Selected = Index;
	if (Terminal.IsValid() && RowIds.IsValidIndex(Selected))
	{
		Terminal->SetPreviewItem(RowIds[Selected]);
	}
	StatusText->SetText(FText::GetEmpty());
	RefreshRows();
	RefreshDetails();
}

void UFTShopWidget::Refresh()
{
	RefreshRows();
	RefreshDetails();
}

void UFTShopWidget::RefreshRows()
{
	const AFTGameState* G = GS();
	const UFTEconomyConfig* Cfg = UFTEconomyConfig::Get();
	BalanceText->SetText(FText::FromString(FFTEconomy::MoneyString(G ? G->StudioMoney : 0)));
	for (int32 i = 0; i < MaxRows; ++i)
	{
		const FFTShopItemDef* Def = RowIds.IsValidIndex(i) ? Cfg->FindItem(RowIds[i]) : nullptr;
		UWidget* RowWidget = RowButtons[i]->GetParent();
		if (!RowWidget)
		{
			RowWidget = RowButtons[i];
		}
		RowWidget->SetVisibility(Def ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!Def)
		{
			continue;
		}
		const bool bOwned = G && G->IsOwned(Def->ItemId);
		const bool bAfford = G && G->StudioMoney >= Def->Price;
		RowButtons[i]->SetStyle(RowStyle(i == Selected ? Cream : Paper));
		RowButtons[i]->SetRenderTranslation(FVector2D(i == Selected ? 10.f : 0.f, 0.f));
		RowSwatches[i]->SetColorAndOpacity(Def->Swatch);
		RowNames[i]->SetText(Def->Name);
		RowPrices[i]->SetText(FText::Format(LOCTEXT("RowValue", "{0}  -  +{1} production"), FText::FromString(FFTEconomy::MoneyString(Def->Price)), FText::AsNumber(Def->StyleValue)));
		RowChips[i]->SetBrush(Round(bOwned ? Teal : (bAfford ? Yellow : PaperDark), 8.f, Ink, 2.f));
		RowChipTexts[i]->SetText(bOwned ? LOCTEXT("OwnedChip2", "OWNED") : (bAfford ? LOCTEXT("BuyChip", "BUY") : LOCTEXT("SaveChip", "SAVE UP")));
	}
}

void UFTShopWidget::RefreshDetails()
{
	const AFTGameState* G = GS();
	const FFTShopItemDef* Def = RowIds.IsValidIndex(Selected) ? UFTEconomyConfig::Get()->FindItem(RowIds[Selected]) : nullptr;
	if (!Def)
	{
		NameText->SetText(LOCTEXT("SoldOut", "Sold out"));
		BuyButton->SetIsEnabled(false);
		return;
	}
	PreviewFallback->SetColorAndOpacity(Def->Swatch * 0.6f);
	CategoryText->SetText(FText::FromString(FFTEconomy::CategoryName(Def->Category).ToString().ToUpper()));
	NameText->SetText(Def->Name);
	DescText->SetText(Def->Description);
	ValueText->SetText(FText::Format(LOCTEXT("ValueFmt", "+{0} PRODUCTION VALUE  =  +{0}% audience for every release it is seen in"), FText::AsNumber(Def->StyleValue)));
	const UFTFilmDefinition* Film = Def->SuggestedFilm.IsNone() ? nullptr : UFTFilmDefinition::Find(Def->SuggestedFilm);
	FitsText->SetText(Film ? FText::Format(LOCTEXT("Fits", "Looks best in: {0}"), Film->Title) : LOCTEXT("FitsAny", "Looks good in any film"));
	UsageText->SetText(FTShop::UsageHint(*Def));

	const bool bOwned = G && G->IsOwned(Def->ItemId);
	const int32 Money = G ? G->StudioMoney : 0;
	const AFTPlayerController* P = PC();
	const bool bPending = P && P->PendingPurchase != 0;
	if (bOwned)
	{
		BuyLabel->SetText(LOCTEXT("OwnedBtn", "OWNED"));
		BuyButton->SetIsEnabled(false);
	}
	else if (bPending)
	{
		BuyLabel->SetText(LOCTEXT("Buying", "BUYING..."));
		BuyButton->SetIsEnabled(false);
	}
	else if (Money < Def->Price)
	{
		BuyLabel->SetText(FText::Format(LOCTEXT("NeedMore", "NEED {0} MORE"), FText::FromString(FFTEconomy::MoneyString(Def->Price - Money))));
		BuyButton->SetIsEnabled(false);
	}
	else
	{
		BuyLabel->SetText(FText::Format(LOCTEXT("BuyFor", "BUY FOR {0}"), FText::FromString(FFTEconomy::MoneyString(Def->Price))));
		BuyButton->SetIsEnabled(true);
	}
}

void UFTShopWidget::HandleBuy()
{
	AFTPlayerController* P = PC();
	if (!P || !RowIds.IsValidIndex(Selected))
	{
		return;
	}
	if (P->RequestPurchase(RowIds[Selected]) != 0)
	{
		FTAudio::Play2D(this, EFTSound::UIClick, 1.f, 0.9f);
		StatusText->SetText(LOCTEXT("Asking", "Asking the studio accountant..."));
	}
	RefreshDetails();
}

void UFTShopWidget::OnPurchaseAnswer(bool bOk)
{
	const AFTPlayerController* P = PC();
	StatusText->SetText(P ? P->LastPurchaseMessage : FText::GetEmpty());
	StatusText->SetColorAndOpacity(FSlateColor(bOk ? TealDark : CoralDarkText()));
	if (bOk)
	{
		StampAge = 0.f;
		SoldStamp->SetVisibility(ESlateVisibility::HitTestInvisible);
		FTAudio::Play2D(this, EFTSound::Stamp, 1.f, 1.1f);
	}
	Refresh();
}

void UFTShopWidget::HandleClose()
{
	FTAudio::Play2D(this, EFTSound::UIClick, 0.8f);
	if (AFTPlayerController* P = PC())
	{
		P->LocalCloseShop();
	}
}

#undef LOCTEXT_NAMESPACE
