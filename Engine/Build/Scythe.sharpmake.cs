using System.IO;
using System;
using System.Runtime;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using GameEngine;
using Sharpmake;
using Sharpmake.Generators.FastBuild;
using Sharpmake.Generators.JsonCompilationDatabase;
using static Sharpmake.Options;


[module: Include(@"BuildToolFiles/*.sharpmake.cs")]
[module: Include(@"../Source/ThirdParty/*/*.sharpmake.cs")]

namespace GameEngine;

public static class Globals
{
    public static string RootDirectory;
    public static string EngineRootDirectory;

    public static string ProjectFilesDirectory => Path.Combine(RootDirectory, "Engine");
    // public static string ProjectFilesDirectory => Path.Combine(RootDirectory, "Engine", "Build", "ProjectFiles");

    public static string EngineDirectory => Path.Combine(RootDirectory, "Engine");

    public static string BuildDirectory => Path.Combine(EngineDirectory, "Build");

    public static string IntermediateDirectory => Path.Combine(EngineDirectory, "Intermediate");

    public static string OutputDirectory => Path.Combine(EngineDirectory, "Binaries");

    public static DevEnv DevEnvVersion = DevEnv.vs2022;
    public static Compiler Compiler = Compiler.MSVC;
    public static BuildSystem BuildSystem = BuildSystem.MSBuild;
    public static bool UseUnity = false;
    public static Platform TargetPlatform = Util.GetExecutingPlatform();

    [CommandLine.Option("devenvversion", @"restrict vs version to a specific one")]
    public static void CommandLineDevVersion(string value)
    {
        Console.WriteLine($"DevEnvVersion argument - {value}");
        switch (value)
        {
            case "vs2019":
                DevEnvVersion = DevEnv.vs2019;
                break;
            case "vs2022":
                DevEnvVersion = DevEnv.vs2022;
                break;
        }
    }

    [CommandLine.Option("compiler", @"Specifies the compiler to use")]
    public static void CommandLineCompiler(string value)
    {
        Console.WriteLine($"Compiler argument - {value}");
        switch (value)
        {
            // Could be changed to only have default and clang. Then use target platform to determine
            case "msvc":
                Compiler = Compiler.MSVC;
                break;
            case "clangcl":
                Compiler = Compiler.ClangCl;
                break;
            case "clang":
                Compiler = Compiler.Clang;
                break;
            case "gcc":
                Compiler = Compiler.GCC;
                break;
        }
    }

    [CommandLine.Option("buildsystem", @"Specifies wether to force use msbuild or make instead of fastbuild")]
    public static void CommandLineBuildSystem(string value)
    {
        Console.WriteLine($"Compiler argument - {value}");
        switch (value)
        {
            case "fbuild":
                BuildSystem = BuildSystem.FastBuild;
                break;
            case "msbuild":
                BuildSystem = BuildSystem.MSBuild;
                break;
        }
    }

    [CommandLine.Option("target", @"specify the target platform")]
    public static void CommandLinePlatform(string value)
    {
        Console.WriteLine($"Platform argument - {value}");
        switch (value)
        {
            case "win64":
                TargetPlatform = Platform.win64;
                break;
            case "win32":
                TargetPlatform = Platform.win32;
                break;
            case "linux":
                TargetPlatform = Platform.linux;
                break;
            case "osx":
                TargetPlatform = Platform.mac;
                break;
        }
    }

    [CommandLine.Option("useunity", @"Specifies wether to use unity builds")]
    public static void CommandLineUnity(string value)
    {
        Console.WriteLine($"Unity argument - {value}");
        switch (value)
        {
            case "true":
            case "yes":
                UseUnity = true;
                break;
            case "false":
            case "no":
                UseUnity = false;
                break;
        }
    }

    // Cleanup 
    public static bool Cleanup = false;

    [CommandLine.Option("clean")]
    public static void CommandLineClean()
    {
        Cleanup = true;
    }

    public static bool IsFull = false;

    [CommandLine.Option("full")]
    public static void CommandLineFull()
    {
        IsFull = true;
    }

    public static bool HideProjectFiles = false;

    [CommandLine.Option("hidevsfiles", "wether to hide project files in the intermediate folder or not")]
    public static void CommandLineHideFiles(string value)
    {
        switch (value)
        {
            case "yes":
            case "true":
                HideProjectFiles = true;
                break;
            case "false":
            case "no":
                HideProjectFiles = false;
                break;
        }
    }

    public static bool GenDB = false;
    [CommandLine.Option("gendb", "Generate compile database")]
    public static void CommandLineGenDb()
    {
        GenDB = true;
    }
}

