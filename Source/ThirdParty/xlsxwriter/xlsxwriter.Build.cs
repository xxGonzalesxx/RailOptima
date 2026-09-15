using UnrealBuildTool;
using System.IO;

public class xlsxwriter : ModuleRules
{
    public xlsxwriter(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string ModulePath = ModuleDirectory;

        PublicIncludePaths.Add(Path.Combine(ModulePath, "include"));

        PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "lib", "xlsxwriter.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "lib", "libzs.lib"));
    }
}