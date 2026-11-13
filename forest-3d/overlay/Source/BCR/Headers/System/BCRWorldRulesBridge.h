#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BCRWorldRulesBridge.generated.h"

UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRTriggerRulesBridge : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    int32 ChangeOccupancy(bool bInner, int32 Player, int32 Component, int32 Body, bool bEntered);
};

USTRUCT(BlueprintType)
struct BCR_API FBCRCameraFraming
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) double ArmLength = 0.0;
    UPROPERTY(BlueprintReadWrite) double HorizontalLimit = 0.0;
    UPROPERTY(BlueprintReadWrite) double DepthLimit = 0.0;
    UPROPERTY(BlueprintReadWrite) double HorizontalDistance = 0.0;
    UPROPERTY(BlueprintReadWrite) double DepthDistance = 0.0;
    UPROPERTY(BlueprintReadWrite) double Pitch = 0.0;
    UPROPERTY(BlueprintReadWrite) double FogDistance = 0.0;
    UPROPERTY(BlueprintReadWrite) double Aperture = 0.0;
};

UCLASS(Abstract, Blueprintable, Transient)
class BCR_API UBCRCameraRulesBridge : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent)
    FVector CameraCenter(FVector First, FVector Second, bool bVertical, double Offset);

    UFUNCTION(BlueprintImplementableEvent)
    FBCRCameraFraming FramePlayers(FVector Separation, FVector Right, FVector Forward, double CurrentPitch,
        double HorizontalFov, double VerticalFov, double MinArm, double MaxArm, double HorizontalBuffer,
        double DepthBuffer, double MinAngleArm, double MaxAngleArm, double MinAngle, double MaxAngle, double Easing,
        double FogOffset, double BlurNear, double BlurFar, double MinFStop, double MaxFStop, double BlurMultiplier);

    UFUNCTION(BlueprintImplementableEvent)
    FVector ConstrainDisplacement(FVector Separation, FVector Displacement, FVector Right, FVector Forward,
        double HorizontalLimit, double DepthLimit);
};
