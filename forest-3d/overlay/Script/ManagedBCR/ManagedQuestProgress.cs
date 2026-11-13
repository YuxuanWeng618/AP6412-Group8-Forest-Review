using UnrealSharp.Attributes;
using UnrealSharp.BCR;

namespace ManagedBCR;

[UClass]
public partial class UManagedQuestProgress : UBCRQuestBridge
{
    private QuestProgress? progress;

    public override bool InitializeQuestProgress(UQuestData data)
    {
        if (progress is { IsPublishing: true })
            return false;

        var line = data.QuestLine;
        if (line.QuestLineData is null)
            return false;
        var next = QuestProgress.FromData(line.Tag, line.QuestLineData.Select(row =>
            new QuestStep(row.Tag, row.Desc, row.Amount, row.NextQuestTag, row.PreviousQuestTag)));
        if (next is null)
            return false;

        progress = next;
        return true;
    }

    public override void SubmitQuestEvent(string tag, bool countProgress)
        => progress?.Submit(tag, countProgress, PublishQuestCompletion);

    public override string ReadQuestTag() => progress?.Current?.Tag ?? string.Empty;
    public override string ReadQuestDescription() => progress?.Current?.Description ?? string.Empty;
    public override int ReadQuestAmount() => progress?.Current?.Amount ?? 0;
    public override int ReadQuestProgress() => progress?.Count ?? 0;
}
