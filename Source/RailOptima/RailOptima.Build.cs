using UnrealBuildTool;

public class RailOptima : ModuleRules
{
    public RailOptima(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "ProceduralMeshComponent",
            "ImageWrapper",
            "HTTP",
            "Json",
            "JsonUtilities",
            "UMG",
            "Slate",
            "SlateCore",
            "Projects"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "HTTP",             // REST API
            "Json",
            "JsonUtilities",
            "UMG",              // UI
            "Slate",
            "SlateCore",
            "ProceduralMeshComponent", // для PathRenderer (лента маршрута),
            "xlsxwriter"
        });

        PublicIncludePaths.Add(ModuleDirectory);
    }
}