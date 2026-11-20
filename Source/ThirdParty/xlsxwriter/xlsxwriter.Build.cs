using UnrealBuildTool;
using System.IO;

public class xlsxwriter : ModuleRules
{
    public xlsxwriter(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string ModulePath = ModuleDirectory;

        PublicIncludePaths.Add(Path.Combine(ModulePath, "include"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "lib", "xlsxwriter.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "lib", "libzs.lib"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add("/usr/lib64/libxlsxwriter.so");
            PublicDependencyModuleNames.Add("zlib");
        }
    }
}
