using UnrealBuildTool;
using System.Collections.Generic;

public class PetProjectEditorTarget : TargetRules
{
    public PetProjectEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("PetProject");
    }
}