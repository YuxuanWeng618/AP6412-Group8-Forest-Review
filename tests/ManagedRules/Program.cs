using System.Numerics;
using ManagedBCR;

var passed = 0;
void Check(bool condition, string name)
{
    if (!condition) throw new InvalidOperationException(name);
    Console.WriteLine($"PASS {name}");
    passed++;
}

bool Near((double X, double Y) actual, double x, double y) =>
    Math.Abs(actual.X - x) < 1e-10 && Math.Abs(actual.Y - y) < 1e-10;

Check(Near(PlayerActionRules.ResolveMovement(true, 0, 0, 1), 1, 0), "zero yaw forward");
Check(Near(PlayerActionRules.ResolveMovement(true, 0, 1, 0), 0, 1), "zero yaw right");
Check(Near(PlayerActionRules.ResolveMovement(true, 90, 0, 1), 0, 1), "quarter-turn forward");
Check(Near(PlayerActionRules.ResolveMovement(true, -90, 1, 0), 1, 0), "negative yaw right");
Check(Near(PlayerActionRules.ResolveMovement(true, 90, -0.5, 0.25), 0.5, 0.25), "analog axes preserve strength and signs");
Check(Near(PlayerActionRules.ResolveMovement(false, 90, 1, 1), 0, 0), "workstation movement lock");
Check(Near(PlayerActionRules.ResolveMovement(true, double.NaN, 1, 1), 0, 0), "invalid camera sample ignored");
Check(Near(PlayerActionRules.ResolveMovement(true, 0, double.PositiveInfinity, 1), 0, 0), "invalid analog sample ignored");
Check(PlayerActionRules.ResolveCarry(true, true) == PlayerAction.Drop, "drop valid carried actor");
Check(PlayerActionRules.ResolveCarry(true, false) == PlayerAction.ClearInvalidCarry, "clear destroyed carried actor");
Check(PlayerActionRules.ResolveCarry(false, false) == PlayerAction.None, "empty carry slot");
Check(PlayerActionRules.ResolvePickup(true) == PlayerAction.SelectPickup, "select pickable actor");
Check(PlayerActionRules.ResolvePickup(false) == PlayerAction.None, "ignore non-pickable actor");
Check(PlayerActionRules.ResolveInteraction(true, false, true) == PlayerAction.None, "ignore non-interactable help candidate");
Check(PlayerActionRules.ResolveInteraction(true, true, true) == PlayerAction.InteractWithItem, "carried item takes priority");
Check(PlayerActionRules.ResolveInteraction(false, true, true) == PlayerAction.OpenHelp, "open help with empty hands");
Check(PlayerActionRules.ResolveInteraction(false, true, false) == PlayerAction.Interact, "ordinary interaction");

QteResult Qte(QteActionType type, bool pressed = false, bool released = false,
    bool held = false, float x = 0, float y = 0) =>
    QteActionRules.Evaluate(type, pressed, released, held, x, y, 0.5f);
Check(Qte(QteActionType.Press, pressed: true) == QteResult.Success, "blade press edge succeeds");
Check(Qte(QteActionType.Press, released: true) == QteResult.Failure, "blade release is the wrong edge");
Check(Qte(QteActionType.Press, held: true) == QteResult.None, "holding blade is not another press");
Check(Qte(QteActionType.Release, released: true) == QteResult.Success, "release action detects release edge");
Check(Qte(QteActionType.Release, pressed: true) == QteResult.Failure, "release action detects wrong press");
Check(Qte(QteActionType.Hold, held: true) == QteResult.Success, "hold action succeeds while held");
Check(Qte(QteActionType.Hold) == QteResult.None, "idle is not a failed hold");
Check(Qte(QteActionType.Rotate) == QteResult.None, "idle analog stick does not trigger an action");
Check(Qte(QteActionType.Rotate, x: .4f) == QteResult.Failure, "under-threshold analog movement fails");
Check(Qte(QteActionType.Rotate, y: -.5f) == QteResult.Success, "rotation threshold inclusive on either axis");
Check(Qte(QteActionType.Rotate, x: float.NaN) == QteResult.None, "invalid rotation sample is ignored");
Check(!QteActionRules.ShouldWait(false, 2, 0) && QteActionRules.ShouldWait(true, 2, 1)
    && !QteActionRules.ShouldWait(true, 2, 2), "synchronization only blocks leading station when configured");
Check(!QteActionRules.ReachedRepeatLimit(-1, 500) &&
    !QteActionRules.ReachedRepeatLimit(5, 4) && QteActionRules.ReachedRepeatLimit(5, 5),
    "infinite and finite repeat limits remain distinct");
