#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Tests/OrderReplay.h"
#include "ForestController.generated.h"
UCLASS()
class AForestController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
private:
    bool HadFocus = true;
    bool ReplayEnabled = false;
    bool ReplaySaved = false;
    float ReplayEndDelay = 0;
    float CaptureAge = 0;
    bool Captured = false;
    bool WorldCaptured = false;
    FOrderReplay Replay;
};
