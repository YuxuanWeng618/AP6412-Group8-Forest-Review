using System.Text.Json;
using UnrealSharp.Attributes;
using UnrealSharp.BCR;
using UnrealSharp.CoreUObject;

namespace ManagedBCR;

[UClass]
public partial class UManagedKeyboardReplay : UBCRKeyboardSmokeBridge
{
    private const string Bottom = "Gamepad_FaceButton_Bottom";
    private const string Pickup = "Gamepad_FaceButton_Right";
    private const string Interact = "Gamepad_FaceButton_Left";
    private const string LeftHand = "Gamepad_LeftTrigger";
    private const string RightHand = "Gamepad_RightTrigger";
    private const string Quest = "WBP_Quest_C";
    private const string Help = "WBP_Input_C";
    private const string Saw = "DA_QTE_SawMachine";
    private const int Collapsed = 1;
    private const int Waiting = 1;
    private const int Completed = 4;

    private readonly List<ReplayCheck> checks = [];
    private int phase;
    private double elapsed;
    private int savedQuestVisibility;
    private bool viewport;

    public override void RunKeyboardFrame(double deltaTime, int mode)
    {
        if (phase == 99) return;
        var p1 = TestController(0);
        var p2 = TestController(1);
        var firstPawn = TestPawn(0);
        var secondPawn = TestPawn(1);
        if (!p1.IsValid() || !p2.IsValid() || !firstPawn.IsValid() || !secondPawn.IsValid()) return;

        viewport = mode >= 2;
        elapsed += deltaTime;
        if (phase == 0)
        {
            if (elapsed < 6) return;
            TestScreenshot("forest-start-menu.png", viewport);
            TestIgnoreInput(false);
            TestGameInputMode(0);
            TestGameInputMode(1);
            TestSetPaused(false);
            Check("original map has two separate player controllers and pawns", p1 != p2 && firstPawn != secondPawn);
            if (mode == 3) CheckReflectedPlayerRules();
            Press("D");
            Next();
            return;
        }

        if (elapsed < 0.35) return;
        FVector2D first = TestStick(0);
        FVector2D second = TestStick(1);
        switch (phase)
        {
            case 1:
                Check("P1 D reaches only P1 raw left-stick X", first.X > 0.8 && Neutral(second.X));
                Press("W", "Left");
                Next(101);
                return;
            case 101:
                Check("C# normalizes P1 diagonal without changing P2 axis",
                    Math.Abs(TestRawAxis(0, "Gamepad_LeftX") - 0.7071) < 0.015 &&
                    Math.Abs(TestRawAxis(0, "Gamepad_LeftY") - 0.7071) < 0.015 &&
                    second.X < -0.8 && Neutral(second.Y));
                Press("A");
                Next(102);
                return;
            case 102:
                Check("C# cancels opposite P1 keys without changing P2 axis", Neutral(first.X) && first.Y > 0.8 && second.X < -0.8);
                Release("A", "W", "D");
                Next(2);
                return;
            case 2:
                Check("P2 left arrow reaches only P2 raw left-stick X", second.X < -0.8 && Neutral(first.X));
                Release("Left");
                Press("W", "Right");
                break;
            case 3:
                Check("both keyboard halves reach distinct QTE stick states simultaneously",
                    first.Y > 0.8 && second.X > 0.8 && Neutral(first.X) && Neutral(second.Y));
                Release("W", "Right");
                Press("E");
                break;
            case 4:
                Check("P1 E reaches original B pickup key without P2 leakage", TestDown(0, Pickup) && !TestDown(1, Pickup));
                Release("E");
                Press("J");
                break;
            case 5:
                Check("P2 J reaches original B pickup key and P1 releases", !TestDown(0, Pickup) && TestDown(1, Pickup));
                Release("J");
                Press("LeftShift");
                Next(105);
                return;
            case 105:
                Check("unbound left Shift does not operate either player's A button", Neither(Bottom));
                Release("LeftShift");
                Press("F", "K");
                Next(6);
                return;
            case 6:
                Check("F and K reach each player's original X interaction key", Both(Interact));
                Release("F", "K");
                Press("Q", "U");
                break;
            case 7:
                Check("Q and U reach both left-hand trigger states", Both(LeftHand));
                Release("Q", "U");
                Press("R", "O");
                break;
            case 8:
                Check("R and O reach right-hand triggers and release left hands", Both(RightHand) && Neither(LeftHand));
                Release("R", "O");
                Press("SpaceBar", "RightShift");
                break;
            case 9:
                Check("Space and right Shift reach both original A QTE keys", Both(Bottom));
                Press("W", "Left", "P");
                break;
            case 10:
                Check("P pauses original game world", TestPaused());
                Press("P");
                break;
            case 11:
                Check("P resumes and both players return to neutral",
                    !TestPaused() && Neutral(first.X) && Neutral(first.Y) && Neutral(second.X) && Neutral(second.Y) && Neither(Bottom));
                Release("SpaceBar", "RightShift", "W", "Left");
                TestStopRound();
                if (!StartRound(Saw)) return;
                TestJoinRound(0, 0);
                TestJoinRound(0, 1);
                Check("one player cannot occupy both cooperative QTE stations", TestRoundState() == Waiting);
                TestJoinRound(1, 1);
                Check("standard saw stations show P1 WASD and P2 right Shift",
                    TestRoundLabel(0) == "WASD" && TestRoundLabel(1) == "R SHIFT");
                Press("D", "RightShift");
                break;
            case 12:
                Check("standard saw stations accept D and right Shift", TestRoundState() == Completed);
                Release("D", "RightShift");
                if (!StartRound(Saw)) return;
                TestJoinRound(0, 1);
                Check("a new round clears the previous saw station assignments", TestRoundState() == Waiting);
                TestJoinRound(1, 0);
                Check("swapped saw stations show P1 Space and P2 arrows",
                    TestRoundLabel(0) == "SPACE" && TestRoundLabel(1) == "ARROWS");
                TestScreenshot("saw-swapped.png", viewport);
                Next(120);
                return;
            case 120:
                Press("Right", "SpaceBar");
                break;
            case 121:
                Check("real QTE accepts simultaneous routed arrow and Space input at swapped saw stations",
                    TestRoundState() == Completed);
                Release("Right", "SpaceBar");
                TestStopRound();
                if (!StartRound("DA_QTE_Tree")) return;
                TestJoinRound(0, 0);
                TestJoinRound(1, 1);
                Press("A", "Left");
                Next(13);
                return;
            case 13:
                Check("real tree QTE accepts both players independent direction inputs", TestRoundState() == Completed);
                Release("A", "Left");
                TestStopRound();
                Press("H");
                break;
            case 14:
                Check("H opens real keyboard help widget and pauses gameplay", TestHelpOpen() && TestPaused());
                TestScreenshot("help-keyboard.png", viewport);
                break;
            case 15:
                Press("H");
                break;
            case 16:
                Check("H closes help and restores the previous running state", !TestHelpOpen() && !TestPaused());
                break;
            case 17:
                int before = TestVisibility(Quest);
                if (before < 0 && elapsed < 35) return;
                bool toggled = false;
                if (before >= 0)
                {
                    Press("Tab");
                    int after = TestVisibility(Quest);
                    Press("Tab");
                    toggled = before != after && (TestVisibility(Quest) == Collapsed) == (before == Collapsed);
                }
                Check("Tab hides and restores the real quest HUD", toggled);
                Check("actual rope actor accepts the same A input routed from Space and right Shift", TestRopeKeys().Contains(Bottom));
                TestScreenshot("ue-keyboard-game.png", viewport);
                if (!viewport)
                {
                    Finish();
                    return;
                }
                break;
            case 18:
                savedQuestVisibility = TestVisibility(Quest);
                TestSetPaused(true);
                Press("H");
                break;
            case 19:
                CheckModalHelp();
                break;
            case 20:
                Check("modal help blocks pause, quest visibility and gameplay, and ignores H repeats",
                    TestVisibility(Quest) >= 0 && TestVisibility(Quest) == savedQuestVisibility &&
                    TestHelpOpen() && TestPaused() && Neither(Pickup));
                Release("E", "J");
                TestToggleHelp();
                break;
            case 21:
                Repeat("P");
                Check("help interaction entry closes the widget and restores the pre-existing pause; P repeat does not resume",
                    !TestHelpOpen() && TestPaused());
                Press("P");
                Repeat("Tab", "E", "J");
                break;
            case 22:
                Check("resume works; Tab repeats leave the real HUD and its bilingual shortcut unchanged",
                    !TestPaused() && TestVisibility(Quest) >= 0 && TestVisibility(Quest) == savedQuestVisibility &&
                    TestWidgetText(Quest, "KeyboardQuestShortcut") == "TAB\n任务");
                Check("mapped repeat events alone do not generate pickup presses for either player", Neither(Pickup));
                Press("D", "Left", "E", "J");
                break;
            case 23:
                Check("both controllers hold independent directions and buttons immediately before focus loss",
                    first.X > 0.8 && second.X < -0.8 && Both(Pickup));
                TestLostFocus();
                break;
            case 24:
                Check("focus loss pauses the original map and releases both players' axes and buttons",
                    TestPaused() && Neutral(first.X) && Neutral(second.X) && Neither(Pickup));
                Press("P");
                break;
            case 25:
                Check("resume after focus loss keeps both players neutral without physical key-up events",
                    !TestPaused() && Neutral(first.X) && Neutral(second.X) && Neither(Pickup));
                TestIgnoreInput(true);
                Press("P");
                TestLostFocus();
                Check("P and focus loss do not pause while the engine ignores input", !TestPaused());
                TestIgnoreInput(false);
                TestPossess(null!);
                bool hadNoPawn = !TestPawn(0).IsValid();
                Press("P");
                TestLostFocus();
                Check("P and focus loss do not pause before player one has a pawn", hadNoPawn && !TestPaused());
                TestPossess(firstPawn);
                Check("the original player pawn can be restored after the readiness check", TestPawn(0) == firstPawn);
                Finish();
                return;
        }

        Next();
    }

