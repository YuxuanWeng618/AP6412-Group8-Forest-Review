#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "BecornCSharpProbe.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBecornNativePulse, int32, Value);

/** Small, isolated reflection boundary. No original gameplay rules are replaced. */
UCLASS(Blueprintable)
class BCR_API ABecornCSharpProbe : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Phase A")
    int32 NativeValue = 0;

    UPROPERTY(BlueprintAssignable, Category="Phase A")
    FBecornNativePulse OnNativePulse;

    UFUNCTION(BlueprintCallable, Category="Phase A")
    int32 NativeDouble(int32 Value) const { return Value * 2; }

    UFUNCTION(BlueprintCallable, Category="Phase A")
    void RecordManagedStart(int32 Result, const FString& Runtime, const FString& CoreLibrary, const FString& ManagedAssembly);

    UFUNCTION(BlueprintCallable, Category="Phase A")
    void RecordManagedEvent(int32 Value) { EventValue = Value; ++EventCount; }

    UFUNCTION(BlueprintCallable, Category="Phase A")
    void RecordManagedEnd() { bManagedEnded = true; }

    int32 StartResult = 0;
    int32 EventValue = 0;
    int32 EventCount = 0;
    bool bManagedStarted = false;
    bool bManagedEnded = false;
    FString RuntimeVersion;
    FString CoreLibraryPath;
    FString ManagedAssemblyPath;
};

/** Entry point for the PhaseA_CSharp map only; works in editor and packaged game. */
UCLASS()
class BCR_API ABecornCSharpProbeGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ABecornCSharpProbeGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void RunProbe();
    void Finish();
    void ShowStatus(const FString& Text, const FColor& Color);
    float Elapsed = 0;
    bool bFinished = false;
    bool bExitScheduled = false;
    float ExitAt = 0;
    TArray<TSharedPtr<class FJsonValue>> Checks;
    UPROPERTY() TObjectPtr<class ATextRenderActor> StatusText;
    FString RuntimeVersion;
    FString CoreLibraryPath;
    FString ManagedAssemblyPath;
    FString ManagedClassPath;
};
