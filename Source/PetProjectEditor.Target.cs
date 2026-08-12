using UnrealBuildTool;
using System.Collections.Generic;

public class PetProjectEditorTarget : TargetRules
{
    public PetProjectEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PetProject");
    }
}