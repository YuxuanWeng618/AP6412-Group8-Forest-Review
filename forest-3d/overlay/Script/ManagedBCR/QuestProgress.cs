namespace ManagedBCR;

internal sealed record QuestStep(string Tag, string Description, int Amount, string NextTag, string PreviousTag);

internal sealed class QuestProgress
{
    private readonly string lineTag;
    private readonly IReadOnlyList<QuestStep> steps;
    private int index;

    private QuestProgress(string lineTag, IReadOnlyList<QuestStep> steps)
    {
        this.lineTag = lineTag;
        this.steps = steps;
    }

    public QuestStep? Current => index < steps.Count ? steps[index] : null;
    public int Count { get; private set; }
    public bool IsPublishing { get; private set; }

    public static QuestProgress? FromData(string lineTag, IEnumerable<QuestStep> rows)
    {
        var byTag = new Dictionary<string, QuestStep>(StringComparer.OrdinalIgnoreCase);
        QuestStep? first = null;
        foreach (var row in rows)
        {
            if (string.IsNullOrEmpty(row.Tag) || row.Amount < 0 || !byTag.TryAdd(row.Tag, row))
                return null;
            if (string.IsNullOrEmpty(row.PreviousTag))
            {
                if (first is not null) return null;
                first = row;
            }
        }

        if (first is null)
            return null;

        var ordered = new List<QuestStep>(byTag.Count);
        var visited = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        QuestStep? step = first;
        while (step is not null)
        {
            if (!visited.Add(step.Tag))
                return null;
            ordered.Add(step);
            if (string.IsNullOrEmpty(step.NextTag))
                break;
            if (!byTag.TryGetValue(step.NextTag, out var next) ||
                !StringComparer.OrdinalIgnoreCase.Equals(next.PreviousTag, step.Tag))
                return null;
            step = next;
        }
        return ordered.Count == byTag.Count ? new QuestProgress(lineTag, ordered) : null;
    }

    public void Submit(string tag, bool countProgress, Action<string, bool> publish)
    {
        if (IsPublishing || Current is not { } step ||
            !StringComparer.OrdinalIgnoreCase.Equals(tag, step.Tag))
            return;

        if (countProgress && ++Count != step.Amount)
            return;

        bool lineCompleted = index + 1 == steps.Count;
        IsPublishing = true;
        try
        {
            // Blueprint delegates have always observed the completing step before it advances.
            publish(lineCompleted ? lineTag : step.Tag, lineCompleted);
        }
        finally
        {
            index++;
            Count = 0;
            IsPublishing = false;
        }
    }
}
