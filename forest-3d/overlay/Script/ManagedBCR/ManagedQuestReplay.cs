using System.Text.Json;
using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.CoreUObject;

namespace ManagedBCR;

[UClass]
public partial class UManagedQuestReplay : UBCRQuestSmokeObserver
{
    private static readonly string[] Tags =
    [
        "the_order", "find_the_mushroom", "cut_the_mushroom", "bring_back_the_log",
        "put_the_log_in_the_machine", "cut_the_log", "put_the_log_in_the_assembler",
        "activate_the_assembler", "deliver_the_product"
    ];

    private readonly List<(string Tag, string Snapshot)> steps = [];
    private readonly List<(string Tag, string Snapshot)> lines = [];
    private readonly List<ReplayCheck> checks = [];
    private UObject? initialBridge;
    private int phase;
    private double since = -1;
    private double nextStart;
    private bool startHeld;
    private bool triedReentry;

    public override void ObserveQuestEvent(bool line, string tag, string currentTag, int progress)
    {
        if (line)
        {
            lines.Add((tag, currentTag));
            return;
        }

        steps.Add((tag, $"{currentTag}:{progress}"));
        if (!triedReentry)
        {
            triedReentry = true;
            ReplayQuest().CompleteQuest(tag);
        }
    }

    public override void RunQuestFrame(double now)
    {
        if (phase == 3) return;
        if (since < 0) since = now;

        var quest = ReplayQuest();
        if (!quest.IsValid()) return;
        string current = quest.CurrentQuestTag;
        double elapsed = now - since;

        if (phase != 1) PulseStart(now, current);
        var data = ReplayQuestData();
        if (!data.IsValid())
        {
            Check("load the original quest asset", false);
            Finish();
            return;
        }

        switch (phase)
        {
            case 0:
                bool ready = ReplayHasHud() && current == Tags[0] && ReplayHudText() == quest.CurrentQuestDesc;
                if (!ready && elapsed < 45) return;
                CheckStartup(quest);
                since = now;
                break;
            case 1:
                bool hudAdvanced = ReplayHasHud() && ReplayHudText() == quest.CurrentQuestDesc;
                if (!hudAdvanced && elapsed < 6) return;
                CheckSequence(quest, data, hudAdvanced);
                phase = 2;
                since = now;
                nextStart = now + 7;
                startHeld = false;
                ReplayReload();
                break;
            case 2:
                if ((ReplaySameWorld() || current != Tags[0]) && elapsed < 45) return;
                CheckReload(quest);
                Finish();
                break;
        }
    }

    private void PulseStart(double now, string current)
    {
        if (now - since < 7 || now < nextStart) return;
        if (startHeld)
        {
            ReplayStart(false);
            startHeld = false;
            nextStart = now + 1;
        }
        else if (current.Length == 0)
        {
            ReplayStart(true);
            startHeld = true;
            nextStart = now + 0.25;
        }
    }

    private void CheckStartup(UQuest_Subsystem quest)
    {
        string description = quest.CurrentQuestDesc;
        Check("original Blueprint startup loaded the first quest into the real managed class",
            quest.CurrentQuestTag == Tags[0] && ReplayHasManagedQuest());
        Check("original quest HUD shows the first bilingual asset description",
            ReplayHasHud() && ReplayHudText() == description && description.Contains("forest") && description.Contains('\n'));
        if (checks.Any(check => !check.passed))
        {
            Finish();
            return;
        }

        initialBridge = ReplayQuestBridge();
        ReplayListen();
        quest.ProgressQuest("invalid-quest");
        quest.CompleteQuest(Tags[1]);
        Check("wrong and future tags cannot change the active quest",
            quest.CurrentQuestTag == Tags[0] && quest.CurrentQuestProgress == 0 && steps.Count == 0);

        quest.ProgressQuest(Tags[0]);
        Check("C++ calls C# and C# returns exactly one native event with the old snapshot",
            triedReentry && steps.SequenceEqual([(Tags[0], Tags[0] + ":1")]) &&
            quest.CurrentQuestTag == Tags[1] && quest.CurrentQuestProgress == 0);
        phase = 1;
    }

