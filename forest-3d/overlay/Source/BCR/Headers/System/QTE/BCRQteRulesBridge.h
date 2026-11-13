#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRQteRulesBridge.generated.h"

/** Reflection boundary for the C# QTE rules. UE owns controllers and delegates. */
UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRQteRulesBridge : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    int32 RoundState();
    UFUNCTION(BlueprintImplementableEvent)
    bool StartRound(int32 FirstSlot, int32 FirstTarget, int32 SecondSlot, int32 SecondTarget, float Timeout);
    UFUNCTION(BlueprintImplementableEvent)
    bool IsRoundActive();
    UFUNCTION(BlueprintImplementableEvent)
    bool JoinRound(int32 Slot, int32 Player);
    UFUNCTION(BlueprintImplementableEvent)
    bool OwnsRoundSlot(int32 Slot, int32 Player);
    UFUNCTION(BlueprintImplementableEvent)
    int32 SuccessCount(int32 Slot);
    UFUNCTION(BlueprintImplementableEvent)
    bool RecordSuccess(int32 Slot);
    UFUNCTION(BlueprintImplementableEvent)
    bool PauseRound(bool bPaused);
    UFUNCTION(BlueprintImplementableEvent)
    bool FinishRound(bool bSuccess, bool bCancelled);
    UFUNCTION(BlueprintImplementableEvent)
    bool FinishAuthored();
    UFUNCTION(BlueprintImplementableEvent)
    float RoundTimeout();
    UFUNCTION(BlueprintImplementableEvent)
    bool ExpireRound();

    UFUNCTION(BlueprintImplementableEvent)
    bool ShouldWait(bool bSynchronize, int32 Current, int32 Other);

    UFUNCTION(BlueprintImplementableEvent)
    bool ReachedRepeatLimit(int32 Target, int32 Successes);

    UFUNCTION(BlueprintImplementableEvent)
    int32 EvaluateAction(int32 Action, bool bPressed, bool bReleased, bool bHeld, float X, float Y, float Minimum);
    UFUNCTION(BlueprintImplementableEvent)
    float CalculateActionProgress(int32 Action, bool bHeld, float X, float Y, float Minimum);
};
