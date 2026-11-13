namespace ManagedBCR;

internal enum QteRoundState { Inactive, Waiting, Running, Paused, Completed, Failed }

internal sealed class QteRound
{
    private const int Disabled = -2;
    private readonly WorkstationSlots slots = new();
    private readonly int[] targets = new int[2];
    private readonly int[] successes = new int[2];
    private QteRoundState resumeState;
    private int requiredPlayers;

    public QteRoundState State { get; private set; }
    public bool Active => State is QteRoundState.Waiting or QteRoundState.Running or QteRoundState.Paused;
    public float TimeoutSeconds { get; private set; }

    public bool StartConfigured(int firstSlot, int firstTarget, int secondSlot, int secondTarget, float timeout)
    {
        if (firstSlot is < 0 or > 1 || firstTarget < -1 || secondSlot is < -1 or > 1 ||
            secondSlot == firstSlot || (secondSlot >= 0 && secondTarget < -1))
            return false;

        int first = firstSlot == 0 ? firstTarget : secondSlot == 0 ? secondTarget : Disabled;
        int second = firstSlot == 1 ? firstTarget : secondSlot == 1 ? secondTarget : Disabled;
        return Start(first, second, timeout);
    }

    public bool Start(int first, int second, float timeout = 0f)
    {
        if (Active || first < Disabled || second < Disabled || (first == Disabled && second == Disabled))
            return false;
        slots.Reset();
        successes[0] = successes[1] = 0;
        targets[0] = first;
        targets[1] = second;
        requiredPlayers = (first != Disabled ? 1 : 0) + (second != Disabled ? 1 : 0);
        TimeoutSeconds = float.IsFinite(timeout) && timeout > 0f ? timeout : 0f;
        State = QteRoundState.Waiting;
        return true;
    }

    public bool Join(int slot, int player)
    {
        if (State != QteRoundState.Waiting || slot is < 0 or > 1 || targets[slot] == Disabled || !slots.Join(slot, player))
            return false;
        if (slots.Count == requiredPlayers) State = QteRoundState.Running;
        return true;
    }

    public bool Owns(int slot, int player) => Active && slot >= 0 && slots.Find(player) == slot;

    public int Successes(int slot) => slot is 0 or 1 ? successes[slot] : 0;

    public bool RecordSuccess(int slot)
    {
        if (State != QteRoundState.Running || slot is < 0 or > 1 || targets[slot] == Disabled) return false;
        if (successes[slot] < int.MaxValue) successes[slot]++;
        return QteActionRules.ReachedRepeatLimit(targets[slot], successes[slot]) && IsDone(0) && IsDone(1);
    }

    public bool SetPaused(bool paused)
    {
        if (!Active || paused == (State == QteRoundState.Paused)) return false;
        if (paused) resumeState = State;
        State = paused ? QteRoundState.Paused : resumeState;
        return true;
    }

    public bool Finish(bool success, bool cancelled)
    {
        if (!Active) return false;
        State = cancelled ? QteRoundState.Inactive : success ? QteRoundState.Completed : QteRoundState.Failed;
        TimeoutSeconds = 0f;
        slots.Reset();
        return true;
    }

    public bool Expire() => TimeoutSeconds > 0f && (State is QteRoundState.Waiting or QteRoundState.Running)
        && Finish(false, false);

    public bool FinishAuthored() => State == QteRoundState.Running && Finish(true, false);

    private bool IsDone(int slot) => targets[slot] < 0 ||
        QteActionRules.ReachedRepeatLimit(targets[slot], successes[slot]);
}