    private void CheckSequence(UQuest_Subsystem quest, UQuestData data, bool hudAdvanced)
    {
        Check("existing Blueprint HUD renders the next bilingual quest after the managed event", hudAdvanced);
        Check("quest asset contains all nine real steps", ReplayAssetCount(data) == Tags.Length);
        quest.ProgressQuest(Tags[0]);
        Check("replaying the previous completed tag does not increment the next step",
            quest.CurrentQuestTag == Tags[1] && quest.CurrentQuestProgress == 0);

        for (int index = 1; index < Tags.Length; index++)
        {
            string tag = Tags[index];
            int amount = index == 6 ? 3 : 1;
            string expected = ReplayAssetDescription(data, tag);
            Check($"load {tag} from the original bilingual asset with target {amount}",
                expected.Length > 0 && quest.CurrentQuestTag == tag && quest.CurrentQuestAmount == amount &&
                quest.CurrentQuestDesc == expected && quest.CurrentQuestProgress == 0);
            if (index == 1)
            {
                quest.CompleteQuest(tag);
                Check("direct CompleteQuest keeps the public Blueprint behavior",
                    steps.LastOrDefault().Snapshot == tag + ":0" && quest.CurrentQuestTag == Tags[2]);
                continue;
            }

            for (int count = 1; count <= amount; count++)
            {
                quest.ProgressQuest(tag);
                if (index == 7)
                {
                    Check("legacy rope presentation cannot advance before managed output",
                        quest.CurrentQuestTag == tag && quest.CurrentQuestProgress == 0);
                    ReplayNotifyOutput();
                }
                if (count < amount)
                    Check($"assembly plank {count} of 3 increments without completing",
                        quest.CurrentQuestProgress == count && quest.CurrentQuestTag == tag);
            }
        }

        Check("eight step events arrive in order and all listeners see pre-advance state",
            steps.SequenceEqual(Tags[..^1].Select((tag, index) => (tag, $"{tag}:{(index == 1 ? 0 : index == 6 ? 3 : 1)}"))));
        Check("final delivery emits only the quest-line event and exposes delivery during the callback",
            lines.SequenceEqual([(ReplayAssetLine(data), Tags[^1])]));
        quest.ProgressQuest(Tags[^1]);
        quest.CompleteQuest("");
        Check("finished quest line clears all HUD values and cannot emit twice",
            quest.CurrentQuestTag.Length == 0 && quest.CurrentQuestDesc.Length == 0 &&
            quest.CurrentQuestAmount == 0 && quest.CurrentQuestProgress == 0 && lines.Count == 1);
        quest.LoadQuestLine(data);
        Check("explicit reload restarts at the first quest with zero progress",
            quest.CurrentQuestTag == Tags[0] && quest.CurrentQuestProgress == 0);
        quest.ProgressQuest(Tags[0]);
    }

    private void CheckReload(UQuest_Subsystem quest)
    {
        Check("reopened original map in the same GameInstance creates fresh managed quest state",
            !ReplaySameWorld() && ReplaySameInstance() && ReplayHasManagedQuest() &&
            ReplayQuestBridge() != initialBridge && quest.CurrentQuestTag == Tags[0] &&
            quest.CurrentQuestProgress == 0);
        int oldEvents = steps.Count;
        quest.ProgressQuest(Tags[0]);
        Check("old world delegates are removed even while the test listener is alive",
            !ReplayIsListening() && steps.Count == oldEvents && quest.CurrentQuestTag == Tags[1]);
    }

    private void Check(string name, bool passed)
    {
        checks.Add(new ReplayCheck(name, passed));
        ReplayQuestLog($"{(passed ? "PASS" : "FAIL")} {name}");
    }

    private void Finish()
    {
        phase = 3;
        bool passed = checks.Count == 25 && checks.All(check => check.passed);
        try
        {
            string path = ReplayQuestReportPath();
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path + ".tmp", JsonSerializer.Serialize(new
            {
                map = "/Game/Becorn/Maps/Map_Blocking_V2", runner = "C#", passed, checks
            }));
            File.Move(path + ".tmp", path, true);
        }
        catch (Exception failure)
        {
            ReplayQuestLog($"could not save the report: {failure}");
            passed = false;
        }
        ReplayQuestExit(passed ? 0 : 1);
    }

    private sealed record ReplayCheck(string name, bool passed);
}
