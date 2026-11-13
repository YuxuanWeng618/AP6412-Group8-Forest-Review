using UnrealSharp.Attributes;
using UnrealSharp.BCR;

namespace ManagedBCR;

[UClass]
public partial class UManagedQteRules : UBCRQteRulesBridge
{
    private readonly QteRound round = new();

    public override int RoundState() => (int)round.State;
    public override bool StartRound(int firstSlot, int firstTarget, int secondSlot, int secondTarget, float timeout) =>
        round.StartConfigured(firstSlot, firstTarget, secondSlot, secondTarget, timeout);
    public override bool IsRoundActive() => round.Active;
    public override bool JoinRound(int slot, int player) => round.Join(slot, player);
    public override bool OwnsRoundSlot(int slot, int player) => round.Owns(slot, player);
    public override int SuccessCount(int slot) => round.Successes(slot);
    public override bool RecordSuccess(int slot) => round.RecordSuccess(slot);
    public override bool PauseRound(bool paused) => round.SetPaused(paused);
    public override bool FinishRound(bool success, bool cancelled) => round.Finish(success, cancelled);
    public override bool FinishAuthored() => round.FinishAuthored();
    public override float RoundTimeout() => round.TimeoutSeconds;
    public override bool ExpireRound() => round.Expire();

    public override bool ShouldWait(bool synchronize, int current, int other) =>
        QteActionRules.ShouldWait(synchronize, current, other);

    public override bool ReachedRepeatLimit(int target, int successes) =>
        QteActionRules.ReachedRepeatLimit(target, successes);

    public override int EvaluateAction(int action, bool pressed, bool released, bool held,
        float x, float y, float minimum) =>
        (int)QteActionRules.Evaluate((QteActionType)action, pressed, released, held, x, y, minimum);

    public override float CalculateActionProgress(int action, bool held, float x, float y, float minimum) =>
        QteActionRules.Progress((QteActionType)action, held, x, y, minimum);
}