    private void CheckReflectedPlayerRules()
    {
        var rules = TestPlayerRules();
        Check("managed player rule class is registered in the original map", rules.IsValid());
        if (!rules.IsValid()) return;
        FVector direction = rules.ResolveMovement(true, 90, -0.5, 0.25);
        FVector stopped = rules.ResolveMovement(false, 90, 1, 1);
        Check("player movement round trips through the reflected C# rule",
            Math.Abs(direction.X - 0.5) < 0.0001 && Math.Abs(direction.Y - 0.25) < 0.0001 &&
            Math.Abs(direction.Z) < 0.0001 && Math.Abs(stopped.X) < 0.0001 &&
            Math.Abs(stopped.Y) < 0.0001 && Math.Abs(stopped.Z) < 0.0001);
        Check("player carry and pickup resolve through the reflected C# rule",
            rules.ResolveCarry(true, false) == (int)PlayerAction.ClearInvalidCarry &&
            rules.ResolvePickup(true) == (int)PlayerAction.SelectPickup);
        Check("player interaction resolves item priority and help through C#",
            rules.ResolveInteraction(true, true, true) == (int)PlayerAction.InteractWithItem &&
            rules.ResolveInteraction(false, true, true) == (int)PlayerAction.OpenHelp);
    }

    private void CheckModalHelp()
    {
        string guide = TestWidgetText(Help, "BilingualControllerGuide");
        string back = TestWidgetText(Help, "TextBlock_194");
        Check("help opens while externally paused and shows the original bilingual guide and return prompt",
            TestHelpOpen() && TestPaused() && guide.Contains("P1 WASD") && guide.Contains("P2 ARROWS") &&
            guide.Contains("双人共用键盘") && guide.Contains("锯木机") && back.Contains("Press H") && back.Contains("按 H 关闭键盘帮助"));
        var policy = TestKeyboardPolicy();
        Check("original viewport uses the reflected C# policy and yields editor shortcut keys to Unreal",
            policy is UManagedKeyboardInput && new[] { "F1", "LeftShift", "Escape" }.All(
                key => !policy.RouteViewportKey(key, true, false, false, true, true, true, true)));
        bool pause = TestKey("P", 0) & TestKey("P", 1);
        bool tab = TestKey("Tab", 0) & TestKey("Tab", 1);
        Repeat("H");
        bool both = TestKey("E", 0) & TestKey("J", 0);
        Check("open help consumes P, Tab and both mapped keyboard regions", pause && tab && both);
    }

