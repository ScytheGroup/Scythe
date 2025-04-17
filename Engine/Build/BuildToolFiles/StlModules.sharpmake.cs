using System.IO;
using System.Linq;
using Sharpmake;

namespace GameEngine;

public class ModulesProject : CommonProject
{
    public ModulesProject()
    {
        var StlDir = Path.Combine(Globals.EngineDirectory, "Build", "StlModules");
        RootPath = Path.Combine(Globals.EngineDirectory, "Build");
        SourceRootPath = StlDir;
        
        SourceFilesExtensions.Clear();
        SourceFilesCompileExtensions.Add(".ixx");
        
        StripFastBuildSourceFiles = false;
        AddTargets(CommonTarget.GetDefaultTargets());
    }

    private static bool alreadyCompiledModules = false;
    
    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.Output = Configuration.OutputType.Lib;
        conf.ProjectPath = Path.Combine(Globals.EngineDirectory, "Build", "StlModules");
        if (!alreadyCompiledModules)
        {
            var StlDir = Path.Join(Globals.BuildDirectory, "StlModules");
            if (!Directory.Exists(StlDir))
                Directory.CreateDirectory(StlDir);
            var modulesPath = Path.Join(conf.Compiler.GetVisualStudioVCRootPath(), "modules");
            File.Copy(Path.Join(modulesPath, "std.ixx"), Path.Combine(StlDir, "std.ixx"), true);
            File.Copy(Path.Join(modulesPath, "std.compat.ixx"), Path.Combine(StlDir, "std.compat.ixx"), true);
            alreadyCompiledModules = true;
        }

        conf.Options.Add(Options.Vc.Compiler.MultiProcessorCompilation.Disable);
        conf.SolutionFolder = "FastBuild/StlModules";
    }
}

[Generate]
public class StdModule : ModulesProject
{
    public StdModule()
    {
        SourceFiles.Add("std.ixx");
        DependenciesOrder = 1;
    }
}

[Generate]
public class StdCompatModule : ModulesProject
{
    public StdCompatModule()
    {
        SourceFiles.Add("std.compat.ixx");
        DependenciesOrder = 2;
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency(target, typeof(StdModule));
    }
}


[Generate]
public class StlModules : ModulesProject
{
    public StlModules()
    {
        Name = "StlModules";
        // SourceFiles.Add("std.ixx");
        // SourceFiles.Add("std.compat.ixx");
        // // Sort for std first compat second
        // SourceFiles.SortedValues.Sort((a, b) => a.Contains("std.compat") ? 1 : -1);
    }
    
    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddPublicDependency(target, typeof(StdCompatModule));
    }
}