[Generate]
[Export]
public class EngineProj : CommonProject
{
    public EngineProj()
        : base(true)
    {
        var fileInfo = Util.GetCurrentSharpmakeFileInfo();
        string engineDirectory = Globals.EngineDirectory;
        ProjectGlobals.ProjectDirectory = engineDirectory;

        Name = "Scythe";
        StripFastBuildSourceFiles = false;

        RootPath = engineDirectory;
        SourceRootPath = engineDirectory + "/Source";
        
        SourceFilesExtensions.Add(".gen");

        AddTargets(CommonTarget.GetDefaultTargets());

        // Additional extensions
        ResourceFilesExtensions.AddRange(new[] { "ico", "hlsl", "hlsli", "fx" });

        SourceFilesBuildExclude.Add(Path.Combine(Globals.EngineDirectory, "Build"));
        
        var tpFiles = Directory.GetFiles(Path.Combine(ProjectGlobals.SourceDirectory, "ThirdParty"), "*", SearchOption.AllDirectories);
        foreach (var file in tpFiles)
        {
            SourceFilesExclude.Add(Path.GetRelativePath(fileInfo.DirectoryName, file));
        }
        
        Reflect();
        RunReflectionTool();
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;

        conf.IncludePaths.Add(Globals.EngineDirectory);
        conf.IncludePaths.Add(ProjectGlobals.SourceDirectory);
        conf.PrecompHeader = null;
        conf.TargetPath = Path.Combine(Globals.OutputDirectory, "[target.DirectoryName]");
        conf.SolutionFolder = "Engine";

        // Some defs
        conf.Defines.Add("NOMINMAX");
        
        foreach (Type depType in Assembly.GetExecutingAssembly().GetTypes().Where(t => !t.IsAbstract &&
                     t.IsSubclassOf(typeof(ScytheTpLib))))
        {
            if (depType.GetCustomAttribute<PublicDependency>() != null)
                conf.AddPublicDependency(target, depType);
            else
                conf.AddPrivateDependency(target, depType);
        }
        
        foreach (Type depType in Assembly.GetExecutingAssembly().GetTypes().Where(t => !t.IsAbstract &&
                     t.IsSubclassOf(typeof(VCPKG))))
        {
            if (depType.GetCustomAttribute<PublicDependency>() != null)
                conf.AddPublicDependency(target, depType);
            else
                conf.AddPrivateDependency(target, depType);
        }
        
    }

    // [Configure(Platform.win64)]
    public override void ConfigureWin64(Configuration conf, CommonTarget target)
    {
        base.ConfigureWin64(conf, target);
        // conf.AdditionalCompilerOptions.Add("/FS");
        conf.LibraryFiles.Add("d3d11.lib");
        conf.LibraryFiles.Add("dxguid.lib");
        conf.LibraryFiles.Add("winmm.lib");
        conf.LibraryFiles.Add("dxgi.lib");
        conf.LibraryFiles.Add("d3dcompiler.lib");
        conf.LibraryFiles.Add("xinput.lib");
        // Add windows headers
        // conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
        conf.Defines.Add("DX11");
        conf.AdditionalCompilerOptions.Add("/FS");
    }

    public override void ConfigureDebug(Configuration conf, CommonTarget target)
    {
        base.ConfigureDebug(conf, target);
    }

    public override void ConfigureRelease(Configuration conf, CommonTarget target)
    {
        base.ConfigureRelease(conf, target);

        var fileInfo = Util.GetCurrentSharpmakeFileInfo();
        string path = ProjectGlobals.SourceDirectory;
        
        ExcludeFolderFromBuild(conf, fileInfo, Path.Combine(path, "Graphics/HelperTools"));
        ExcludeFolderFromBuild(conf, fileInfo, Path.Combine(path, "Tools/Profiling"));
        ExcludeFolderFromBuild(conf, fileInfo, Path.Combine(path, "imgui"));
    }

    public override void ConfigureGame(Configuration conf, CommonTarget target)
    {
        base.ConfigureGame(conf, target);
        var fileInfo = Util.GetCurrentSharpmakeFileInfo();
        string path = ProjectGlobals.SourceDirectory;

        if (!Directory.Exists(Path.Combine(path, "Editor")))
            return;

        var editorFiles = Directory.GetFiles(Path.Combine(path, "Editor"), "*",
            SearchOption.AllDirectories);

        foreach (var file in editorFiles)
        {
            conf.SourceFilesBuildExclude.Add(Path.GetRelativePath(fileInfo.DirectoryName, file));
        }
    }

