using System.Numerics;

namespace ManagedBCR;

internal sealed class KeyboardInputPolicy
{
    internal readonly record struct Binding(int Player, string Control, string Label, int X = 0, int Y = 0)
    {
        public bool IsDirection => X != 0 || Y != 0;
    }

    private static readonly Dictionary<string, Binding> Bindings = new(StringComparer.Ordinal)
    {
        ["W"] = new(0, "Gamepad_LeftY", "WASD", Y: 1),
        ["S"] = new(0, "Gamepad_LeftY", "WASD", Y: -1),
        ["A"] = new(0, "Gamepad_LeftX", "WASD", X: -1),
        ["D"] = new(0, "Gamepad_LeftX", "WASD", X: 1),
        ["E"] = new(0, "Gamepad_FaceButton_Right", "E"),
        ["F"] = new(0, "Gamepad_FaceButton_Left", "F"),
        ["SpaceBar"] = new(0, "Gamepad_FaceButton_Bottom", "SPACE"),
        ["Q"] = new(0, "Gamepad_LeftTrigger", "Q"),
        ["R"] = new(0, "Gamepad_RightTrigger", "R"),
        ["Up"] = new(1, "Gamepad_LeftY", "ARROWS", Y: 1),
        ["Down"] = new(1, "Gamepad_LeftY", "ARROWS", Y: -1),
        ["Left"] = new(1, "Gamepad_LeftX", "ARROWS", X: -1),
        ["Right"] = new(1, "Gamepad_LeftX", "ARROWS", X: 1),
        ["J"] = new(1, "Gamepad_FaceButton_Right", "J"),
        ["K"] = new(1, "Gamepad_FaceButton_Left", "K"),
        ["RightShift"] = new(1, "Gamepad_FaceButton_Bottom", "R SHIFT"),
        ["U"] = new(1, "Gamepad_LeftTrigger", "U"),
        ["O"] = new(1, "Gamepad_RightTrigger", "O"),
    };

    private readonly HashSet<string> held = new(StringComparer.Ordinal);

    public bool HasHeldKeys => held.Count != 0;

    public static bool TryGetBinding(string key, out Binding binding) => Bindings.TryGetValue(key, out binding);

    public bool Press(string key) => held.Add(key);

    public void Release(string key) => held.Remove(key);

    public Binding[] Reset()
    {
        var releases = new List<Binding>();
        foreach (string key in held)
        {
            Binding binding = Bindings[key];
            if (!binding.IsDirection)
                releases.Add(binding);
        }
        held.Clear();
        return releases.ToArray();
    }

    public Vector2 SampleStick(int player)
    {
        Vector2 stick = Vector2.Zero;
        foreach (string key in held)
        {
            Binding binding = Bindings[key];
            if (binding.Player == player)
                stick += new Vector2(binding.X, binding.Y);
        }
        return stick.LengthSquared() > 1 ? Vector2.Normalize(stick) : stick;
    }

    public static string ActionLabel(int player, string control)
    {
        if (player is not (0 or 1))
            return "?";
        if (control is "Gamepad_LeftShoulder" or "Gamepad_RightShoulder")
            return "TAB";
        if (control == "Gamepad_Left2D")
            control = "Gamepad_LeftX";
        foreach (Binding binding in Bindings.Values)
        {
            if (binding.Player == player && binding.Control == control)
                return binding.Label;
        }
        return string.Empty;
    }

    public static string HorizontalLabel(int player)
    {
        string left = "", right = "";
        foreach (var (key, binding) in Bindings)
        {
            if (binding.Player != player)
                continue;
            if (binding.X < 0)
                left = key;
            else if (binding.X > 0)
                right = key;
        }
        return $"{left.ToUpperInvariant()} / {right.ToUpperInvariant()}";
    }
}
