using System.Text.Json;
using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.CoreUObject;
using UnrealSharp.Engine;

namespace ManagedBCR;

[UClass]
public partial class UManagedWorkshopReplay : UBCRWorkshopReplayBridge
{
    private enum Step
    {
        Start, Order, FindMushroom, EnterTree, CutMushroom, LiftLog, ReturnLog, LoadSaw,
        EnterSaw, CutLog, Assemble, Ropes, Deliver, FiniteTimer, UnlimitedTimer, Finished
    }

    private static readonly string[] Wheel = ["D", "S", "A", "W"];
    private static readonly string[] SawKeys = ["W", "A", "S", "D", "RightShift"];
    private static readonly string[] Seams = ["Box1", "Box2", "Box3"];
    private static readonly FVector PartnerOffset = new(95, 0, 0);
    private readonly List<ReplayCheck> checks = new();
    private readonly HashSet<string> cutSeams = new();
    private AActor first = null!;
    private AActor second = null!;
    private AActor? lastItem;
    private Step step;
    private double since = double.NaN;
    private double now;
    private double nextAction;
    private double nextBlade;
    private double cutUntil;
    private int input;
    private int directGrabs;
    private int dropAlignments;
    private bool bladePressed;
    private bool sawExited;
    private bool startPressed;

    private sealed record ReplayCheck(string name, bool passed);
    private double Elapsed => now - since;
    private QteRoundState Round => (QteRoundState)ReplayRoundState();

    public override void RunReplayFrame(double time)
    {
        if (step == Step.Finished) return;
        now = time;
        if (double.IsNaN(since)) since = now;
        try
        {
            double timeout = step switch
            {
                Step.Start => 45,
                Step.CutMushroom or Step.CutLog => 85,
                Step.Assemble => 60,
                _ => 25
            };
            if (Elapsed > timeout)
            {
                ReplayLog($"stopped at {step}, quest={ReplayQuestTag()}, round={Round}");
                Finish(false);
                return;
            }

            first = ReplayPlayer(0);
            second = ReplayPlayer(1);
            if (!first.IsValid() || !second.IsValid()) return;
            RunStep(ReplayQuestTag());
        }
        catch (Exception failure)
        {
            ReplayLog($"exception at {step}: {failure}");
            Finish(false);
        }
    }

    private void RunStep(string tag)
    {
        switch (step)
        {
            case Step.Start: Start(tag); break;
            case Step.Order: EnterZone("BP_GrandFather_C", "", tag, "find_the_mushroom", "original NPC zone accepts the order"); break;
            case Step.FindMushroom: EnterZone("Quest_TriggerZone_C", "find_the_mushroom", tag, "cut_the_mushroom", "original forest trigger finds the mushroom"); break;
            case Step.EnterTree: EnterStations("BP_Tree_C", "distinct players start the original mushroom round"); break;
            case Step.CutMushroom: CutMushroom(tag); break;
            case Step.LiftLog: LiftLog(); break;
            case Step.ReturnLog: ReturnLog(tag); break;
            case Step.LoadSaw: LoadSaw(tag); break;
            case Step.EnterSaw: EnterStations("BP_SawMachine_C", "distinct players start the original saw round"); break;
            case Step.CutLog: CutLog(tag); break;
            case Step.Assemble: Assemble(tag); break;
            case Step.Ropes: PullRopes(tag); break;
            case Step.Deliver: Deliver(tag); break;
            case Step.FiniteTimer: FiniteTimer(); break;
            case Step.UnlimitedTimer: UnlimitedTimer(); break;
        }
    }

    private void Start(string tag)
    {
        if (Elapsed < 7) return;
        if (tag.Length == 0)
        {
            if (now < nextAction) return;
            startPressed = !startPressed;
            ReplayKey("SpaceBar", startPressed);
            nextAction = now + (startPressed ? 0.3 : 0.7);
            return;
        }

        ReplayKey("SpaceBar", false);
        bool ready = tag == "the_order" && first != second;
        Check("original start screen creates two local players and loads the order", ready);
        if (ready) Next();
        else Finish(false);
    }

    private void EnterZone(string type, string zoneTag, string tag, string expected, string check)
    {
        if (input++ == 0) GoTo(first, type, zoneTag);
        AdvanceIf(tag == expected, check);
    }

