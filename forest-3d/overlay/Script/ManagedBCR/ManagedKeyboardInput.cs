using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.CoreUObject;
using UnrealSharp.UMG;

namespace ManagedBCR;

[UClass]
public partial class UManagedKeyboardInput : UBCRKeyboardBridge
{
    private readonly KeyboardInputPolicy policy = new();
    private bool keyboardPaused;
    private bool pausedBeforeHelp;

    public override bool RouteViewportKey(string physicalKey, bool pressed, bool released,
        bool inputIgnored, bool paused, bool helpOpen, bool p1HasPawn, bool p2HasPawn)
    {
        if (pressed && physicalKey == "H")
        {
            RequestKeyboardHelpToggle(paused, helpOpen);
            return true;
        }

        if (helpOpen)
            return physicalKey is "P" or "Tab" || KeyboardInputPolicy.TryGetBinding(physicalKey, out _);

        if (pressed && physicalKey == "Tab")
        {
            foreach (var widget in FindViewportQuestWidgets())
            {
                var next = widget.Visibility == ESlateVisibility.Collapsed
                    ? ESlateVisibility.SelfHitTestInvisible : ESlateVisibility.Collapsed;
                ApplyViewportQuestVisibility(widget, next);
            }
            return true;
        }

        if (pressed && physicalKey == "P" && (keyboardPaused || (!inputIgnored && p1HasPawn)))
        {
            ClearKeyboard();
            if (ApplyViewportPause(!paused))
                keyboardPaused = !paused;
            return true;
        }

        if (!KeyboardInputPolicy.TryGetBinding(physicalKey, out var binding))
            return false;
        if (released)
        {
            policy.Release(physicalKey);
            if (!binding.IsDirection)
                SendKeyboardButton(binding.Player, binding.Control, false);
            return true;
        }
        if (inputIgnored || paused || !(binding.Player == 0 ? p1HasPawn : p2HasPawn))
            return false;
        if (pressed && policy.Press(physicalKey) && !binding.IsDirection)
            SendKeyboardButton(binding.Player, binding.Control, true);
        return true;
    }

    public override void RequestKeyboardHelpToggle(bool paused, bool helpOpen)
    {
        ClearKeyboard();
        if (helpOpen)
        {
            if (SetViewportHelpVisible(false) && ApplyViewportPause(pausedBeforeHelp))
                keyboardPaused = pausedBeforeHelp;
            return;
        }

        if (SetViewportHelpVisible(true))
        {
            pausedBeforeHelp = paused;
            ApplyViewportPause(true);
        }
    }

    public override void OnViewportFocusLost(bool inputIgnored, bool paused, bool p1HasPawn)
    {
        ClearKeyboard();
        if (!inputIgnored && !paused && p1HasPawn && ApplyViewportPause(true))
            keyboardPaused = true;
    }

    public override void TickViewportInput(bool inputIgnored, bool paused, bool helpOpen)
    {
        if (inputIgnored || paused || helpOpen)
        {
            if (policy.HasHeldKeys)
                ClearKeyboard();
            return;
        }

        for (int player = 0; player < 2; player++)
        {
            var stick = policy.SampleStick(player);
            SendKeyboardAxes(player, stick.X, stick.Y);
        }
    }

    private void ClearKeyboard()
    {
        foreach (var binding in policy.Reset())
            SendKeyboardButton(binding.Player, binding.Control, false);
        SendKeyboardAxes(0, 0, 0);
        SendKeyboardAxes(1, 0, 0);
    }

    public override string GetActionLabel(int playerIndex, string controllerKey)
        => KeyboardInputPolicy.ActionLabel(playerIndex, controllerKey);

    public override string GetHorizontalLabel(int playerIndex)
        => KeyboardInputPolicy.HorizontalLabel(playerIndex);

    public override IList<string> OverviewKeyboardPrompts(bool paused) => KeyboardPromptRules.Overview(paused);

    public override string QteKeyboardPrompt(int playerIndex, string label, bool waiting) =>
        KeyboardPromptRules.Qte(playerIndex, label, waiting);

    public override double PositionKeyboardStatus(double preferred, double height, double overviewTop, double scale) =>
        KeyboardPromptRules.StatusTop(preferred, height, overviewTop, scale);

    public override string WorldKeyboardPrompt(string asset, bool isRope, int ropePlayer, bool isMachine,
        int firstPlayer, int secondPlayer, double firstDistance, double secondDistance) =>
        KeyboardPromptRules.World(asset, isRope, ropePlayer, isMachine, firstPlayer, secondPlayer, firstDistance, secondDistance);

    public override FVector2D PlaceKeyboardPrompt(FVector screen, double width, double height, double scale,
        IList<FVector2D> placed)
    {
        var position = KeyboardPromptRules.Place(screen.X, screen.Y, screen.Z, width, height, scale,
            placed.Select(point => new PromptPoint(point.X, point.Y)));
        return position is { } point ? new FVector2D(point.X, point.Y) : new FVector2D(double.NaN, double.NaN);
    }
}