Check(QteActionRules.Progress(QteActionType.Rotate, false, .1f, 0, .5f) == 0 &&
    QteActionRules.Progress(QteActionType.Rotate, false, 0, -.25f, .5f) == .5f &&
    QteActionRules.Progress(QteActionType.Rotate, false, 0, 1f, .5f) == 2f,
    "rotation progress preserves the original dead zone, ratio, and values above one");
Check(QteActionRules.Progress(QteActionType.Press, true, 0, 0, 0) == 1f &&
    QteActionRules.Progress(QteActionType.Hold, false, 0, 0, 0) == 0 &&
    QteActionRules.Progress(QteActionType.Release, true, 0, 0, 0) == 1f,
    "button progress follows held state while action edges still determine success");
Check(QteActionRules.Progress(QteActionType.Rotate, false, 1, 0, 0) == 0 &&
    QteActionRules.Progress(QteActionType.Rotate, false, 1, 0, float.NaN) == 0 &&
    QteActionRules.Progress(QteActionType.Rotate, false, float.PositiveInfinity, 0, 1) == 0,
    "invalid progress samples cannot divide by zero or publish non-finite input");

var keyboard = new KeyboardInputPolicy();
Check(KeyboardInputPolicy.TryGetBinding("RightShift", out var shift) && shift.Player == 1 &&
    !KeyboardInputPolicy.TryGetBinding("LeftShift", out _), "right Shift belongs exclusively to P2");
Check(keyboard.Press("D") && !keyboard.Press("D"), "repeat press does not produce new edge");
keyboard.Press("Left");
keyboard.Press("W");
Check(keyboard.SampleStick(1) == -Vector2.UnitX, "two controllers stay separate");
Check(MathF.Abs(keyboard.SampleStick(0).Length() - 1) < 0.00001f, "diagonal is normalized");
keyboard.Press("A");
Check(keyboard.SampleStick(0) == Vector2.UnitY, "opposing horizontal keys cancel");
keyboard.Press("E");
Check(keyboard.Reset() is [{ Player: 0, Label: "E" }] && !keyboard.HasHeldKeys, "reset releases only held buttons");
Check(keyboard.SampleStick(0) == Vector2.Zero && keyboard.SampleStick(1) == Vector2.Zero, "reset neutralizes both axes");

string[] tags = ["order", "grove", "harvest", "return", "load_saw", "saw", "planks", "ropes", "delivery"];
var rows = tags.Select((tag, i) => new QuestStep(tag, tag, tag == "planks" ? 3 : 1,
    i == tags.Length - 1 ? "" : tags[i + 1], i == 0 ? "" : tags[i - 1])).ToArray();
var quest = QuestProgress.FromData("complete", rows.Reverse()) ?? throw new InvalidOperationException("valid quest data");
Check(quest.Current?.Tag == "order", "configuration resolved by links");
quest.Submit("delivery", true, (_, _) => throw new InvalidOperationException("unexpected completion"));
Check(quest.Current?.Tag == "order" && quest.Count == 0, "out-of-order event does not mutate progress");
var events = new List<(string Tag, bool Complete)>();
for (int index = 0; index < tags.Length; index++)
{
    string tag = tags[index];
    if (tag == "planks")
    {
        quest.Submit(tag, true, (_, _) => throw new InvalidOperationException("first plank completed"));
        quest.Submit(tag, true, (_, _) => throw new InvalidOperationException("second plank completed"));
        Check(quest.Current?.Tag == tag && quest.Count == 2, "two planks leave the stage active");
    }
    quest.Submit(tag, true, (published, complete) =>
    {
        Check(quest.Current?.Tag == tag && quest.IsPublishing, $"callback sees completing step {tag}");
        events.Add((published, complete));
        quest.Submit(tag, true, (_, _) => throw new InvalidOperationException("reentrant publication"));
    });
    Check(!quest.IsPublishing && quest.Count == 0, $"callback clears state {tag}");
}
quest.Submit("delivery", true, (_, _) => throw new InvalidOperationException("duplicate delivery"));
Check(quest.Current is null && events.Count == 9 && events[^1] == ("complete", true), "one terminal publication");
Check(QuestProgress.FromData("complete", rows)?.Current?.Tag == "order", "fresh session starts at order");
Check(QuestProgress.FromData("complete", rows.Append(rows[0])) is null, "duplicate tags rejected");
Check(QuestProgress.FromData("complete", rows.Append(new QuestStep("extra", "", 1, "", ""))) is null, "multiple starts rejected");
Check(QuestProgress.FromData("complete", rows.Take(8)) is null, "missing next link rejected");
Check(QuestProgress.FromData("complete", rows.Select(row =>
    row.Tag == "saw" ? row with { PreviousTag = "wrong" } : row)) is null, "inconsistent backward link rejected");
