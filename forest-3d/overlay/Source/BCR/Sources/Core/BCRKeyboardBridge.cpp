#include "BCR/Headers/Core/BCRKeyboardBridge.h"
#include "BCR/Headers/Core/BCRGameViewportClient.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

void UBCRKeyboardBridge::SendKeyboardButton(int32 PlayerIndex, const FString& ControllerKey, bool bPressed)
{
    if (auto* Viewport = GetTypedOuter<UBCRGameViewportClient>())
        Viewport->SendKey(PlayerIndex, FKey(FName(*ControllerKey)), bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f);
}

void UBCRKeyboardBridge::SendKeyboardAxes(int32 PlayerIndex, float X, float Y)
{
    if (auto* Viewport = GetTypedOuter<UBCRGameViewportClient>())
    {
        Viewport->SendKey(PlayerIndex, EKeys::Gamepad_LeftX, IE_Axis, X);
        Viewport->SendKey(PlayerIndex, EKeys::Gamepad_LeftY, IE_Axis, Y);
    }
}

TArray<UUserWidget*> UBCRKeyboardBridge::FindViewportQuestWidgets() const
{
    TArray<UUserWidget*> Widgets;
    const auto* Viewport = GetTypedOuter<UBCRGameViewportClient>();
    UWorld* World = Viewport ? Viewport->GetWorld() : nullptr;
    if (!World) return Widgets;
    for (TObjectIterator<UUserWidget> It; It; ++It)
        if (It->GetWorld() == World && It->IsInViewport() && It->GetClass()->GetName() == TEXT("WBP_Quest_C"))
            Widgets.Add(*It);
    return Widgets;
}

void UBCRKeyboardBridge::ApplyViewportQuestVisibility(UUserWidget* Widget, ESlateVisibility Visibility)
{
    if (IsValid(Widget)) Widget->SetVisibility(Visibility);
}

bool UBCRKeyboardBridge::ApplyViewportPause(bool bPaused)
{
    const auto* Viewport = GetTypedOuter<UBCRGameViewportClient>();
    return Viewport && Viewport->GetWorld() && UGameplayStatics::SetGamePaused(Viewport->GetWorld(), bPaused);
}

bool UBCRKeyboardBridge::SetViewportHelpVisible(bool bVisible)
{
    auto* Viewport = GetTypedOuter<UBCRGameViewportClient>();
    return Viewport && Viewport->SetKeyboardHelpVisible(bVisible);
}
