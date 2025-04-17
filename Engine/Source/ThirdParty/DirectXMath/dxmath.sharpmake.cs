using System.IO;
using Sharpmake;
using System;
using System.Linq;

namespace GameEngine
{
    [Generate]
    public class DirectMath : ScytheTpLib
    {
        public DirectMath()
        {
            ProjectGlobals.ProjectDirectory = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
            SourceRootPath = ProjectGlobals.ProjectDirectory;
            RootPath = ProjectGlobals.ProjectDirectory;
            Name = "DXMath";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
            
            conf.IncludePaths.Add(Path.Combine(ProjectGlobals.ProjectDirectory, "Inc"));
            conf.Options.Add(Options.Vc.Compiler.JumboBuild.Disable);
        }
    }
}