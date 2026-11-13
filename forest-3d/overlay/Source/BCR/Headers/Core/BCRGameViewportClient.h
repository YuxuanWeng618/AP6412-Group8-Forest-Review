#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "BCRGameViewportClient.generated.h"

class UNiagaraComponent;
class UUserWidget;
class UFont;
class UTexture2D;
class UBCRKeyboardBridge;
class UBCRWorkshopReplayBridge;
class UBCRQuestSmokeObserver;
class UBCRKeyboardSmokeBridge;

/** Routes two keyboard regions into the original per-player controller input paths. */
UCLASS()
class BCR_API UBCRGameViewportClient : public UGameViewportClient
{
    GENERATED_BODY()
public:
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual void Tick(float DeltaTime) override;
    virtual void LostFocus(FViewport* InViewport) override;
    virtual void PostRender(UCanvas* Canvas) override;

    FString KeyboardActionLabel(int32 PlayerIndex, FKey ControllerKey);
    FString KeyboardHorizontalLabel(int32 PlayerIndex);
    bool IsKeyboardHelpOpen() const;
    void ToggleKeyboardHelp();
    bool SetKeyboardHelpVisible(bool bVisible);
private:
    friend class UBCRKeyboardBridge;
    UBCRKeyboardBridge* GetKeyboardBridge();
    UPROPERTY(Transient) TObjectPtr<UBCRKeyboardBridge> KeyboardBridge;
    TWeakObjectPtr<UWorld> KeyboardWorld;
    bool bLoggedMissingKeyboardBridge = false;
    void SendKey(int32 PlayerIndex, FKey Key, EInputEvent Event, float Amount = 1.f);
    void RefreshWorldPrompts(float DeltaTime);
    void DrawKeyboardPrompts(UCanvas* Canvas);
    void TickWorkshopSmoke();
    void TickQuestSmoke();
    void TickKeyboardSmoke(float DeltaTime);
    UPROPERTY(Transient) TObjectPtr<UBCRQuestSmokeObserver> QuestSmoke;
    UPROPERTY(Transient) TObjectPtr<UBCRKeyboardSmokeBridge> KeyboardSmoke;
    UPROPERTY(Transient) TObjectPtr<UBCRWorkshopReplayBridge> WorkshopReplay;
    TWeakObjectPtr<UWorld> WorkshopReplayWorld;
    float PromptRefreshTime = 0.f;
    TArray<TWeakObjectPtr<UNiagaraComponent>> WorldPrompts;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> KeyboardHelp;
    UPROPERTY(Transient) TObjectPtr<UFont> KeyboardFont;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> KeyboardPaper;
};
