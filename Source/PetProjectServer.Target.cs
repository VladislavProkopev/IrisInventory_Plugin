using UnrealBuildTool;
using System.Collections.Generic;

public class PetProjectServerTarget : TargetRules
{
    public PetProjectServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("PetProject");
    }
}