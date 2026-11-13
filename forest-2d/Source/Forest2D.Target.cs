using UnrealBuildTool;
public class Forest2DTarget : TargetRules
{
    public Forest2DTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Forest2D");
    }
}
