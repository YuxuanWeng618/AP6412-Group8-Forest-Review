using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.CoreUObject;

namespace ManagedBCR;

[UClass]
public partial class UManagedPlayerRules : UBCRPlayerRulesBridge
{
    public override FVector ResolveMovement(bool canMove, double cameraYawDegrees, double right, double forward)
    {
        var (x, y) = PlayerActionRules.ResolveMovement(canMove, cameraYawDegrees, right, forward);
        return new FVector(x, y, 0);
    }

    public override int ResolveCarry(bool carrying, bool heldActorValid) =>
        (int)PlayerActionRules.ResolveCarry(carrying, heldActorValid);

    public override int ResolvePickup(bool isPickable) =>
        (int)PlayerActionRules.ResolvePickup(isPickable);

    public override int SelectPickupCandidate(IList<int> eligible) =>
        PlayerActionRules.SelectPickupCandidate(eligible);

    public override int ResolveInteraction(bool hasItem, bool isInteractable, bool isHelp) =>
        (int)PlayerActionRules.ResolveInteraction(hasItem, isInteractable, isHelp);
}
