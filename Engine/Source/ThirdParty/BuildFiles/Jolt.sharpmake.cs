using System.IO;
using Sharpmake;
using System;
using System.Linq;

namespace GameEngine
{
    internal record JoltConfig
    {
        public static string JoltRoot = Path.Combine(Util.GetCurrentSharpmakeFileInfo().DirectoryName, "..", "JoltPhysics");
    }
    
    [Generate]
    public class JoltPhysics : ScytheTpLib
    {
        public JoltPhysics()
        {
            ProjectGlobals.ProjectDirectory = Path.Combine(JoltConfig.JoltRoot,  "Jolt");
            SourceRootPath = ProjectGlobals.ProjectDirectory;
            RootPath = JoltConfig.JoltRoot;
            Name = "JoltPhysics";
            
            var rendererFiles = Directory.GetFiles(Path.Combine(ProjectGlobals.ProjectDirectory, "Renderer"), "*", SearchOption.AllDirectories);
            
            // foreach (var file in rendererFiles)
            // {
            //     var absolutePath = Path.GetFullPath(file);
            //     SourceFilesBuildExclude.Add(absolutePath);
            // }
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.IncludePaths.Add(JoltConfig.JoltRoot);
            if (target.Optimization != Optimization.Release)
            {
                conf.Defines.Add("JPH_DEBUG_RENDERER");
                conf.ExportDefines.Add("JPH_DEBUG_RENDERER");
            }
        }

        public override void ConfigureRelease(Configuration conf, CommonTarget target)
        {
            base.ConfigureRelease(conf, target);
            var rendererFiles = Directory.GetFiles(Path.Combine(ProjectGlobals.ProjectDirectory, "Renderer"), "*", SearchOption.AllDirectories);

            foreach (var file in rendererFiles)
            {
                var absolutePath = Path.GetFullPath(file);
                conf.SourceFilesBuildExclude.Add(absolutePath);
            }
            
            ExcludeFolderFromBuild(conf, Util.GetCurrentSharpmakeFileInfo(),
                Path.Combine(ProjectGlobals.ProjectDirectory, "Renderer"));
            
            conf.Options.Add(Options.Vc.Compiler.Optimization.FullOptimization);
        }
    }

    // [Generate]
    // public class JoltViewer : ToolProject
    // {
    //     public JoltViewer()
    //     {
    //         AddTargets(CommonTarget.GetDefaultTargets());
    //         ProjectGlobals.ProjectDirectory = Path.Combine(JoltConfig.JoltRoot, "JoltViewer");
    //         SourceRootPath = ProjectGlobals.ProjectDirectory;
    //         RootPath = JoltConfig.JoltRoot;
    //         Name = "JoltViewer";
    //     }

    //     public override void ConfigureAll(Configuration conf, CommonTarget target)
    //     {
    //         base.ConfigureAll(conf, target);

    //         conf.IncludePaths.Add(JoltConfig.JoltRoot);
    //         conf.IncludePaths.Add(JoltConfig.JoltRoot + "/JoltViewer");
    //         conf.IncludePaths.Add(JoltConfig.JoltRoot + "/TestFramework");
    //         conf.Output = Configuration.OutputType.Exe;
    //         conf.SolutionFolder = "Engine/ThirdParty/Jolt";
    //         conf.AddPrivateDependency<JoltPhysics>(target);
    //         conf.AddPrivateDependency<JoltTestFramework>(target);
    //     }

    //     public override void ConfigureWin64(Configuration conf, CommonTarget target)
    //     {
    //         base.ConfigureWin64(conf, target);
    //         conf.LibraryFiles.Add("d3d12.lib");
    //         conf.LibraryFiles.Add("shcore.lib");
    //     }
    // }

    // [Generate]
    // public class JoltTestFramework : CommonProject
    // {
    //     public JoltTestFramework()
    //     {
    //         AddTargets(CommonTarget.GetDefaultTargets());
    //         ProjectGlobals.ProjectDirectory = Path.Combine(JoltConfig.JoltRoot, "TestFramework");
    //         SourceRootPath = ProjectGlobals.ProjectDirectory;
    //         RootPath = JoltConfig.JoltRoot;
    //         Name = "TestFramework";
    //     }

    //     public override void ConfigureAll(Configuration conf, CommonTarget target)
    //     {
    //         base.ConfigureAll(conf, target);

    //         conf.IncludePaths.Add(JoltConfig.JoltRoot);
    //         conf.IncludePaths.Add(JoltConfig.JoltRoot + "/TestFramework");
    //         conf.SolutionFolder = "Engine/ThirdParty/Jolt";
    //         conf.AddPrivateDependency<JoltPhysics>(target);
    //     }

    //     public override void ConfigureWin64(Configuration conf, CommonTarget target)
    //     {
    //         base.ConfigureWin64(conf, target);
    //         conf.LibraryFiles.Add("dxguid.lib");
    //         conf.LibraryFiles.Add("dinput8.lib");
    //         conf.LibraryFiles.Add("dxgi.lib");
    //         conf.LibraryFiles.Add("d3d12.lib");
    //         conf.LibraryFiles.Add("d3dcompiler.lib");
    //     }
    // }
}


