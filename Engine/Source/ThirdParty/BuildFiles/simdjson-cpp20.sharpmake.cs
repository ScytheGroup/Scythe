using System.IO;
using Sharpmake;
using System;

namespace GameEngine
{
    [Generate]
    [PublicDependency]
    public class SimdJsonCPP20Export : ScytheTpLib
    {
        public SimdJsonCPP20Export()
        {
            RootPath = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
            ProjectGlobals.ProjectDirectory = Path.Combine(RootPath, "..", "simdjson-cpp20");
            SourceRootPath = ProjectGlobals.ProjectDirectory;
            Name = "simdjson-cpp20";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
            conf.IncludePaths.Add(SourceRootPath);
            conf.IncludePaths.Add(SourceRootPath + "/../imgui");
        }
    }
}
