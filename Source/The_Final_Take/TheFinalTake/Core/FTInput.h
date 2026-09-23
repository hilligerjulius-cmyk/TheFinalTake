#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FTInput.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Enhanced Input actions + mapping context for The Final Take.
 * Built once at runtime so the control scheme lives in one readable place.
 */
UCLASS()
class THE_FINAL_TAKE_API UFTInput : public UObject
{
	GENERATED_BODY()

public:
	static UFTInput* Get();

	UPROPERTY() TObjectPtr<UInputMappingContext> Context;
	UPROPERTY() TObjectPtr<UInputAction> Move;
	UPROPERTY() TObjectPtr<UInputAction> Look;
	UPROPERTY() TObjectPtr<UInputAction> Jump;
	UPROPERTY() TObjectPtr<UInputAction> Sprint;
	UPROPERTY() TObjectPtr<UInputAction> Interact;
	UPROPERTY() TObjectPtr<UInputAction> Drop;
	UPROPERTY() TObjectPtr<UInputAction> Primary;
	UPROPERTY() TObjectPtr<UInputAction> Zoom;
	UPROPERTY() TObjectPtr<UInputAction> Recenter;
	UPROPERTY() TObjectPtr<UInputAction> CostumeAction;
	UPROPERTY() TObjectPtr<UInputAction> Emote1;
	UPROPERTY() TObjectPtr<UInputAction> Emote2;
	UPROPERTY() TObjectPtr<UInputAction> Emote3;
	UPROPERTY() TObjectPtr<UInputAction> Emote4;
	UPROPERTY() TObjectPtr<UInputAction> Ping;
	UPROPERTY() TObjectPtr<UInputAction> Help;
	UPROPERTY() TObjectPtr<UInputAction> Pause;

private:
	void Build();
	UInputAction* MakeAction(const TCHAR* Name, int32 ValueType);
};
