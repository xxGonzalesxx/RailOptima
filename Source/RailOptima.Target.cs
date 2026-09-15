// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RailOptimaTarget : TargetRules
{
    public RailOptimaTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new string[] { "RailOptima" });
    }
}