using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.CoreUObject;

namespace ManagedBCR;

[UClass]
public partial class UManagedTriggerRules : UBCRTriggerRulesBridge
{
    private readonly TriggerOccupancy occupancy = new();

    public override int ChangeOccupancy(bool inner, int player, int component, int body, bool entered) =>
        (int)occupancy.Change(inner, player, component, body, entered);
}

[UClass]
public partial class UManagedCameraRules : UBCRCameraRulesBridge
{
    public override FVector CameraCenter(FVector first, FVector second, bool vertical, double offset) =>
        Write(SharedCameraRules.Center(Read(first), Read(second), vertical, offset));

    public override FBCRCameraFraming FramePlayers(FVector separation, FVector right, FVector forward,
        double currentPitch, double horizontalFov, double verticalFov, double minArm, double maxArm,
        double horizontalBuffer, double depthBuffer, double minAngleArm, double maxAngleArm, double minAngle,
        double maxAngle, double easing, double fogOffset, double blurNear, double blurFar,
        double minFStop, double maxFStop, double blurMultiplier)
    {
        var settings = new CameraSettings(minArm, maxArm, horizontalBuffer, depthBuffer, minAngleArm, maxAngleArm,
            minAngle, maxAngle, easing, fogOffset, blurNear, blurFar, minFStop, maxFStop, blurMultiplier);
        var frame = SharedCameraRules.Frame(Read(separation), Read(right), Read(forward), currentPitch,
            horizontalFov, verticalFov, settings);
        return new FBCRCameraFraming
        {
            ArmLength = frame.ArmLength,
            HorizontalLimit = frame.HorizontalLimit,
            DepthLimit = frame.DepthLimit,
            HorizontalDistance = frame.HorizontalDistance,
            DepthDistance = frame.DepthDistance,
            Pitch = frame.Pitch,
            FogDistance = frame.FogDistance,
            Aperture = frame.Aperture
        };
    }

    public override FVector ConstrainDisplacement(FVector separation, FVector displacement, FVector right,
        FVector forward, double horizontalLimit, double depthLimit) =>
        Write(SharedCameraRules.Correction(Read(separation), Read(displacement), Read(right), Read(forward),
            horizontalLimit, depthLimit));

    private static CameraVector Read(FVector vector) => new(vector.X, vector.Y, vector.Z);
    private static FVector Write(CameraVector vector) => new(vector.X, vector.Y, vector.Z);
}
