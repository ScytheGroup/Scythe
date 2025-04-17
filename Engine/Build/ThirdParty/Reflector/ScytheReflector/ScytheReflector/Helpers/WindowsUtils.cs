using Microsoft.VisualStudio.Setup.Configuration;
using Microsoft.Win32;

namespace ScytheReflector.Helpers;

public class WindowsUtils
{
    private static string WindowsKitPath = "";
    private static string VisualStudioPath = "";

    // walk through the registry to find the Windows Kits path
    public static string GetWindowsKitPath()
    {
        if (WindowsKitPath != "")
            return WindowsKitPath;

        using (RegistryKey? key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows Kits\Installed Roots"))
        {
            if (key != null)
            {
                WindowsKitPath = key.GetValue("KitsRoot10") as string;
            }
        }

        if (WindowsKitPath == "")
            throw new Exception("Could not find Windows Kit Path");

        return WindowsKitPath;
    }

    private static string FindLatestKitVersion(string kitPath)
    {
        string latestVersion = "";
        string[] versions = Directory.GetDirectories(kitPath);
        foreach (var version in versions)
        {
            if (latestVersion == "")
            {
                latestVersion = version;
                continue;
            }

            if (string.Compare(version, latestVersion) > 0)
                latestVersion = version;
        }

        return latestVersion;
    }

    public static string GetWindowsIncludePath()
    {
        return FindLatestKitVersion(Path.Combine(GetWindowsKitPath(), "Include"));
    }

    public static string GetWindowsLibPath()
    {
        return FindLatestKitVersion(Path.Combine(GetWindowsKitPath(), "Lib"));
    }

    public static string GetWindowsSdkPath()
    {
        return FindLatestKitVersion(Path.Combine(GetWindowsKitPath(), "SDK"));
    }

    // always returns the latest version of Visual Studio
    public static string GetVisualStudioPath()
    {
        var setupConfig = new SetupConfiguration();
        var query = (ISetupConfiguration2)setupConfig;
        var e = query.EnumAllInstances();

        int fetched;
        var instances = new ISetupInstance[1];
        e.Next(1, instances, out fetched);
        if (fetched > 0)
        {
            VisualStudioPath = instances[0].GetInstallationPath();
        }
        
        return VisualStudioPath;
    }
    
    public static string GetLatestMSVCPath()
    {
        return FindLatestKitVersion(Path.Combine(GetVisualStudioPath(), "VC", "Tools", "MSVC"));
    }
    
    public static string GetMSVCIncludePath()
    {
        return Path.Combine(GetLatestMSVCPath(), "Include");
    }
    
    public static string GetLLVMPath()
    {
        return Path.Combine(GetVisualStudioPath(), "VC", "Tools", "Llvm");
    }
    
}