    private bool StartRound(string asset)
    {
        if (TestStartRound(asset, 1)) return true;
        Check($"load {asset}", false);
        Finish();
        return false;
    }

    private static bool Neutral(double axis) => Math.Abs(axis) < 0.01;
    private bool Both(string key) => TestDown(0, key) && TestDown(1, key);
    private bool Neither(string key) => !TestDown(0, key) && !TestDown(1, key);
    private void Press(params string[] keys) { foreach (string key in keys) TestKey(key, 0); }
    private void Release(params string[] keys) { foreach (string key in keys) TestKey(key, 1); }
    private void Repeat(params string[] keys) { foreach (string key in keys) TestKey(key, 2); }

    private void Next(int? next = null)
    {
        phase = next ?? phase + 1;
        elapsed = 0;
    }

    private void Check(string name, bool passed)
    {
        checks.Add(new ReplayCheck(name, passed));
        TestKeyboardLog($"{(passed ? "PASS" : "FAIL")}: {name}");
    }

    private void Finish()
    {
        phase = 99;
        bool passed = checks.All(check => check.passed);
        try
        {
            string path = TestReportPath(viewport);
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path + ".tmp", JsonSerializer.Serialize(new
            {
                status = passed ? "passed" : "failed",
                scope = "Original Unreal map, synthetic events through custom viewport to real PlayerControllers; not physical keyboard rollover or full production-loop validation",
                runner = "C#",
                checks
            }));
            File.Move(path + ".tmp", path, true);
        }
        catch (Exception failure)
        {
            TestKeyboardLog($"could not save the report: {failure}");
            passed = false;
        }
        TestKeyboardExit(passed ? 0 : 1);
    }

    private sealed record ReplayCheck(string name, bool passed);
}
