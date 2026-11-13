#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRQuestSmokeObserver.generated.h"

class UQuestData;
class UQuest_Subsystem;
class UGameInstance;

UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRQuestSmokeObserver : public UObject
{
    GENERATED_BODY()
public:
    virtual UWorld* GetWorld() const override { return BoundWorld.Get(); }

    UFUNCTION(BlueprintImplementableEvent) void RunQuestFrame(double Now);
    UFUNCTION(BlueprintImplementableEvent) void ObserveQuestEvent(bool bLine, const FString& Tag, const FString& CurrentTag, int32 Progress);

    UFUNCTION(BlueprintCallable) UQuest_Subsystem* ReplayQuest() const;
    UFUNCTION(BlueprintCallable) UObject* ReplayQuestBridge() const;
    UFUNCTION(BlueprintCallable) bool ReplayHasManagedQuest() const;
    UFUNCTION(BlueprintCallable) UQuestData* ReplayQuestData() const;
    UFUNCTION(BlueprintCallable) int32 ReplayAssetCount(UQuestData* Data) const;
    UFUNCTION(BlueprintCallable) FString ReplayAssetLine(UQuestData* Data) const;
    UFUNCTION(BlueprintCallable) FString ReplayAssetDescription(UQuestData* Data, const FString& Tag) const;
    UFUNCTION(BlueprintCallable) bool ReplayHasHud() const;
    UFUNCTION(BlueprintCallable) FString ReplayHudText() const;
    UFUNCTION(BlueprintCallable) bool ReplaySameWorld() const;
    UFUNCTION(BlueprintCallable) bool ReplaySameInstance() const;
    UFUNCTION(BlueprintCallable) void ReplayListen();
    UFUNCTION(BlueprintCallable) bool ReplayIsListening() const;
    UFUNCTION(BlueprintCallable) void ReplayNotifyOutput();
    UFUNCTION(BlueprintCallable) void ReplayStart(bool bDown);
    UFUNCTION(BlueprintCallable) void ReplayReload();
    UFUNCTION(BlueprintCallable) FString ReplayQuestReportPath() const;
    UFUNCTION(BlueprintCallable) void ReplayQuestLog(const FString& Message);
    UFUNCTION(BlueprintCallable) void ReplayQuestExit(int32 ExitCode);

private:
    UFUNCTION() void OnStep(FString Tag);
    UFUNCTION() void OnLine(FString Tag);
    UWorld* ActiveWorld() const;
    TWeakObjectPtr<UWorld> BoundWorld;
    TWeakObjectPtr<UGameInstance> BoundInstance;
};
