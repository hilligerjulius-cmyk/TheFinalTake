#include "TheFinalTake/Core/FTInput.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/StrongObjectPtr.h"

UFTInput* UFTInput::Get()
{
	static TStrongObjectPtr<UFTInput> Instance;
	if (!Instance.IsValid())
	{
		UFTInput* In = NewObject<UFTInput>(GetTransientPackage(), TEXT("FTInput"));
		In->Build();
		Instance.Reset(In);
	}
	return Instance.Get();
}

UInputAction* UFTInput::MakeAction(const TCHAR* Name, int32 ValueType)
{
	UInputAction* A = NewObject<UInputAction>(this, Name);
	A->ValueType = static_cast<EInputActionValueType>(ValueType);
	return A;
}

void UFTInput::Build()
{
	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_FinalTake"));

	Move = MakeAction(TEXT("IA_FT_Move"), (int32)EInputActionValueType::Axis2D);
	Look = MakeAction(TEXT("IA_FT_Look"), (int32)EInputActionValueType::Axis2D);
	Jump = MakeAction(TEXT("IA_FT_Jump"), (int32)EInputActionValueType::Boolean);
	Sprint = MakeAction(TEXT("IA_FT_Sprint"), (int32)EInputActionValueType::Boolean);
	Interact = MakeAction(TEXT("IA_FT_Interact"), (int32)EInputActionValueType::Boolean);
	Drop = MakeAction(TEXT("IA_FT_Drop"), (int32)EInputActionValueType::Boolean);
	Primary = MakeAction(TEXT("IA_FT_Primary"), (int32)EInputActionValueType::Boolean);
	Zoom = MakeAction(TEXT("IA_FT_Zoom"), (int32)EInputActionValueType::Axis1D);
	Recenter = MakeAction(TEXT("IA_FT_Recenter"), (int32)EInputActionValueType::Boolean);
	CostumeAction = MakeAction(TEXT("IA_FT_CostumeAction"), (int32)EInputActionValueType::Boolean);
	Emote1 = MakeAction(TEXT("IA_FT_Emote1"), (int32)EInputActionValueType::Boolean);
	Emote2 = MakeAction(TEXT("IA_FT_Emote2"), (int32)EInputActionValueType::Boolean);
	Emote3 = MakeAction(TEXT("IA_FT_Emote3"), (int32)EInputActionValueType::Boolean);
	Emote4 = MakeAction(TEXT("IA_FT_Emote4"), (int32)EInputActionValueType::Boolean);
	Ping = MakeAction(TEXT("IA_FT_Ping"), (int32)EInputActionValueType::Boolean);
	Help = MakeAction(TEXT("IA_FT_Help"), (int32)EInputActionValueType::Boolean);
	Pause = MakeAction(TEXT("IA_FT_Pause"), (int32)EInputActionValueType::Boolean);

	// WASD -> 2D axis (X = right, Y = forward)
	{
		FEnhancedActionKeyMapping& W = Context->MapKey(Move, EKeys::W);
		W.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		FEnhancedActionKeyMapping& S = Context->MapKey(Move, EKeys::S);
		S.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
		S.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		Context->MapKey(Move, EKeys::D);
		FEnhancedActionKeyMapping& A = Context->MapKey(Move, EKeys::A);
		A.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		Context->MapKey(Move, EKeys::Gamepad_Left2D);
	}
	{
		FEnhancedActionKeyMapping& M = Context->MapKey(Look, EKeys::Mouse2D);
		UInputModifierNegate* NegY = NewObject<UInputModifierNegate>(Context);
		NegY->bX = false;
		NegY->bY = true;
		NegY->bZ = false;
		M.Modifiers.Add(NegY);
		FEnhancedActionKeyMapping& G = Context->MapKey(Look, EKeys::Gamepad_Right2D);
		UInputModifierNegate* NegGY = NewObject<UInputModifierNegate>(Context);
		NegGY->bX = false;
		NegGY->bY = true;
		NegGY->bZ = false;
		G.Modifiers.Add(NegGY);
		UInputModifierScalar* Scale = NewObject<UInputModifierScalar>(Context);
		Scale->Scalar = FVector(4.f, 4.f, 1.f);
		G.Modifiers.Add(Scale);
	}
	Context->MapKey(Jump, EKeys::SpaceBar);
	Context->MapKey(Jump, EKeys::Gamepad_FaceButton_Bottom);
	Context->MapKey(Sprint, EKeys::LeftShift);
	Context->MapKey(Sprint, EKeys::Gamepad_LeftThumbstick);
	Context->MapKey(Interact, EKeys::E);
	Context->MapKey(Interact, EKeys::Gamepad_FaceButton_Left);
	Context->MapKey(Drop, EKeys::Q);
	Context->MapKey(Drop, EKeys::Gamepad_FaceButton_Right);
	Context->MapKey(Primary, EKeys::LeftMouseButton);
	Context->MapKey(Primary, EKeys::Gamepad_RightTrigger);
	Context->MapKey(Zoom, EKeys::MouseWheelAxis);
	Context->MapKey(Recenter, EKeys::R);
	Context->MapKey(CostumeAction, EKeys::F);
	Context->MapKey(CostumeAction, EKeys::Gamepad_FaceButton_Top);
	Context->MapKey(Emote1, EKeys::One);
	Context->MapKey(Emote2, EKeys::Two);
	Context->MapKey(Emote3, EKeys::Three);
	Context->MapKey(Emote4, EKeys::Four);
	Context->MapKey(Ping, EKeys::MiddleMouseButton);
	Context->MapKey(Ping, EKeys::G);
	Context->MapKey(Help, EKeys::H);
	Context->MapKey(Help, EKeys::F1);
	Context->MapKey(Pause, EKeys::Escape);
	Context->MapKey(Pause, EKeys::P);
	Context->MapKey(Pause, EKeys::Gamepad_Special_Right);
}
