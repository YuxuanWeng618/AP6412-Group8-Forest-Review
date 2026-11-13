using UnrealBuildTool;
public class Forest2DEditorTarget : TargetRules
{
    public Forest2DEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Forest2D");
    }
}