    [ConfigurePriority(ConfigurePriorities.PostAll)]
    [Configure]
    public void Post(Configuration conf, CommonTarget target)
    {
        // var projPath = conf.ProjectPath;
        // foreach (var file in conf.TargetCopyFiles)
        // {
        //     var rel = Util.PathGetRelative(projPath, Path.GetFullPath(file));
        //     conf.TargetCopyFiles.Remove(file);
        //     conf.TargetCopyFiles.Add(rel);
        // }
    }
}

[Export]
[Generate]
public class EngineLibProj : EngineProj
{
    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;
        conf.LibraryPaths.Add(conf.TargetPath);
        conf.SourceFilesBuildExcludeRegex.Add(@".*main.*");
        conf.SourceFilesBuildExcludeRegex.Add(@".*Main.*");
        conf.Defines.Add("ScytheLib");
        // conf.TargetPath = Path.Combine(Globals.OutputDirectory, "lib", "[target.DirectoryName]");
    }
}

public class CommonGameProject : CommonProject
{
    public CommonGameProject()
    {
        StripFastBuildSourceFiles = false;

        SourceFilesExtensions.Add(".gen");

        AddTargets(CommonTarget.GetDefaultTargets());

        // Additional extensions
        ResourceFilesExtensions.AddRange(new[] { "ico", "hlsl", "fx" });
        SourceFilesBuildExclude.Add(Path.Combine(ProjectGlobals.ProjectDirectory, "Build"));
        Reflect("[project.Name]");
        RunReflectionTool(Name);
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Exe;

        conf.IncludePaths.Add(Globals.EngineDirectory);

        conf.SolutionFolder = "Game";

        conf.AddPrivateDependency<EngineProj>(target);
        if (target.DevEnv != DevEnv.make && target.DevEnv != DevEnv.xcode)
        {
            if (conf.VcxprojUserFile == null)
                conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings();
            conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = RootPath;
        }
        
        conf.EventPostBuildExe.Add(
            new Configuration.BuildStepCopy(
                Path.Join(RootPath, "Resources"),
                Path.Join(conf.TargetPath, "Resources"),
                false,
                "*",
                false,
                true
            )
        );
        
        conf.EventPostBuildExe.Add(
            new Configuration.BuildStepCopy(
                Path.Join(Globals.EngineDirectory, "Source", "Shaders"),
                Path.Join(conf.TargetPath, "Source", "Shaders"),
                false,
                "*",
                false,
                true
            )
        );
    }
    
    public override void ConfigureEditor(Configuration conf, CommonTarget target)
    {
        base.ConfigureEditor(conf, target);
        
        conf.EventPostBuildExe.Add(
            new Configuration.BuildStepCopy(
                Path.Join(Globals.EngineDirectory, "Resources"),
                Path.Join(conf.TargetPath, "Resources"),
                false,
                "*",
                false,
                false
            )
        );
    }
}

public class ScytheTpLib : CommonProject
{
    public ScytheTpLib()
    {
        AddTargets(CommonTarget.GetDefaultTargets());
        ProjectGlobals.ProjectDirectory = "./";
        SourceRootPath = ProjectGlobals.ProjectDirectory;
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = "Engine/ThirdParty";

        conf.IntermediatePath = Path.Combine(Globals.IntermediateDirectory, @"Build", "ThirdParty", Name,
            "[target.DirectoryName]");
        conf.TargetPath = Path.Combine(Globals.OutputDirectory, "ThirdParty", "[target.DirectoryName]");
        conf.TargetLibraryPath = Path.Combine(Globals.IntermediateDirectory, "ThirdParty",
            @"lib/[target.DirectoryName]/[project.Name]");
        conf.ProjectPath = Util.SimplifyPath(Path.Combine(Globals.IntermediateDirectory, "ProjectFiles"));
        conf.Options.Add(Options.Vc.General.WarningLevel.Level0);
        
        conf.Options.Add(Options.Vc.Compiler.JumboBuild.Enable);
        conf.IsBlobbed = true;
        conf.IncludeBlobbedSourceFiles = true;
    }
}

[Generate]
public class GameEngineSln : CommonSolution
{
    public GameEngineSln()
    {
        Name = "ScytheEngine";
        AddTargets(CommonTarget.GetDefaultTargets());
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddProject<EngineProj>(target);
        conf.SolutionPath = Globals.RootDirectory;
    }
}

