using System.Runtime.InteropServices;
using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.Engine;

namespace ManagedBCR;

[UClass]
public partial class AManagedPhaseAActor : ABecornCSharpProbe
{
    [UProperty(PropertyFlags.BlueprintReadWrite)]
    public partial int ManagedValue { get; set; }

    public override void BeginPlay()
    {
        base.BeginPlay();
        NativeValue = 21;
        ManagedValue = 7;
        OnNativePulse += HandleNativePulse;
        RecordManagedStart(NativeDouble(NativeValue), RuntimeInformation.FrameworkDescription,
            typeof(object).Assembly.Location, GetType().Assembly.Location);
    }

    [UFunction(FunctionFlags.BlueprintCallable)]
    public int ManagedMultiply(int value) => ManagedValue * value;

    [UFunction]
    private void HandleNativePulse(int value) => RecordManagedEvent(ManagedValue + value);

    public override void EndPlay(EEndPlayReason endPlayReason)
    {
        OnNativePulse -= HandleNativePulse;
        RecordManagedEnd();
        base.EndPlay(endPlayReason);
    }
}
