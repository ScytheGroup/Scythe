using Sharpmake;
using System;
using System.IO;
using System.Collections.Generic;
using Fragment = Sharpmake.Fragment;

namespace GameEngine;

[Fragment, Flags]
public enum Optimization
{
    Debug = 1 << 0,
    Development = 1 << 1,
    Release = 1 << 2,
}
    
[Fragment, Flags]
public enum Compiler
{
    MSVC = 1 << 0,
    ClangCl = 1 << 1,
    GCC = 1 << 2,
    Clang = 1 << 3
}

[Fragment, Flags]
public enum ProjectType
{
    Game = 1 << 0,
    Editor = 1 << 1,
    Server = 1 << 2,
    Client = 1 << 3,
    //Profile = 1 << 4,
}

public class CommonTarget : ITarget
{
    public Platform Platform;
    public Compiler Compiler;
    public DevEnv DevEnv;
    public Optimization Optimization;
    public Blob Blob;
    public BuildSystem BuildSystem;
    public ProjectType ProjectType;
    
    public CommonTarget() { }

    public CommonTarget(
        Platform platform,
        Compiler compiler,
        DevEnv devEnv,
        Optimization optimization,
        Blob blob,
        BuildSystem buildSystem,
        ProjectType projectType = ProjectType.Game
    )
    {
        Platform = platform;
        Compiler = compiler;
        DevEnv = devEnv;
        Optimization = optimization;
        Blob = blob;
        BuildSystem = buildSystem;
        ProjectType = projectType;
    }

    public override string Name
    {
        get
        {
            var nameParts = new List<string>
            {
                // Compiler.ToString(),
                Optimization.ToString(),
            };
    
            // if (Blob == Blob.Blob)
            //     nameParts.Add("Blob");
            
            if (ProjectType != ProjectType.Game)
                nameParts.Add(ProjectType.ToString());
            
            var result = string.Join(" ", nameParts);

            return result;

        }
    }
    
    public string SolutionPlatformName
    {
        get
        {
            var nameParts = new List<string>();

            // nameParts.Add(BuildSystem.ToString());
            nameParts.Add(Platform.ToString());
            if (Blob != Blob.NoBlob)
                nameParts.Add(Blob.ToString());

            // if (ProjectType == ProjectType.Editor)
            //     nameParts.Add(ProjectType.ToString());
            
            return string.Join("_", nameParts);
        }
    }

    public string TargetName
    {
        get
        {
            return ProjectType.ToString();
        }
    }
    
    /// <summary>
    /// returns a string usable as a directory name, to use for instance for the intermediate path
    /// </summary>
    public string DirectoryName
    {
        get
        {
            var dirNameParts = new List<string>();

            // dirNameParts.Add(BuildSystem.ToString());
            dirNameParts.Add(Platform.ToString());
            dirNameParts.Add(Optimization.ToString());
            // dirNameParts.Add(Compiler.ToString());
            if (Blob != Blob.NoBlob && BuildSystem == BuildSystem.FastBuild)
                dirNameParts.Add(Blob.ToString());
            // if (ProjectType != ProjectType.Game)
            dirNameParts.Add(ProjectType.ToString());

            return string.Join("/", dirNameParts);
        }
    }   
    
    // To investigate : retail optimization is not used in the generated projects
    public override Sharpmake.Optimization GetOptimization()
    {
        switch (Optimization)
        {
            case Optimization.Debug:
                return Sharpmake.Optimization.Debug;
            case Optimization.Release:
                return Sharpmake.Optimization.Release;
            case Optimization.Development:
                return Sharpmake.Optimization.Release;
            default:
                throw new NotSupportedException("Optimization value " + Optimization.ToString());
        }
    }

    public override Platform GetPlatform()
    {
        return Platform;
    }

    public static CommonTarget[] GetDefaultTargets()
    {
        var result = new List<CommonTarget>();
        // Get all the targets from the static methods
        switch (Util.GetExecutingPlatform())
        {
        case Platform.win64:
            result.AddRange(GetWin64Targets());
            break;
        case Platform.linux:
            result.AddRange(GetLinuxNativeTargets());
            break;
        }
        
        return result.ToArray();
    }
    
    public static CommonTarget[] GetWin64Targets()
    {
        var compiler = Globals.Compiler;
        var targetPlatform = Globals.TargetPlatform; 
        var devEnv = Globals.DevEnvVersion;
        var buildSystem = Globals.BuildSystem;
        var blobMethod = Blob.NoBlob;
        if (Globals.UseUnity)
            blobMethod |= Globals.BuildSystem != BuildSystem.MSBuild ? Blob.Blob : Blob.FastBuildUnitys;

        List<CommonTarget> targets = new List<CommonTarget>();

        var defaultTarget = new CommonTarget(
            targetPlatform,
            compiler,
            devEnv,
            Optimization.Debug | Optimization.Development /*| Optimization.Release*/,
            blobMethod ,
            buildSystem,
            ProjectType.Game | ProjectType.Editor
        );
        targets.Add(defaultTarget);
        
        if (Globals.IsFull)
           targets.Add(((CommonTarget)defaultTarget.Clone(ProjectType.Server | ProjectType.Client)));
        
        targets.Add( ((CommonTarget)defaultTarget.Clone(Optimization.Release, /*ProjectType.Profile |*/ ProjectType.Game)));

        return targets.ToArray();
    }

    public static CommonTarget[] GetLinuxNativeTargets()
    {
        var compiler = Globals.Compiler;
        var targetPlatform = Globals.TargetPlatform;
        var devEnv = DevEnv.make;
        var buildSystem = BuildSystem.FastBuild;
        var blobMethod = Blob.NoBlob;
        if (Globals.UseUnity)
            blobMethod |= Blob.FastBuildUnitys;
        
        var defaultTarget = new CommonTarget(
            targetPlatform,
            compiler,
            devEnv,
            Optimization.Debug | Optimization.Development,
            blobMethod,
            buildSystem,
            ProjectType.Game | ProjectType.Editor
        );
        
        return new[] { defaultTarget, ((CommonTarget)defaultTarget.Clone(Optimization.Release, /*ProjectType.Profile |*/ ProjectType.Game)) };
    }
    
    public override string ToString()
    {
        return Name;
    }
}