Check(QuestProgress.FromData("complete", rows.Append(new QuestStep("orphan", "", 1, "", "delivery"))) is null, "unreachable row rejected");
var throwing = QuestProgress.FromData("complete", rows) ?? throw new InvalidOperationException("valid quest data");
try { throwing.Submit("order", true, (_, _) => throw new ApplicationException("listener failure")); }
catch (ApplicationException) { }
Check(!throwing.IsPublishing && throwing.Current?.Tag == "grove", "listener exception releases publication guard");
Check(PlayerActionRules.ResolveCarry(false, true) == PlayerAction.Drop,
    "the saw can release an actor grabbed by Blueprint without a native toggle flag");

// Workshop stations, machine input, recipe, ropes and outputs are checked against the shared C++ rules
// (forest-2d Domain) by the UE tests BCR.SharedRules.Workshop and Forest2D.Rules.SharedRecipeRopesOutput.

var round = new QteRound();
Check(!round.Start(-2, -2) && round.Start(2, 1) && !round.Start(1, 1), "round validates configuration and rejects concurrent start");
Check(round.Join(0, 1) && !round.Join(1, 1) && !round.Join(0, 2) && round.Join(1, 2),
    "round rejects duplicate player and duplicate station");
Check(round.SetPaused(true) && !round.RecordSuccess(1) && round.Successes(1) == 0 && round.SetPaused(false),
    "paused input cannot advance the round");
Check(!round.RecordSuccess(0) && !round.RecordSuccess(1) && round.RecordSuccess(0) &&
    round.Finish(true, false) && !round.Finish(false, false), "finite co-op round completes once");
Check(round.Start(-2, -1) && !round.Join(0, 1) && round.Join(1, 2) && round.State == QteRoundState.Running &&
    !round.RecordSuccess(1) && round.Successes(1) == 1 && round.Finish(false, true),
    "single-station infinite round starts and waits for explicit completion");
Check(!round.StartConfigured(0, 2, 0, 1, 2) && !round.StartConfigured(1, -2, -1, 0, 2) &&
    !round.StartConfigured(3, 1, -1, 0, 2) && round.StartConfigured(1, 2, 0, 1, 2),
    "managed configuration rejects invalid and duplicate slots and accepts reversed asset order");
Check(round.TimeoutSeconds == 2 && !round.FinishAuthored() && round.SetPaused(true) &&
    !round.Expire() && round.SetPaused(false) && round.Expire() && !round.Expire() &&
    round.State == QteRoundState.Failed && round.TimeoutSeconds == 0,
    "finite deadline can fail a waiting round once and cannot expire it while paused");
Check(round.StartConfigured(1, -1, -1, 0, -1) && round.TimeoutSeconds == 0 &&
    round.Join(1, 2) && !round.Expire() && round.FinishAuthored() &&
    round.State == QteRoundState.Completed && !round.FinishAuthored(),
    "unlimited authored rounds have no deadline and complete once from running");

Check(PlayerActionRules.SelectPickupCandidate([]) == -1 &&
    PlayerActionRules.SelectPickupCandidate([0, 0]) == -1, "empty and ineligible sweeps have no pickup");
Check(PlayerActionRules.SelectPickupCandidate([0, 1, 0]) == 1 &&
    PlayerActionRules.SelectPickupCandidate([1, 0, 1, 0]) == 2, "pickup keeps the last eligible UE hit");

var occupancy = new TriggerOccupancy();
Check(occupancy.Change(true, 1, 10, 0, true) == ZoneTransition.OneEntered &&
    occupancy.Change(true, 1, 10, 0, true) == ZoneTransition.None &&
    occupancy.Change(true, 1, 10, 1, true) == ZoneTransition.None,
    "multiple bodies and duplicate callbacks count as one player");
Check(occupancy.Change(true, 2, 20, 0, true) == ZoneTransition.TwoEntered &&
    occupancy.Change(false, 2, 20, 0, true) == ZoneTransition.OneEntered,
    "inner and outer trigger occupancy is independent");
Check(occupancy.Change(true, 1, 10, 0, false) == ZoneTransition.None &&
    occupancy.Change(true, 2, 20, 0, false) == ZoneTransition.None &&
    occupancy.Change(true, 1, 10, 1, false) == ZoneTransition.Emptied &&
    occupancy.Change(true, 1, 10, 1, false) == ZoneTransition.None,
    "only the final matching overlap emits the empty event");
