// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class demo : ModuleRules
{
	public demo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UMG",
            "Slate",
            "SlateCore"
        });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "EnhancedInput" ,"AIModule",
        "NavigationSystem" });
	}
}
