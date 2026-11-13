#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRQuestBridge.generated.h"

class UQuestData;

/** Reflected calls between the existing quest subsystem and its C# state. */
UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRQuestBridge : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    bool InitializeQuestProgress(UQuestData* Data);

    UFUNCTION(BlueprintImplementableEvent)
    void SubmitQuestEvent(const FString& Tag, bool bCountProgress);

    UFUNCTION(BlueprintImplementableEvent)
    FString ReadQuestTag() const;

    UFUNCTION(BlueprintImplementableEvent)
    FString ReadQuestDescription() const;

    UFUNCTION(BlueprintImplementableEvent)
    int32 ReadQuestAmount() const;

    UFUNCTION(BlueprintImplementableEvent)
    int32 ReadQuestProgress() const;

    UFUNCTION(BlueprintCallable)
    void PublishQuestCompletion(const FString& Tag, bool bLineCompleted);
};
