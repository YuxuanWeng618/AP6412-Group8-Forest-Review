namespace ManagedBCR;

// These values match the UE Data Asset enums in QTETypes.h.
internal enum QteActionType { None, Press, Hold, Release, Rotate }
internal enum QteResult { None, Success, Failure }

internal static class QteActionRules
{
    internal static bool ShouldWait(bool synchronize, int current, int other) =>
        synchronize && current > 0 && current > other;

    internal static bool ReachedRepeatLimit(int target, int successes) =>
        target != -1 && successes >= target;

    internal static QteResult Evaluate(QteActionType type, bool pressed, bool released,
        bool held, float x, float y, float minimum)
    {
        return type switch
        {
            QteActionType.Press => ButtonEdge(pressed, released),
            QteActionType.Release => ButtonEdge(released, pressed),
            QteActionType.Hold => held ? QteResult.Success : QteResult.None,
            QteActionType.Rotate => Rotation(x, y, minimum),
            _ => QteResult.None
        };
    }

    internal static float Progress(QteActionType type, bool held, float x, float y, float minimum)
    {
        if (type is QteActionType.Press or QteActionType.Hold or QteActionType.Release)
            return held ? 1f : 0f;
        if (type != QteActionType.Rotate || !float.IsFinite(x) || !float.IsFinite(y) ||
            !float.IsFinite(minimum) || minimum <= 0f)
            return 0f;
        double magnitude = Magnitude(x, y);
        // The authored saw reads the raw ratio, including values above one.
        return (float)magnitude > 0.1f ? (float)(magnitude / minimum) : 0f;
    }

    private static QteResult ButtonEdge(bool expected, bool opposite)
    {
        if (expected) return QteResult.Success;
        return opposite ? QteResult.Failure : QteResult.None;
    }

    private static QteResult Rotation(float x, float y, float minimum)
    {
        if (!float.IsFinite(x) || !float.IsFinite(y) || !float.IsFinite(minimum))
            return QteResult.None;
        float magnitude = (float)Magnitude(x, y);
        if (magnitude == 0) return QteResult.None;
        return magnitude >= minimum ? QteResult.Success : QteResult.Failure;
    }

    private static double Magnitude(float x, float y) => Math.Sqrt((double)x * x + (double)y * y);
}
