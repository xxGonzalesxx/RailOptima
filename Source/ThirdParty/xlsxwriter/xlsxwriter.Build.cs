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
            PublicDefinitions.Add("RAIL_WITH_XLSX=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux
                 && File.Exists(Path.Combine(ModulePath, "lib", "Linux", "libxlsxwriter.a")))
        {
            // Linux-сборка, если вы её положите сюда (см. ниже)
            PublicAdditionalLibraries.Add(Path.Combine(ModulePath, "lib", "Linux", "libxlsxwriter.a"));
            AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");
            PublicDefinitions.Add("RAIL_WITH_XLSX=1");
        }
        else
        {
            PublicDefinitions.Add("RAIL_WITH_XLSX=0");
        }
    }
}