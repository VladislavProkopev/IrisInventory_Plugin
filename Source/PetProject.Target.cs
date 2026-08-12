using UnrealBuildTool;
using System.Collections.Generic;

public class PetProjectTarget : TargetRules
{
    public PetProjectTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PetProject");
    }
}