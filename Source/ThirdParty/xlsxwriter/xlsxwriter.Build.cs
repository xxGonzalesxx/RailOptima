
using UnrealBuildTool;
using System.IO;

public class xlsxwriter : ModuleRules
{
    public xlsxwriter(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string ModulePath = ModuleDirectory;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Windows: локальные библиотеки и заголовки.
            PublicIncludePaths.Add(
                Path.Combine(ModulePath, "include")
            );

            PublicAdditionalLibraries.Add(
                Path.Combine(ModulePath, "lib", "xlsxwriter.lib")
            );

            PublicAdditionalLibraries.Add(
                Path.Combine(ModulePath, "lib", "libzs.lib")
            );
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            // Linux: системные заголовки libxlsxwriter 1.1.7.
            PublicSystemIncludePaths.Add("/usr/include");

            // Подключаем библиотеки по абсолютным путям.
            PublicAdditionalLibraries.Add("/usr/lib64/libxlsxwriter.so");
            PublicAdditionalLibraries.Add("/usr/lib64/libz.so");
        }
    }
}