    private void EnterStations(string type, string check)
    {
        if (input++ == 0 && Find(type) is { } machine)
        {
            ReplayPlace(first, ReplayStationPosition(machine, 0));
            ReplayPlace(second, ReplayStationPosition(machine, 1));
            ReplayInteract(machine, first, null!);
            ReplayInteract(machine, second, null!);
        }
        AdvanceIf(Round == QteRoundState.Running, check);
    }

    private void CutMushroom(string tag)
    {
        if (tag == "bring_back_the_log")
        {
            Release(["A", "D", "Left", "Right"]);
            AdvanceIf(true, "two routed inputs complete the mushroom QTE");
            return;
        }
        var tree = Find("BP_Tree_C");
        double target = ReplayNumber(tree!, "nextExtremity");
        if (double.IsNaN(target)) return;
        int direction = target > 0 ? 1 : -1;
        if (direction == input) return;
        bool right = direction > 0;
        ReplayKey(right ? "A" : "D", false);
        ReplayKey(right ? "Left" : "Right", false);
        ReplayKey(right ? "D" : "A", true);
        ReplayKey(right ? "Right" : "Left", true);
        input = direction;
    }

    private void LiftLog()
    {
        if (Find("BP_LogTEST_C") is not { } log) return;
        if (Elapsed > 0.75 && input == 0)
        {
            Pick(first, log, 1);
            Pick(second, log, -1);
            input++;
        }
        if (Elapsed > 2.5 && input == 1)
        {
            AssistGrab(first, log, 1);
            AssistGrab(second, log, -1);
            input++;
        }
        AdvanceIf(Elapsed > 1.5 && BothHold(log), "both players lift the log produced by the original mushroom round");
    }

    private void ReturnLog(string tag)
    {
        if (input++ == 0)
        {
            GoTo(first, "Quest_TriggerZone_C", "bring_back_the_log");
            ReplayPlace(second, Add(ReplayPosition(first), PartnerOffset));
        }
        AdvanceIf(tag == "put_the_log_in_the_machine", "original return trigger accepts the carried log");
    }

    private void LoadSaw(string tag)
    {
        var saw = Find("BP_SawMachine_C");
        if (input++ == 0 && saw is not null && ReplayHeld(first) is { } log)
        {
            ReplayPlace(first, ReplayMachineInput(saw));
            ReplayPlace(second, Add(ReplayPosition(first), PartnerOffset));
            ReplayInteract(saw, first, log);
        }
        if (tag != "cut_the_log") return;
        Grab(first, false);
        Grab(second, false);
        Check("original saw accepts the carried log", true);
        Check("the saw Blueprint registers its spawned replacement in the C# machine recipe",
            saw is not null && ReplayHasMachineItem(saw));
        Next();
    }

    private void CutLog(string tag)
    {
        if (tag == "put_the_log_in_the_assembler")
        {
            Release(SawKeys);
            AdvanceIf(true, "player one turns the saw and player two lowers its blade");
            nextAction = now + 1;
            return;
        }
        var saw = Find("BP_SawMachine_C");
        if (Round == QteRoundState.Completed && saw is not null)
        {
            if (sawExited) return;
            sawExited = true;
            Release(SawKeys);
            ReplayInteract(saw, first, null!);
            ReplayInteract(saw, second, null!);
            return;
        }
        var log = Find("BP_Log_C");
        var blade = ReplayPart(saw!, "BC_Blade");
        double alpha = ReplayNumber(saw!, "BladeHeightAlpha");
        if (now >= cutUntil && alpha > 0.8 && blade.IsValid())
        {
            foreach (string name in Seams)
            {
                var box = ReplayPart(log!, name);
                if (!box.IsValid() || cutSeams.Contains(name) ||
                    HorizontalDistanceSquared(ReplayPosition(box), ReplayPosition(blade)) > 144)
                    continue;
                cutSeams.Add(name);
                cutUntil = now + 1.4;
                Release(Wheel);
                bladePressed = false;
                ReplayKey("RightShift", false);
                ReplayLog($"released blade at {name}");
                break;
            }
        }
        if (now < cutUntil) return;
        if (alpha > 0.8 && now >= nextAction)
        {
            if (input > 0) ReplayKey(Wheel[(input - 1) % Wheel.Length], false);
            ReplayKey(Wheel[input++ % Wheel.Length], true);
            nextAction = now + 0.06;
        }
        if (now < nextBlade) return;
        bladePressed = !bladePressed;
        ReplayKey("RightShift", bladePressed);
        nextBlade = now + 0.06;
    }

