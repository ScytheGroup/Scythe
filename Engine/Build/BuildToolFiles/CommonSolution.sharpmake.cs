using Sharpmake;
using System;
using System.IO;
using System.Reflection;
using System.Linq;
using System.Runtime;

namespace GameEngine;

public class CommonSolution : Sharpmake.Solution
{
    public CommonSolution()
        : base(typeof(CommonTarget))
    {
        IsFileNameToLower = false;
    }

    [ConfigurePriority(ConfigurePriorities.All)]
    [Configure]
    public virtual void ConfigureAll(Configuration conf, CommonTarget target)
    {
        conf.SolutionFileName = "[solution.Name]";
        if (target.DevEnv != CommonProject.GetPlatformDefaultDevEnv())
            conf.SolutionFileName += "_[target.DevEnv]";
        conf.PlatformName = "[target.SolutionPlatformName]";
        conf.SolutionPath = @"[solution.SharpmakeCsPath]/..";

        foreach (Type toolType in Assembly.GetExecutingAssembly().GetTypes().Where(t => !t.IsAbstract &&
                     t.IsSubclassOf(typeof(ToolProject))))
        {
            conf.AddProject(toolType, target, true);
        }
    }
    
    [ConfigurePriority(ConfigurePriorities.BuildSystem)]
    [Configure(BuildSystem.FastBuild)]
    public virtual void ConfigureFastBuild(Configuration conf, CommonTarget target)
    {
        // conf.SolutionPath += "_FBuild";
    }
}