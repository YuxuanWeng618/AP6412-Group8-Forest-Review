namespace ManagedBCR;

internal readonly record struct CameraVector(double X, double Y, double Z)
{
    public static CameraVector operator +(CameraVector a, CameraVector b) => new(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
    public static CameraVector operator -(CameraVector a, CameraVector b) => new(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
    public static CameraVector operator *(CameraVector v, double scale) => new(v.X * scale, v.Y * scale, v.Z * scale);
    internal double Dot(CameraVector other) => X * other.X + Y * other.Y + Z * other.Z;
    internal CameraVector Cross(CameraVector other) =>
        new(Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X);

    internal double ProjectHorizontally(CameraVector axis)
    {
        double lengthSquared = axis.X * axis.X + axis.Y * axis.Y;
        return lengthSquared > 1e-8 ? Math.Abs((X * axis.X + Y * axis.Y) / Math.Sqrt(lengthSquared)) : 0;
    }
}

internal readonly record struct CameraSettings(
    double MinArm, double MaxArm, double HorizontalBuffer, double DepthBuffer,
    double MinAngleArm, double MaxAngleArm, double MinAngle, double MaxAngle, double Easing,
    double FogOffset, double BlurNear, double BlurFar, double MinFStop, double MaxFStop, double BlurMultiplier);

internal readonly record struct CameraFraming(
    double ArmLength, double HorizontalLimit, double DepthLimit, double HorizontalDistance,
    double DepthDistance, double Pitch, double FogDistance, double Aperture);

internal static class SharedCameraRules
{
    private const double Radians = Math.PI / 180;

    internal static CameraVector Center(CameraVector first, CameraVector second, bool vertical, double offset)
    {
        CameraVector center = (first + second) * 0.5;
        return center with { Z = vertical ? center.Z + offset : offset };
    }

    internal static CameraFraming Frame(CameraVector separation, CameraVector right, CameraVector forward,
        double pitch, double horizontalFov, double verticalFov, CameraSettings settings)
    {
        var (sin, cos) = Math.SinCos(pitch * Radians);
        CameraVector backward = forward * -1;
        CameraVector backAxis = backward * cos + right.Cross(backward) * sin +
            right * (right.Dot(backward) * (1 - cos));
        double horizontal = separation.ProjectHorizontally(right);
        double depth = separation.ProjectHorizontally(backAxis);
        double horizontalTan = HalfFovTangent(horizontalFov);
        double verticalTan = HalfFovTangent(verticalFov);
        double depthFactor = cos - sin / verticalTan;
        double horizontalArm = (horizontal * 0.5 + settings.HorizontalBuffer) / horizontalTan;
        double depthArm = (depth * 0.5 + settings.DepthBuffer) * depthFactor;
        double arm = Math.Min(Math.Max(Math.Max(horizontalArm, depthArm), settings.MinArm), settings.MaxArm);

        double angleAlpha = Math.Clamp(RangeFraction(arm, settings.MinAngleArm, settings.MaxAngleArm), 0, 1);
        double easing = Math.Max(0, settings.Easing);
        double eased = angleAlpha < 0.5 ? 0.5 * Math.Pow(2 * angleAlpha, easing)
            : 1 - 0.5 * Math.Pow(2 * (1 - angleAlpha), easing);
        double blurAlpha = RangeFraction(depth, settings.BlurNear, settings.BlurFar);
        double blurScale = Math.Abs(settings.BlurMultiplier) > 1e-8 ? settings.BlurMultiplier : 1;
        return new(arm, settings.MaxArm * horizontalTan * 2,
            depthFactor > 1e-8 ? settings.MaxArm / depthFactor : double.MaxValue,
            horizontal, depth, -(settings.MinAngle + (settings.MaxAngle - settings.MinAngle) * eased),
            arm + settings.FogOffset + depth * 0.5,
            (settings.MinFStop + (settings.MaxFStop - settings.MinFStop) * blurAlpha) / blurScale);
    }

    internal static CameraVector Correction(CameraVector separation, CameraVector displacement,
        CameraVector right, CameraVector forward, double horizontalLimit, double depthLimit)
    {
        CameraVector correction = default;
        if (Math.Abs(separation.Dot(right)) > horizontalLimit)
            correction -= right * displacement.Dot(right);
        if (Math.Abs(separation.Dot(forward)) > depthLimit)
            correction -= forward * displacement.Dot(forward);
        return correction;
    }

    private static double RangeFraction(double value, double start, double end) =>
        Math.Abs(end - start) > 1e-8 ? (value - start) / (end - start) : 0;

    private static double HalfFovTangent(double degrees) =>
        Math.Tan(Math.Clamp(degrees, 0.001, 179.999) * Radians * 0.5);
}