    private void Assemble(string tag)
    {
        var assembler = Find("BP_FurnitureAssembler_C");
        if (assembler is null || ReplayPart(assembler, "Collision Zone") is not { } zone) return;
        int remaining = ReplayRecipeRemaining(assembler);
        if (remaining < 0)
        {
            ReplayLog("recipe output is missing from the loaded map");
            Finish(false);
            return;
        }
        if (tag == "activate_the_assembler")
        {
            Check("the three original planks are consumed and complete the managed recipe", remaining == 0);
            Next();
            return;
        }
        if (now < nextAction) return;
        var carrier = remaining % 2 != 0 ? first : second;
        FVector position = ReplayPosition(zone);
        switch (input)
        {
            case 0:
                if (Find("BP_NewPlank_C") is not { } plank) return;
                ReplayPlace(carrier == first ? second : first, Add(position, new(600, 600, 40)));
                Pick(carrier, plank, 1);
                var hit = ReplayTraceGrab(carrier);
                lastItem = hit.IsValid() && ReplayClass(hit) == ReplayClass(plank) ? hit : plank;
                break;
            case 1:
                if (ReplayHeld(carrier).IsValid()) break;
                AssistGrab(carrier, lastItem, 1);
                if (ReplayHeld(carrier).IsValid()) break;
                Grab(carrier, false);
                lastItem = null;
                input = 0;
                nextAction = now + 0.25;
                return;
            case 2: ReplayPlace(carrier, position); break;
            case 3:
                if (ReplayPart(ReplayHeld(carrier), "StaticMesh") is { } mesh)
                {
                    FVector from = ReplayPosition(mesh);
                    ReplayPlace(carrier, Add(ReplayPosition(carrier), new(position.X - from.X, position.Y - from.Y, 0)));
                }
                break;
            case 4: Drop(carrier); break;
            case 5:
                if (ReplayAlignLooseItem(assembler, lastItem!, position)) dropAlignments++;
                break;
            default: input = 0; return;
        }
        input++;
        nextAction = now + (input is 2 or 5 ? 1 : input == 6 ? 2.5 : 0.3);
    }

    private void PullRopes(string tag)
    {
        var ropes = Find("BP_NewActivationRopes_C");
        if (ropes is null || ReplayPart(ropes, "CollisionBox_Left") is not { } left ||
            ReplayPart(ropes, "CollisionBox_Right") is not { } right) return;
        if (tag == "deliver_the_product" && Find("BP_NewPaquet_C") is not null)
        {
            Release(["SpaceBar", "RightShift"]);
            AdvanceIf(true, "two original rope pulls produce the package");
            return;
        }
        if (now < nextAction) return;
        switch (input++)
        {
            case 0:
                ReplayPlace(first, ReplayPosition(left));
                ReplayPlace(second, ReplayPosition(right));
                break;
            case 1: ReplayInteract(ropes, first, null!); break;
            case 2: ReplayInteract(ropes, second, null!); break;
            default:
                bool pressed = input % 2 == 0;
                ReplayKey("SpaceBar", pressed);
                ReplayKey("RightShift", pressed);
                break;
        }
        nextAction = now + 0.65;
    }

    private void Deliver(string tag)
    {
        if (tag.Length == 0 && Find("BP_Rocking_Chair_C") is not null)
        {
            AdvanceIf(true, "delivery creates the original chair and completes the order");
            return;
        }
        if (now < nextAction || Find("BP_DeliveryPoint_C") is not { } delivery ||
            Find("BP_NewPaquet_C") is not { } package) return;
        if (input == 0)
        {
            Pick(first, package, 1);
            Pick(second, package, -1);
        }
        else if (input == 1)
        {
            AssistGrab(first, package, 1);
            AssistGrab(second, package, -1);
            if (!BothHold(package))
            {
                Grab(first, false);
                Grab(second, false);
                input = 0;
                nextAction = now + 0.4;
                return;
            }
            ReplayPlace(first, ReplayDeliveryPosition(delivery));
            ReplayPlace(second, Add(ReplayPosition(first), PartnerOffset));
        }
        else if (input == 2)
        {
            Drop(first);
            Drop(second);
        }
        input++;
        nextAction = now + 0.4;
    }

