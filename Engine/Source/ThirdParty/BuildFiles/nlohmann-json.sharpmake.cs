using System.IO;
using Sharpmake;
using System;

namespace GameEngine
{
    [Generate]
    public class NlohmannJsonExport : ScytheTpLib
    {
        public NlohmannJsonExport()
        {
            RootPath = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
            ProjectGlobals.ProjectDirectory = Path.Combine(RootPath, "..", "nlohmann-json");
            SourceRootPath = Path.Combine(ProjectGlobals.ProjectDirectory, "single_include");
            Name = "nlohmann-json";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
            conf.IncludePaths.Add(Path.Combine(SourceRootPath));
            conf.Output = Configuration.OutputType.None;
            conf.Options.Add(Options.Vc.General.WarningLevel.Level0);
        }
    }
}
