#pragma once

#include "CoreMinimal.h"
#include "BCRWorkshopRulesBridge.generated.h"

struct FForestWorkshopRulesState;

/**
 * UE adapter for the workshop rules. Station occupancy, the recipe, the two-rope
 * pull and the one-off outputs are decided by the shared Forest rules that the 2D
 * game also compiles (forest-2d/Source/Forest2D/Domain/WorkshopSession.h). UE passes
 * player indices, item identities, distances and spawn results in.
 */
UCLASS(Transient)
class BCR_API UBCRWorkshopRulesBridge : public UObject
{
    GENERATED_BODY()

public:
    UBCRWorkshopRulesBridge();
    static UBCRWorkshopRulesBridge* Create(UObject* Owner);

    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    int32 InteractWithStation(int32 Player, bool bSupplied, float FirstDistance, float SecondDistance);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool AreStationsFilled();

    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void ResetMachineRequirements();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void AddMachineRequirement(const FString& ItemName);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    int32 FindMachineRequirement(const FString& ItemName);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool IsMachineSupplied();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool HasMachineItems();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    int32 DepositMachineItem(int32 Item, const FString& ItemName, bool bInside);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void StageMachineReplacement(int32 Index);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool RegisterMachineReplacement(int32 Item);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void ForgetMachineItem(int32 Item);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool CanStartMachineRound(int32 State);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ConfirmMachineRound(int32 State);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool OwnsMachineRound();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void ReleaseMachineRound();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ShouldFinishSawRound(bool bCut);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ShouldFinishTreeRound(int32 CutsRemaining);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ShouldUnlockReplacementCarry(bool bHeldIsValid, bool bHeldWasReplacedLog);

    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void ResetRecipe();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void AddIngredient(int32 Type, int32 Count);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    int32 RemainingIngredient(int32 Type);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    int32 RemainingTotal();
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ReserveIngredient(int32 Item, int32 Type);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool FinishIngredient(int32 Item, bool bConsumed);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ShouldQueueIngredient(bool bStillHeld, bool bInZone);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool CanConsumeIngredient(bool bAttached, bool bInZone);

    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    void ObserveRopes(int32 LeftPlayer, int32 RightPlayer, bool bLeftPulled, bool bRightPulled);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ReserveCraft(int32 LeftPlayer, int32 RightPlayer, bool bLeftPulled, bool bRightPulled);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool ReserveDelivery(int32 ActualType, int32 RequiredType);
    UFUNCTION(BlueprintCallable, Category = "Forest|Workshop")
    bool FinishOutput(bool bSpawned);

private:
    TSharedPtr<FForestWorkshopRulesState> State;
};