    private void FiniteTimer()
    {
        if (input == 0)
        {
            ReplayStartTimer(0.4f);
            if (Round != QteRoundState.Waiting)
            {
                Check("finite QTE probe starts in the original world", false);
                Finish(false);
                return;
            }
            ReplayPauseTimer(true);
            nextAction = now + 0.8;
            input++;
        }
        else if (input == 1 && now >= nextAction)
        {
            Check("UE timer does not expire the paused managed QTE", Round == QteRoundState.Paused);
            ReplayPauseTimer(false);
            input++;
        }
        else if (input == 2)
            AdvanceIf(Round == QteRoundState.Failed, "resuming the timer expires the finite managed QTE");
    }

    private void UnlimitedTimer()
    {
        if (input == 0)
        {
            ReplayStartTimer(-1);
            nextAction = now + 0.8;
            input++;
        }
        else if (now >= nextAction)
        {
            bool waiting = Round == QteRoundState.Waiting;
            ReplayStopTimer();
            bool passed = waiting && Round == QteRoundState.Inactive;
            Check("unlimited managed QTE waits without a timer and can be cancelled", passed);
            Finish(passed);
        }
    }

    private AActor? Find(string type, string questTag = "") => ReplayFind(type, questTag);

    private void GoTo(AActor player, string type, string questTag)
    {
        if (Find(type, questTag) is { } target) ReplayPlace(player, ReplayTriggerPosition(target));
    }

    private void Grab(AActor player, bool held)
    {
        ReplayKey(player == first ? "Q" : "U", held);
        ReplayKey(player == first ? "R" : "O", held);
    }

    private void Pick(AActor player, AActor item, double side)
    {
        ReplayPlaceForGrab(player, item, side);
        Grab(player, true);
    }

    private void AssistGrab(AActor player, AActor? item, double side)
    {
        if (item is null || !item.IsValid() || ReplayHeld(player) == item) return;
        if (ReplayTraceGrab(player) != item) Pick(player, item, side);
        if (ReplayTraceGrab(player) != item || !ReplayFlag(player, "CanPickUp")) return;
        ReplaySetHeld(player, item);
        directGrabs++;
    }

    private void Drop(AActor player)
    {
        Grab(player, false);
        if (directGrabs > 0 && ReplayHeld(player).IsValid() && !ReplayFlag(player, "BothTriggerPressed"))
            ReplaySetHeld(player, null!);
    }

    private bool BothHold(AActor item) => ReplayHeld(first) == item && ReplayHeld(second) == item;

    private void Release(IEnumerable<string> keys)
    {
        foreach (string key in keys) ReplayKey(key, false);
    }

    private void AdvanceIf(bool condition, string name)
    {
        if (!condition) return;
        Check(name, true);
        Next();
    }

    private void Next()
    {
        step++;
        since = nextAction = nextBlade = now;
        input = 0;
        ReplayLog($"phase {step}, quest={ReplayQuestTag()}");
    }

    private void Check(string name, bool passed)
    {
        checks.Add(new ReplayCheck(name, passed));
        ReplayLog($"{(passed ? "PASS" : "FAIL")} {name}");
    }

    private void Finish(bool passed)
    {
        if (!passed) Check("original map replay completed all production steps", false);
        step = Step.Finished;
        try
        {
            string path = ReplayReportPath();
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            var report = new
            {
                map = "Map_Blocking_V2",
                scope = "position-assisted replay through original actors and synthetic player input",
                runner = "C#",
                directGrabAssists = directGrabs,
                dropAlignmentAssists = dropAlignments,
                checks
            };
            File.WriteAllText(path + ".tmp", JsonSerializer.Serialize(report));
            File.Move(path + ".tmp", path, true);
        }
        catch (Exception failure)
        {
            ReplayLog($"could not save the report: {failure}");
            passed = false;
        }
        ReplayExit(passed && checks.All(check => check.passed) ? 0 : 1);
    }

    private static FVector Add(FVector a, FVector b) => new(a.X + b.X, a.Y + b.Y, a.Z + b.Z);

    private static double HorizontalDistanceSquared(FVector a, FVector b)
    {
        double x = a.X - b.X, y = a.Y - b.Y;
        return x * x + y * y;
    }
}
