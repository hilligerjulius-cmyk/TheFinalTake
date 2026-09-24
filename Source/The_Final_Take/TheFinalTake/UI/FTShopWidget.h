#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FTShopWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UWidget;
class AFTPlayerController;
class AFTShopTerminal;
class AFTGameState;

/**
 * STUDIO SUPPLY CO. catalogue: category tabs, item list with owned/price chips, a live 3D preview
 * from the counter's photo studio, the production value an item adds and a buy button that is
 * validated on the server (one request in flight, answers come back as toasts + a SOLD stamp).
 */
UCLASS()
class THE_FINAL_TAKE_API UFTShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr int32 MaxRows = 10;
	static constexpr int32 NumTabs = 5;

	void SetTerminal(AFTShopTerminal* InTerminal);
	/** Re-reads money / ownership (called on every career change). */
	void Refresh();
	/** The server answered the last purchase request. */
	void OnPurchaseAnswer(bool bOk);
	/** Releases the counter's preview capture. */
	void Shutdown();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void HandleTab0();
	UFUNCTION() void HandleTab1();
	UFUNCTION() void HandleTab2();
	UFUNCTION() void HandleTab3();
	UFUNCTION() void HandleTab4();
	UFUNCTION() void HandleItem0();
	UFUNCTION() void HandleItem1();
	UFUNCTION() void HandleItem2();
	UFUNCTION() void HandleItem3();
	UFUNCTION() void HandleItem4();
	UFUNCTION() void HandleItem5();
	UFUNCTION() void HandleItem6();
	UFUNCTION() void HandleItem7();
	UFUNCTION() void HandleItem8();
	UFUNCTION() void HandleItem9();
	UFUNCTION() void HandleBuy();
	UFUNCTION() void HandleClose();

	void SelectTab(int32 NewTab);
	void SelectItem(int32 Index);
	void RefreshRows();
	void RefreshDetails();
	AFTPlayerController* PC() const;
	const AFTGameState* GS() const;

	UPROPERTY() TArray<TObjectPtr<UButton>> TabButtons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> TabLabels;
	UPROPERTY() TArray<TObjectPtr<UButton>> RowButtons;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> RowNames;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> RowPrices;
	UPROPERTY() TArray<TObjectPtr<UImage>> RowSwatches;
	UPROPERTY() TArray<TObjectPtr<UBorder>> RowChips;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> RowChipTexts;
	UPROPERTY() TObjectPtr<UImage> Preview;
	UPROPERTY() TObjectPtr<UImage> PreviewFallback;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UTextBlock> CategoryText;
	UPROPERTY() TObjectPtr<UTextBlock> DescText;
	UPROPERTY() TObjectPtr<UTextBlock> ValueText;
	UPROPERTY() TObjectPtr<UTextBlock> FitsText;
	UPROPERTY() TObjectPtr<UTextBlock> UsageText;
	UPROPERTY() TObjectPtr<UTextBlock> BalanceText;
	UPROPERTY() TObjectPtr<UTextBlock> StatusText;
	UPROPERTY() TObjectPtr<UTextBlock> BuyLabel;
	UPROPERTY() TObjectPtr<UButton> BuyButton;
	UPROPERTY() TObjectPtr<UBorder> SoldStamp;
	UPROPERTY() TObjectPtr<UTextBlock> SoldText;
	UPROPERTY() TObjectPtr<UWidget> Card;

	TWeakObjectPtr<AFTShopTerminal> Terminal;
	TArray<FName> RowIds;
	int32 Tab = 0;
	int32 Selected = 0;
	float Age = 0.f;
	float StampAge = 100.f;
	bool bPreviewActive = false;
};
