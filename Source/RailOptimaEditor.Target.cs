// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RailOptimaEditorTarget : TargetRules
{
    public RailOptimaEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.AddRange(new string[] { "RailOptima" });
    }
}