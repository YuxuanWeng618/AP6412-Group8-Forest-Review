#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Components/SlateWrapperTypes.h"
#include "BCRKeyboardBridge.generated.h"

class UUserWidget;

/** Reflected boundary between the viewport and its per-world C# input policy. */
UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRKeyboardBridge : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    bool RouteViewportKey(const FString& PhysicalKey, bool bPressed, bool bReleased, bool bInputIgnored,
        bool bPaused, bool bHelpOpen, bool bP1HasPawn, bool bP2HasPawn);
    UFUNCTION(BlueprintImplementableEvent)
    void RequestKeyboardHelpToggle(bool bPaused, bool bHelpOpen);
    UFUNCTION(BlueprintImplementableEvent)
    void OnViewportFocusLost(bool bInputIgnored, bool bPaused, bool bP1HasPawn);
    UFUNCTION(BlueprintImplementableEvent)
    void TickViewportInput(bool bInputIgnored, bool bPaused, bool bHelpOpen);

    UFUNCTION(BlueprintImplementableEvent)
    FString GetActionLabel(int32 PlayerIndex, const FString& ControllerKey) const;

    UFUNCTION(BlueprintImplementableEvent)
    FString GetHorizontalLabel(int32 PlayerIndex) const;

    UFUNCTION(BlueprintImplementableEvent)
    TArray<FString> OverviewKeyboardPrompts(bool bPaused) const;
    UFUNCTION(BlueprintImplementableEvent)
    FString QteKeyboardPrompt(int32 PlayerIndex, const FString& Label, bool bWaiting) const;
    UFUNCTION(BlueprintImplementableEvent)
    double PositionKeyboardStatus(double Preferred, double Height, double OverviewTop, double Scale) const;
    UFUNCTION(BlueprintImplementableEvent)
    FString WorldKeyboardPrompt(const FString& Asset, bool bIsRope, int32 RopePlayer, bool bIsMachine,
        int32 FirstPlayer, int32 SecondPlayer, double FirstDistance, double SecondDistance) const;
    UFUNCTION(BlueprintImplementableEvent)
    FVector2D PlaceKeyboardPrompt(FVector Screen, double Width, double Height, double Scale, const TArray<FVector2D>& Placed) const;

    UFUNCTION(BlueprintCallable)
    void SendKeyboardButton(int32 PlayerIndex, const FString& ControllerKey, bool bPressed);

    UFUNCTION(BlueprintCallable)
    void SendKeyboardAxes(int32 PlayerIndex, float X, float Y);

    UFUNCTION(BlueprintCallable)
    TArray<UUserWidget*> FindViewportQuestWidgets() const;
    UFUNCTION(BlueprintCallable)
    void ApplyViewportQuestVisibility(UUserWidget* Widget, ESlateVisibility Visibility);
    UFUNCTION(BlueprintCallable)
    bool ApplyViewportPause(bool bPaused);
    UFUNCTION(BlueprintCallable)
    bool SetViewportHelpVisible(bool bVisible);
};
