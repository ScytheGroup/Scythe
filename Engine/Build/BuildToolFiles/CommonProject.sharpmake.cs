using System.Diagnostics;
using Sharpmake;
using System.IO;
using System;
using Sharpmake.Generators.VisualStudio;
using System.Collections.Generic;

#nullable enable

[module: Sharpmake.Include("StlModules.sharpmake.cs")]
[module: Sharpmake.Include("Utils.sharpmake.cs")]

namespace GameEngine;

public static class ConfigurePriorities
{
    public const int All = -75;
    public const int Platform = -50;

    public const int Optimization = -25;

    /*     SHARPMAKE DEFAULT IS 0     */
    public const int Blobbing = 25;
    public const int ProjectType = 30;
    public const int BuildSystem = 50;
    public const int Compiler = 75;
    public const int PostAll = 150;
}

public class CommonProject : Project
{
    protected record CommonProjectGlobals
    {
        public string ProjectDirectory = Path.Combine(Util.SimplifyPath(@"[project.SharpmakeCsPath]"), "..");

        public string ProjectFilesDirectory => Globals.HideProjectFiles
            ? Path.Combine(IntermediateDirectory,
                "ProjectFiles")
            : SourceDirectory;

        public string BuildDirectory => Path.Combine(ProjectDirectory, "Build");

        public string IntermediateDirectory => Path.Combine(ProjectDirectory, "Intermediate");

        public string OutputDirectory => Path.Combine(ProjectDirectory, "Binaries");

        public string SourceDirectory => Path.Combine(ProjectDirectory, "Source");
    }

    private bool IsReflected = false;
    private string ReflectionName = "";

    protected CommonProjectGlobals ProjectGlobals { get; set; } = new CommonProjectGlobals();

    protected CommonProject(bool isReflected = false)
        : base(typeof(CommonTarget))
    {
        IsReflected = isReflected;
        IsFileNameToLower = false;
        IsTargetFileNameToLower = false;

        // ensure that no file will be automagically added
        // SourceFilesExtensions.Clear();
        SourceFiles.Clear();
        ResourceFilesExtensions.Clear();
        PRIFilesExtensions.Clear();
        ResourceFiles.Clear();
        NoneExtensions.Clear();
        SourceFilesExcludeRegex.Clear();

        RootPath = ProjectGlobals.ProjectDirectory;
        SourceRootPath = ProjectGlobals.SourceDirectory;
        SourceFilesExtensions.AddRange(new[] { ".ixx", ".clang-format", ".editorconfig" });
        SourceFilesCompileExtensions.Add(".ixx");
    }

    static public DevEnv GetPlatformDefaultDevEnv()
    {
        switch (Util.GetExecutingPlatform())
        {
            case Platform.win32:
                return DevEnv.vs2019;
            case Platform.win64:
                return DevEnv.vs2022;
            case Platform.linux:
                return DevEnv.make;
            default:
                throw new System.NotImplementedException();
        }
    }

