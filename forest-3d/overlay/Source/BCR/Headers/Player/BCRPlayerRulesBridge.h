#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRPlayerRulesBridge.generated.h"

// Values are the C++/C# reflection contract in PlayerActionRules.cs.
enum class EBCRPlayerAction : int32
{
    None,
    Drop,
    ClearInvalidCarry,
    SelectPickup,
    Interact,
    InteractWithItem,
    OpenHelp
};

UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRPlayerRulesBridge : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent)
    FVector ResolveMovement(bool bCanMove, double CameraYawDegrees, double Right, double Forward);

    UFUNCTION(BlueprintImplementableEvent)
    int32 ResolveCarry(bool bCarrying, bool bHeldActorValid);

    UFUNCTION(BlueprintImplementableEvent)
    int32 ResolvePickup(bool bIsPickable);

    UFUNCTION(BlueprintImplementableEvent)
    int32 SelectPickupCandidate(const TArray<int32>& Eligible);

    UFUNCTION(BlueprintImplementableEvent)
    int32 ResolveInteraction(bool bHasItem, bool bIsInteractable, bool bIsHelp);
};
