#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRWorkshopReplayBridge.generated.h"

class AActor;
class USceneComponent;
class UQTE_Subsystem;

/** Engine operations used by the managed development replay. */
UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRWorkshopReplayBridge : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    void RunReplayFrame(double Now);

    UFUNCTION(BlueprintCallable) AActor* ReplayPlayer(int32 Index) const;
    UFUNCTION(BlueprintCallable) AActor* ReplayHeld(AActor* Player) const;
    UFUNCTION(BlueprintCallable) AActor* ReplayFind(const FString& ClassName, const FString& QuestTag) const;
    UFUNCTION(BlueprintCallable) USceneComponent* ReplayPart(AActor* Actor, const FString& Name) const;
    UFUNCTION(BlueprintCallable) FString ReplayClass(AActor* Actor) const;
    UFUNCTION(BlueprintCallable) FVector ReplayPosition(UObject* Object) const;
    UFUNCTION(BlueprintCallable) FVector ReplayTriggerPosition(AActor* Actor) const;
    UFUNCTION(BlueprintCallable) FVector ReplayStationPosition(AActor* Actor, int32 Station) const;
    UFUNCTION(BlueprintCallable) FVector ReplayMachineInput(AActor* Actor) const;
    UFUNCTION(BlueprintCallable) FVector ReplayDeliveryPosition(AActor* Actor) const;
    UFUNCTION(BlueprintCallable) void ReplayPlace(AActor* Player, FVector Position);
    UFUNCTION(BlueprintCallable) void ReplayPlaceForGrab(AActor* Player, AActor* Item, double Side);
    UFUNCTION(BlueprintCallable) void ReplayInteract(AActor* Target, AActor* Player, AActor* Item);
    UFUNCTION(BlueprintCallable) void ReplayKey(const FString& Input, bool bDown);
    UFUNCTION(BlueprintCallable) AActor* ReplayTraceGrab(AActor* Player) const;
    UFUNCTION(BlueprintCallable) bool ReplayFlag(AActor* Actor, const FString& Name) const;
    UFUNCTION(BlueprintCallable) double ReplayNumber(AActor* Actor, const FString& Name) const;
    UFUNCTION(BlueprintCallable) void ReplaySetHeld(AActor* Player, AActor* Item);
    UFUNCTION(BlueprintCallable) bool ReplayHasMachineItem(AActor* Actor);
    UFUNCTION(BlueprintCallable) int32 ReplayRecipeRemaining(AActor* Actor) const;
    UFUNCTION(BlueprintCallable) bool ReplayAlignLooseItem(AActor* Assembler, AActor* Item, FVector Position);
    UFUNCTION(BlueprintCallable) FString ReplayQuestTag() const;
    UFUNCTION(BlueprintCallable) int32 ReplayRoundState() const;
    UFUNCTION(BlueprintCallable) void ReplayStartTimer(float Timeout);
    UFUNCTION(BlueprintCallable) void ReplayPauseTimer(bool bPaused);
    UFUNCTION(BlueprintCallable) void ReplayStopTimer();
    UFUNCTION(BlueprintCallable) FString ReplayReportPath() const;
    UFUNCTION(BlueprintCallable) void ReplayExit(int32 ExitCode);
    UFUNCTION(BlueprintCallable) void ReplayLog(const FString& Message);

private:
    UWorld* ReplayWorld() const;
    UQTE_Subsystem* ReplayQte() const;
};
