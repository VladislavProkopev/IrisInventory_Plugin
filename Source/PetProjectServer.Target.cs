using UnrealBuildTool;
using System.Collections.Generic;

public class PetProjectServerTarget : TargetRules
{
    public PetProjectServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("PetProject");
    }
}