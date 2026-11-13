namespace ManagedBCR;

internal enum PlayerAction
{
    None,
    Drop,
    ClearInvalidCarry,
    SelectPickup,
    Interact,
    InteractWithItem,
    OpenHelp
}

internal static class PlayerActionRules
{
    internal static (double X, double Y) ResolveMovement(
        bool canMove, double cameraYawDegrees, double right, double forward)
    {
        if (!canMove || !double.IsFinite(cameraYawDegrees) ||
            !double.IsFinite(right) || !double.IsFinite(forward))
            return default;

        var (sin, cos) = Math.SinCos(cameraYawDegrees * (Math.PI / 180));
        return (forward * cos - right * sin, forward * sin + right * cos);
    }

    internal static PlayerAction ResolveCarry(bool carrying, bool heldActorValid)
    {
        // Original Blueprint pickup can attach an actor before the native toggle flag is set.
        if (heldActorValid) return PlayerAction.Drop;
        return carrying ? PlayerAction.ClearInvalidCarry : PlayerAction.None;
    }

    internal static PlayerAction ResolvePickup(bool isPickable) =>
        isPickable ? PlayerAction.SelectPickup : PlayerAction.None;

    internal static int SelectPickupCandidate(IList<int> eligible)
    {
        // The original sweep kept the last eligible hit in UE's order.
        for (int index = eligible.Count - 1; index >= 0; index--)
            if (eligible[index] != 0) return index;
        return -1;
    }

    internal static PlayerAction ResolveInteraction(bool hasItem, bool isInteractable, bool isHelp)
    {
        if (!isInteractable) return PlayerAction.None;
        if (hasItem) return PlayerAction.InteractWithItem;
        return isHelp ? PlayerAction.OpenHelp : PlayerAction.Interact;
    }
}