    [ConfigurePriority(ConfigurePriorities.All)]
    [Configure]
    public virtual void ConfigureAll(Configuration conf, CommonTarget target)
    {
        if (target.ProjectType != ProjectType.Game)
            conf.TargetFileName += "[target.ProjectType]";

        conf.ProjectFileName = "[project.Name]";
        if (target.DevEnv != GetPlatformDefaultDevEnv())
            conf.ProjectFileName += "_[target.DevEnv]";
        conf.ProjectPath = Path.Combine(ProjectGlobals.ProjectFilesDirectory);
        conf.IsFastBuild = target.BuildSystem == BuildSystem.FastBuild;

        conf.IntermediatePath = Path.Combine(ProjectGlobals.IntermediateDirectory, @"Build", "[target.DirectoryName]");
        conf.TargetPath = Path.Combine(ProjectGlobals.OutputDirectory, "[target.DirectoryName]");

        conf.TargetLibraryPath = Path.Combine(ProjectGlobals.IntermediateDirectory,
            @"lib/[target.DirectoryName]/[project.Name]");
        conf.Output = Configuration.OutputType.Lib; // defaults to creating static libs

        // Set as UNICODE character set
        conf.Options.Add(Options.Vc.Compiler.CppLanguageStandard.Latest);
        conf.Options.Add(Options.Vc.Compiler.CLanguageStandard.C17);
        conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);
        conf.Options.Add(Options.Vc.Compiler.ConformanceMode.Enable);
        conf.Options.Add(Options.Vc.Compiler.Exceptions.Disable);
        conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);
        conf.Options.Add(Options.Vc.Compiler.BuildStlModules.Enable);

        conf.PrecompHeader = null;

        // conf.CustomProperties.Add("CustomTarget", $"Custom-{target.ProjectType}");
        conf.AdditionalCompilerOptions.Add("/EHsc");

        if (IsReflected)
        {
            var generatedPath = Path.Combine(ProjectGlobals.SourceDirectory, "Reflection", "Generated");
            var exePath = Utils.Reflector.Executable;

            var commandString = exePath +
                                $" -source [project.SourceRootPath] -output {Path.Combine("[project.SourceRootPath]", "Reflection")} " +
                                $"-exclude {Path.Combine("[project.SourceRootPath]", "ThirdParty")} {Path.Combine("[project.SourceRootPath]", "ImGui")} {Path.Combine("[project.SourceRootPath]", "Profiling")} " +
                                $"{Path.Combine("[project.SourceRootPath]", "Editor")} {Path.Combine("[project.SourceRootPath]", "Reflection")} -verbose " +
                                $"-fasttypeonly -editor";
            if (ReflectionName != "")
                commandString += $" -name [project.Name]";
            
            conf.EventPreBuild.Add(commandString);

            if (target.ProjectType != ProjectType.Editor)
            {
                conf.SourceFilesBuildExcludeRegex.Add(".*Editor\\.generated.*");
            }
        }
    }

    [ConfigurePriority(ConfigurePriorities.Platform)]
    [Configure(Platform.win64)]
    public virtual void ConfigureWin64(Configuration conf, CommonTarget target)
    {
        conf.SourceFilesBuildExclude.Add("*/Linux/*");
        conf.Options.Add(Options.Vc.Linker.EmbedManifest.No);
        conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
    }

    [ConfigurePriority(ConfigurePriorities.Platform)]
    [Configure(Platform.linux)]
    public void ConfigureLinux(Configuration conf, CommonTarget target)
    {
        conf.SourceFilesBuildExclude.Add("*/Windows/*");
        // conf.Defines.Add("VULKAN");
    }

    [ConfigurePriority(ConfigurePriorities.Optimization)]
    [Configure(Optimization.Debug)]
    public virtual void ConfigureDebug(Configuration conf, CommonTarget target)
    {
        conf.DefaultOption = Options.DefaultTarget.Debug;
        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebug);
        conf.Defines.Add("SC_DEV_VERSION");
        conf.Defines.Add("PROFILING");
        conf.Defines.Add("RENDERDOC");
        conf.Defines.Add("IMGUI_ENABLED");
    }

    [ConfigurePriority(ConfigurePriorities.Optimization)]
    [Configure(Optimization.Release)]
    public virtual void ConfigureRelease(Configuration conf, CommonTarget target)
    {
        // Testing generation of what is needed for working fastbuild deoptimization when using non-exposed compiler optimization options.
        conf.AdditionalCompilerOptimizeOptions
            .Add("/O2"); // This switch is known but for the purpose of this test we will put in in this field.
        
        conf.AdditionalCompilerOptions.Add("/bigobj");
        // conf.FastBuildDeoptimization = Configuration.DeoptimizationWritableFiles.DeoptimizeWritableFiles;
        conf.DefaultOption = Options.DefaultTarget.Release;
        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreaded);
        // Generate debug symbols for release builds off
        conf.Options.Add(Options.Vc.Linker.GenerateDebugInformation.Disable);
        conf.Options.Add(Options.Vc.Compiler.Optimization.FullOptimization);
    }

    [ConfigurePriority(ConfigurePriorities.Optimization)]
    [Configure(Optimization.Development)]
    public virtual void ConfigureDevelopment(Configuration conf, CommonTarget target)
    {
        // Testing generation of what is needed for working fastbuild deoptimization when using non-exposed compiler optimization options.
        conf.AdditionalCompilerOptimizeOptions
            .Add("/O2"); // This switch is known but for the purpose of this test we will put in in this field.
        // conf.AdditionalCompilerOptimizeOptions
        //     .Add("/Os"); // This switch is known but for the purpose of this test we will put in in this field.
        conf.AdditionalCompilerOptions.Add("/bigobj");
        // conf.FastBuildDeoptimization = Configuration.DeoptimizationWritableFiles.DeoptimizeWritableFiles;
        conf.DefaultOption = Options.DefaultTarget.Release;
        conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreaded);
        conf.Options.Add(Options.Vc.Compiler.Optimization.FullOptimization);

        conf.Defines.Add("SC_DEV_VERSION");
        conf.Defines.Add("PROFILING");
        conf.Defines.Add("RENDERDOC");
        conf.Defines.Add("IMGUI_ENABLED");
    }

    #region Blobs and unitys

    [Configure(Blob.FastBuildUnitys)]
    [ConfigurePriority(ConfigurePriorities.Blobbing)]
    public virtual void FastBuildUnitys(Configuration conf, CommonTarget target)
    {
        conf.FastBuildBlobbed = true;
        conf.FastBuildUnityPath = Path.Combine(ProjectGlobals.IntermediateDirectory, @"unity\[project.Name]");
        conf.IncludeBlobbedSourceFiles = false;
        conf.IsBlobbed = false;
    }

    [Configure(Blob.NoBlob)]
    [ConfigurePriority(ConfigurePriorities.Blobbing)]
    public virtual void BlobNoBlob(Configuration conf, CommonTarget target)
    {
        conf.FastBuildBlobbed = false;
        conf.IsBlobbed = false;

        if (conf.IsFastBuild)
        {
            conf.Name += "_NoBlob";
        }
    }

    [Configure(Blob.Blob)]
    [ConfigurePriority(ConfigurePriorities.Blobbing)]
    public virtual void IsBlob(Configuration conf, CommonTarget target)
    {
        conf.Name += "_Blob";
        conf.ProjectFileName += "_Blob";
        conf.Options.Add(Options.Vc.Compiler.JumboBuild.Enable);
        conf.FastBuildBlobbed = false;
        conf.BlobPath = Path.Combine(ProjectGlobals.IntermediateDirectory, @"unity\[project.Name]");
        conf.IncludeBlobbedSourceFiles = true;
        conf.IsBlobbed = true;
    }

    #endregion

    // Todo reassess this

    #region Project Type

    [ConfigurePriority(ConfigurePriorities.ProjectType)]
    [Configure(ProjectType.Game)]
    public virtual void ConfigureGame(Configuration conf, CommonTarget target)
    {
    }

    [ConfigurePriority(ConfigurePriorities.ProjectType)]
    [Configure(ProjectType.Editor)]
    public virtual void ConfigureEditor(Configuration conf, CommonTarget target)
    {
        conf.Defines.Add("EDITOR");
    }

    #endregion

    #region Build system

    [ConfigurePriority(ConfigurePriorities.BuildSystem)]
    [Configure(BuildSystem.MSBuild)]
    public virtual void ConfigureMSBuild(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Vc.Compiler.MultiProcessorCompilation.Enable);
    }

    [ConfigurePriority(ConfigurePriorities.BuildSystem)]
    [Configure(BuildSystem.FastBuild)]
    public virtual void ConfigureFastBuild(Configuration conf, CommonTarget target)
    {
        // conf.SolutionFolder = Path.Combine("FastBuild", conf.SolutionFolder);
        conf.Defines.Add("USES_FASTBUILD");
        conf.TargetLibraryPath =
            conf.IntermediatePath; // .lib files must be with the .obj files when running in fastbuild distributed mode or we'll have missing symbols due to merging of the .pdb

        // Force writing to pdb from different cl.exe process to go through the pdb server
        if (target.Compiler == Compiler.MSVC)
            conf.AdditionalCompilerOptions.Add("/FS");

        // Necessary to prebuild std.ixx before build
        if (target.Compiler == Compiler.MSVC && !GetType().IsSubclassOf(typeof(ModulesProject)))
        {
            conf.AddPublicDependency(target, typeof(StdCompatModule), DependencySetting.OnlyBuildOrder);
        }
    }

    #endregion

    ////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////

    #region Compilers and toolchains

    [ConfigurePriority(ConfigurePriorities.Compiler)]
    [Configure(Compiler.MSVC)]
    public virtual void ConfigureMSVC(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Vc.General.WarningLevel.Level3);
        // conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
    }

    [ConfigurePriority(ConfigurePriorities.Compiler)]
    [Configure(Compiler.ClangCl)]
    public virtual void ConfigureClangCl(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Vc.General.PlatformToolset.ClangCL);
        // conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
        conf.Options.Add(Options.Vc.General.WarningLevel.Level3);

        conf.AdditionalCompilerOptions.Add(
            "-Wno-#pragma-messages",
            "-Wno-c++98-compat",
            "-Wno-microsoft-include",
            "-Wno-unused-variable",
            "-Wno-unused-private-field",
            "-Wno-unused-function",
            "-Wno-nonportable-include-path"
        );

        // conf.AdditionalCompilerOptions.Add("/clang:-fms-compatibility-version=19.30");
        conf.AdditionalCompilerOptions.Add("/clang:-fms-extensions");
        conf.AdditionalCompilerOptions.Add("/clang:-Qunused-arguments");
        // conf.AdditionalCompilerOptions.AddRange(new [] {"/clang:-fmodules", "/clang:--precompile"});
        conf.Options.Add(Options.Vc.Compiler.BuildStlModules.Enable);
    }

    [ConfigurePriority(ConfigurePriorities.Compiler)]
    [Configure(Compiler.Clang)]
    public virtual void ConfigureClang(Configuration conf, CommonTarget target)
    {
        conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);

        conf.AdditionalCompilerOptions.Add(
            "-Wno-#pragma-messages",
            "-Wno-c++98-compat",
            "-Wno-microsoft-include"
        );
    }

    [ConfigurePriority(ConfigurePriorities.Compiler)]
    [Configure(Compiler.GCC)]
    public virtual void ConfigureGCC(Configuration conf, CommonTarget target)
    {
        // conf.Options.Add(Options.Vc.General.PlatformToolset.);

        conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);

        conf.AdditionalCompilerOptions.Add(
            "-Wno-#pragma-messages",
            "-Wno-c++98-compat",
            "-Wno-microsoft-include"
        );
    }

    #endregion

    ////////////////////////////////////////////////////////////////////////

    public static void ExcludeFolderFromBuild(Configuration conf, FileInfo fileInfo, string path,
        string pattern = "*")
    {
        // Exclude profiling code
        if (Directory.Exists(path))
        {
            var files = Directory.GetFiles(path, pattern, SearchOption.AllDirectories);

            foreach (var file in files)
            {
                conf.SourceFilesBuildExclude.Add(Path.GetRelativePath(fileInfo.DirectoryName, file));
            }
        }
    }

    public void ExcludeFolderFromProjects(FileInfo fileInfo, string path, string pattern = "*")
    {
        // Exclude profiling code
        if (Directory.Exists(path))
        {
            var files = Directory.GetFiles(path, pattern, SearchOption.AllDirectories);

            foreach (var file in files)
            {
                SourceFilesExclude.Add(Path.GetRelativePath(fileInfo.DirectoryName, file));
            }
        }
    }

    protected void Reflect(string ReflectionName = "")
    {
        IsReflected = true;
        this.ReflectionName = ReflectionName;
        var reflectorPath = GameEngine.Utils.Reflector.Executable;

        SourceFiles.Add(Path.Combine("[project.SourceRootPath]", "Reflection", "Generated", "*.cpp"));
        SourceFiles.Add(Path.Combine("[project.SourceRootPath]", "Reflection", "Generated", "*.ixx"));
        SourceFiles.Add(Path.Combine("[project.SourceRootPath]", "Reflection", "Generated.ixx"));
        // Regex for any file name containing .generated
        SourceFilesExcludeRegex.Add(".*\\.generated.*");
    }
    
    // Runs the reflection tool on the project immediately
    protected void RunReflectionTool(string ReflectionName = "")
    {
        // var reflectorPath = GameEngine.Utils.Reflector.Executable;

        // var currentSourcePath = SourceRootPath.Replace("[project.SharpmakeCsPath]", SharpmakeCsPath).Replace("[project.SharpmakeCsProjectPath]", SharpmakeCsProjectPath);;
        // // normalize path
        // currentSourcePath = Path.GetFullPath(currentSourcePath);
        // var commandString = reflectorPath +
        //                     $" -source {currentSourcePath} -output {Path.Combine(currentSourcePath, "Reflection")} " +
        //                     $"-exclude {Path.Combine(currentSourcePath, "ThirdParty")} {Path.Combine(currentSourcePath, "ImGui")} {Path.Combine(currentSourcePath, "Profiling")} " +
        //                     $"{Path.Combine(currentSourcePath, "Editor")} {Path.Combine(currentSourcePath, "Reflection")} -verbose " +
        //                     $"-fasttypeonly -editor";
        // if (ReflectionName != "")
        //     commandString += $" -name {currentSourcePath}";
        
        // var process = new Process
        // {
        //     StartInfo = new ProcessStartInfo
        //     {
        //         FileName = reflectorPath,
        //         Arguments = commandString,
        //         UseShellExecute = false,
        //         RedirectStandardOutput = true,
        //         CreateNoWindow = true
        //     }
        // };
        // process.Start();
    }
}


// Attribute to define a public dep
public class PublicDependency : Attribute
{
    public PublicDependency()
    {
    }
}

public class ExportProject : Project
{
    
    protected ExportProject()
        : base(typeof(CommonTarget))
    {
    }

    [Configure, ConfigurePriority(ConfigurePriorities.All)]
    public virtual void ConfigureAll(Configuration conf, CommonTarget target)
    {
    }

    [Configure(Optimization.Debug), ConfigurePriority(ConfigurePriorities.Optimization)]
    public virtual void ConfigureDebug(Configuration conf, CommonTarget target)
    {
    }

    [Configure(Optimization.Release), ConfigurePriority(ConfigurePriorities.Optimization)]
    public virtual void ConfigureRelease(Configuration conf, CommonTarget? target)
    {
    }

    [Configure(Optimization.Development), ConfigurePriority(ConfigurePriorities.Optimization)]
    public virtual void ConfigureDevelopment(Configuration conf, CommonTarget target)
    {
        ConfigureRelease(conf, target);
    }
}

public class ToolProject : CommonProject
{
    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.SolutionFolder = "Engine/Tools";
    }
}