Check(occupancy.Change(false, 2, 20, 0, false) == ZoneTransition.Emptied &&
    new TriggerOccupancy().Change(true, 1, 10, 1, false) == ZoneTransition.None,
    "outer exit and a new zone session do not share stale state");

var settings = new CameraSettings(4000, 16000, 200, 400, 4000, 20000, 10, 20, 1,
    1000, 100, 1500, .2, 2, 1);
var right = new CameraVector(0, 1, 0);
var forward = new CameraVector(1, 0, 0);
var first = new CameraVector(100, 200, 300);
var second = new CameraVector(300, 500, 700);
Check(SharedCameraRules.Center(first, second, true, 50) == new CameraVector(200, 350, 550) &&
    SharedCameraRules.Center(first, second, false, 50) == new CameraVector(200, 350, 50),
    "camera center preserves authored vertical offset modes");
var frame = SharedCameraRules.Frame(new(1000, 16000, 0), right, forward, 0, 90, 90, settings);
Check(Math.Abs(frame.ArmLength - 8200) < 0.001 && Math.Abs(frame.DepthLimit - 16000) < 0.001 &&
    Math.Abs(frame.HorizontalLimit - 32000) < 0.001 && Math.Abs(frame.Pitch - -12.625) < 0.001,
    "framing matches the legacy horizontal, depth and easing equations");
Check(Math.Abs(frame.FogDistance - 9700) < 0.001 && Math.Abs(frame.Aperture - (0.2 + 1.8 * 900 / 1400)) < 0.001,
    "fog and aperture follow the original depth projection");
Check(SharedCameraRules.Frame(new(0, 60000, 0), right, forward, 0, 90, 90, settings).ArmLength == 16000 &&
    SharedCameraRules.Frame(default, right, forward, 0, 90, 90, settings).ArmLength == 4000,
    "framing respects the authored minimum and maximum boom length");
Check(SharedCameraRules.Correction(new(20, 40, 0), new(3, 5, 7), right, forward, 30, 10) ==
    new CameraVector(-3, -5, 0), "camera constraint cancels only movement on the exceeded axes");

var overview = KeyboardPromptRules.Overview(false);
Check(overview.Length == 3 && overview[0].Contains("P1: WASD Move") &&
    overview[1].Contains("P2: ARROWS Move") && overview[2] == "H Help | TAB Quest | P Pause",
    "overview uses the two keyboard bindings and English help labels");
Check(KeyboardPromptRules.Overview(true).Last() == "Paused | P Continue",
    "overview adds the original pause row only while paused");
Check(KeyboardPromptRules.World("NS_UI_LeftStick_Rotating", false, -1, true, 1, 0, 10, 20)
    .StartsWith("P2 [ARROWS]") &&
    KeyboardPromptRules.World("NS_UI_A", false, -1, true, 1, 0, 10, 20).StartsWith("P1 [SPACE]"),
    "saw prompts follow swapped station occupants");
Check(KeyboardPromptRules.World("NS_UI_X", false, -1, true, 0, 1, 10, 10).StartsWith("P2 [K]") &&
    KeyboardPromptRules.World("NS_UI_A_Pressed", true, 0, false, -1, -1, 0, 0) == "P1 [SPACE]\nPull / 拉绳",
    "equal-distance station prompts and individual rope labels retain their original behavior");
Check(KeyboardPromptRules.Qte(0, "WASD", true) == "P1 [WASD] Waiting for partner / 等待队友站位" &&
    KeyboardPromptRules.Qte(1, "R SHIFT", false) == "P2 [R SHIFT] Cooperate now / 按提示配合",
    "QTE status distinguishes waiting from an active round");
Check(KeyboardPromptRules.StatusTop(384, 50, 414, 0.65) + 50 < 414 &&
    KeyboardPromptRules.StatusTop(720, 50, 820, 1) == 720,
    "QTE status clears the paper controls when the original position overlaps");
Check(KeyboardPromptRules.Place(500, 300, 1, 1600, 900, 1, [new(405, 270), new(450, 190)]) == new PromptPoint(400, 105),
    "world prompts move above already placed prompts in draw order");
Check(KeyboardPromptRules.Place(500, 480, 1, 880, 540, .65, []) is null &&
    KeyboardPromptRules.Place(500, 300, 1, 880, 540, .65, []) == new PromptPoint(435, 281.8),
    "narrow viewports protect the reserved lower panel");

Console.WriteLine($"{passed} checks passed.");