public static class Main
{
    private static void ConfigureEngineRootDirectory()
    {
        FileInfo fileInfo = Util.GetCurrentSharpmakeFileInfo();
        // Load .engine_root file to get the root directory
        var parentDir = fileInfo.Directory.Parent!;
        var parentDirFullPath = Path.GetFullPath(parentDir.ToString());
        // in parentDirFullPath, find the .scythe file
        var rootFile = Directory.GetFiles(parentDirFullPath, "*.scythe")[0];
        if (!File.Exists(rootFile))
        {
            throw new Exception("Root directory not found. Please run the setup program first to generate hint files.");
        }

        var content = File.ReadAllText(rootFile).Trim();
        // read all lines in the files and create split them using = as separator. First argument is key second value
        var lines = content.Split('\n');
        foreach (var line in lines)
        {
            var split = line.Split('=');
            if (split.Length != 2)
            {
                throw new Exception("Invalid .scythe file. Please run the setup program first to generate hint files.");
            }

            var key = split[0].Trim();
            var value = split[1].Trim();
            if (key == "engine_dir")
            {
                Globals.EngineRootDirectory = value;
            }
        }

        Globals.EngineRootDirectory = Util.SimplifyPath(Globals.EngineRootDirectory);
        Globals.RootDirectory = Globals.EngineRootDirectory;
    }

    private static void ConfigureAutoCleanup()
    {
        Util.FilesAutoCleanupActive = true;
        Util.FilesAutoCleanupDBPath = Path.Combine(Globals.IntermediateDirectory, "sharpmake");

        if (!Directory.Exists(Util.FilesAutoCleanupDBPath))
            Directory.CreateDirectory(Util.FilesAutoCleanupDBPath);
    }


    private static void GenerateProjectDatabase(Project project)
    {
        var outdir = Path.GetDirectoryName(project.ProjectFilesMapping.First().Key);

        GenerateDatabase(outdir, project.Configurations, CompileCommandFormat.Arguments);
    }

    // private static void GenerateSolutionDatabase(Solution solution)
    // {
    //     var outdir = Path.GetDirectoryName(solution.SolutionFilesMapping.First().Key);
    //
    //     var configs = solution.Configurations.SelectMany(c => c.IncludedProjectInfos.Select(pi => pi.Configuration));
    //
    //     GenerateDatabase(outdir, configs, CompileCommandFormat.Command);
    // }

    private static void GenerateDatabase(string outdir, IEnumerable<Project.Configuration> configs,
        CompileCommandFormat format)
    {
        var builder = Builder.Instance;

        if (builder == null)
        {
            System.Console.Error.WriteLine("CompilationDatabase: No builder instance.");
            return;
        }

        var generator = new JsonCompilationDatabase();

        generator.Generate(builder, outdir, configs, format, new List<string>(), new List<string>());

        builder.Generate();
    }

    [Sharpmake.Main]
    public static void SharpmakeMain(Arguments arguments)
    {
        CommandLine.ExecuteOnType(typeof(Globals));
        ConfigureEngineRootDirectory();
        ConfigureAutoCleanup();

        if (Globals.Cleanup)
        {
            Util.ExecuteFilesAutoCleanup();
            Environment.Exit(0);
        }

        FastBuildSettings.FastBuildWait = true;
        FastBuildSettings.FastBuildSummary = false;
        FastBuildSettings.FastBuildNoSummaryOnError = true;
        FastBuildSettings.FastBuildDistribution = false;
        FastBuildSettings.FastBuildMonitor = true;
        FastBuildSettings.FastBuildAllowDBMigration = true;
        FastBuildSettings.SetPathToResourceCompilerInEnvironment = true;
        FastBuildSettings.IncludeBFFInProjects = true;

        if (Globals.GenDB)
            arguments.Builder.EventPostProjectLink += GenerateProjectDatabase;

        KitsRootPaths.SetKitsRoot10ToHighestInstalledVersion(Globals.DevEnvVersion);

        // FastBuild should be installed and your path should be set to the installation directory
        string sharpmakeFastBuildDir = Path.Combine(Globals.BuildDirectory, @"ThirdParty", "FastBuild");
        switch (Util.GetExecutingPlatform())
        {
            case Platform.win64:
                FastBuildSettings.FastBuildMakeCommand = Path.Combine(sharpmakeFastBuildDir, "Windows", "FBuild.exe");
                break;
            case Platform.linux:
                FastBuildSettings.FastBuildMakeCommand = Path.Combine(sharpmakeFastBuildDir, "Linux", "fbuild");
                break;
            case Platform.mac:
                FastBuildSettings.FastBuildMakeCommand = Path.Combine(sharpmakeFastBuildDir, "OSX", "FBuild");
                break;
        }

        Bff.UnityResolver = new Bff.FragmentUnityResolver();

        // Tells Sharpmake to generate the solution
        arguments.Generate<GameEngineSln>();
        var solutions = Assembly.GetExecutingAssembly().GetTypes().Where(t => !t.IsAbstract &&
                                                                              t.IsSubclassOf(typeof(CommonSolution))
                                                                              && t != typeof(GameEngineSln));
        foreach (Type solutionType in solutions)
            arguments.Generate(solutionType);
    }
}