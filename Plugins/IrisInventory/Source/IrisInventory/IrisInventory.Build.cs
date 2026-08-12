// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IrisInventory : ModuleRules
{
	public IrisInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		//Включает поддержку Iris: ставит UE_NET_HAS_IRIS_FASTARRAY_BINDING=1 и UE_WITH_IRIS=1,
		//без которых UHT генерирует UE_NET_IMPLEMENT_FASTARRAY_STUB вместо реальной привязки,
		//и FastArray под Iris не реплицируется вовсе.
		//IrisCore добавляется этим вызовом сам - отдельно перечислять не нужно
		SetupIrisSupport(Target);
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ModularGameplay",
				"CoreFeatures",
				"GameFeatures",
				"CoreUObject",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"IrisCore",
				"NetCore",
				"StructUtils",
				"GameplayAbilities",
				"GameplayMessageRuntime",
				"UMG",
				

				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
