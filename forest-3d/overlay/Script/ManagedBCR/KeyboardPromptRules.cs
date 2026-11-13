namespace ManagedBCR;

internal readonly record struct PromptPoint(double X, double Y);

internal static class KeyboardPromptRules
{
    private const string Rotate = "Gamepad_Left2D";
    private const string Interact = "Gamepad_FaceButton_Left";
    private const string Press = "Gamepad_FaceButton_Bottom";
    private const string LeftHand = "Gamepad_LeftTrigger";
    private const string RightHand = "Gamepad_RightTrigger";

    public static string[] Overview(bool paused)
    {
        var rows = new List<string>(4);
        for (int player = 0; player < 2; player++)
        {
            rows.Add($"P{player + 1}: {Label(player, Rotate)} Move | {Label(player, "Gamepad_FaceButton_Right")} Pick up / Drop | " +
                $"{Label(player, Interact)} Interact | {Label(player, Press)} Jump | " +
                $"{Label(player, LeftHand)} / {Label(player, RightHand)} Hands");
        }

        rows.Add("H Help | TAB Quest | P Pause");
        if (paused) rows.Add("Paused | P Continue");
        return rows.ToArray();
    }

    public static string Qte(int player, string label, bool waiting) =>
        $"P{player + 1} [{label}]" + (waiting ? " Waiting for partner / 等待队友站位" : " Cooperate now / 按提示配合");

    public static double StatusTop(double preferred, double height, double overviewTop, double scale) =>
        Math.Max(10, Math.Min(preferred, overviewTop - height - 8 * scale));

    public static string World(string asset, bool isRope, int ropePlayer, bool isMachine,
        int firstPlayer, int secondPlayer, double firstDistance, double secondDistance)
    {
        if (asset == "NS_UI_LeftStick_Alternate")
            return $"P1 [{KeyboardInputPolicy.HorizontalLabel(0)}] / P2 [{KeyboardInputPolicy.HorizontalLabel(1)}]\nAlternate / 左右交替";

        (string key, string action) = asset switch
        {
            "NS_UI_A" or "NS_UI_A_Pressed" => (Press, isRope ? "Pull / 拉绳" : "Press repeatedly / 反复按下"),
            "NS_UI_LT" => (LeftHand, "Left hand / 左手"),
            "NS_UI_RT" => (RightHand, "Right hand / 右手"),
            "NS_UI_LeftStick_Rotating" => (Rotate, "Follow rotation / 按方向转动"),
            _ => (Interact, "Interact / 交互")
        };

        int player = isRope && key == Press ? ropePlayer : -1;
        if (isMachine)
        {
            switch (key)
            {
                case Press:
                    player = secondPlayer;
                    break;
                case Rotate:
                    player = firstPlayer;
                    break;
                case Interact:
                    player = firstDistance < secondDistance ? firstPlayer : secondPlayer;
                    action = "Join / leave; load item\n站位 / 退出；放入材料";
                    break;
            }
        }

        string label = player is 0 or 1
            ? $"P{player + 1} [{Label(player, key)}]"
            : $"P1 [{Label(0, key)}] / P2 [{Label(1, key)}]";
        return label + "\n" + action;
    }

    public static PromptPoint? Place(double x, double y, double z, double width, double height, double scale,
        IEnumerable<PromptPoint> placed)
    {
        if (z <= 0 || x < 0 || x > width || y < 0 || y > height - 180 * scale) return null;

        x -= 100 * scale;
        y -= 28 * scale;
        foreach (PromptPoint point in placed)
        {
            if (Math.Abs(x - point.X) < 220 * scale && Math.Abs(y - point.Y) < 75 * scale)
                y = point.Y - 85 * scale;
        }

        return new PromptPoint(x, y);
    }

    private static string Label(int player, string key) => KeyboardInputPolicy.ActionLabel(player, key);
}
