#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRKeyboardSmokeBridge.generated.h"

class APawn;
class APlayerController;
class UBCRGameViewportClient;
class UBCRKeyboardBridge;
class UBCRPlayerRulesBridge;
class UQTE_Subsystem;
class UUserWidget;

UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRKeyboardSmokeBridge : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent) void RunKeyboardFrame(double DeltaTime, int32 Mode);
    UFUNCTION(BlueprintCallable) APlayerController* TestController(int32 Player) const;
    UFUNCTION(BlueprintCallable) APawn* TestPawn(int32 Player) const;
    UFUNCTION(BlueprintCallable) UBCRPlayerRulesBridge* TestPlayerRules() const;
    UFUNCTION(BlueprintCallable) UBCRKeyboardBridge* TestKeyboardPolicy() const;
    UFUNCTION(BlueprintCallable) bool TestKey(const FString& Key, int32 Event);
    UFUNCTION(BlueprintCallable) bool TestDown(int32 Player, const FString& Key) const;
    UFUNCTION(BlueprintCallable) FVector2D TestStick(int32 Player) const;
    UFUNCTION(BlueprintCallable) float TestRawAxis(int32 Player, const FString& Key) const;
    UFUNCTION(BlueprintCallable) void TestIgnoreInput(bool bIgnore);
    UFUNCTION(BlueprintCallable) void TestGameInputMode(int32 Player);
    UFUNCTION(BlueprintCallable) void TestSetPaused(bool bPause);
    UFUNCTION(BlueprintCallable) bool TestPaused() const;
    UFUNCTION(BlueprintCallable) bool TestHelpOpen() const;
    UFUNCTION(BlueprintCallable) void TestToggleHelp();
    UFUNCTION(BlueprintCallable) void TestLostFocus();
    UFUNCTION(BlueprintCallable) void TestPossess(APawn* Pawn);
    UFUNCTION(BlueprintCallable) bool TestStartRound(const FString& AssetName, int32 Repeat);
    UFUNCTION(BlueprintCallable) void TestStopRound();
    UFUNCTION(BlueprintCallable) void TestJoinRound(int32 Player, int32 Slot);
    UFUNCTION(BlueprintCallable) int32 TestRoundState() const;
    UFUNCTION(BlueprintCallable) FString TestRoundLabel(int32 Player) const;
    UFUNCTION(BlueprintCallable) int32 TestVisibility(const FString& Widget) const;
    UFUNCTION(BlueprintCallable) FString TestWidgetText(const FString& Widget, const FString& Element) const;
    UFUNCTION(BlueprintCallable) TArray<FString> TestRopeKeys() const;
    UFUNCTION(BlueprintCallable) FString TestReportPath(bool bViewport) const;
    UFUNCTION(BlueprintCallable) void TestScreenshot(const FString& Name, bool bViewport);
    UFUNCTION(BlueprintCallable) void TestKeyboardLog(const FString& Message);
    UFUNCTION(BlueprintCallable) void TestKeyboardExit(int32 Code);

private:
    UBCRGameViewportClient* Client() const;
    UQTE_Subsystem* Qte() const;
    UUserWidget* FindWidget(const FString& ClassName) const;
};
