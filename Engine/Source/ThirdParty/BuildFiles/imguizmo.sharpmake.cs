using System.IO;
using Sharpmake;
using System;

namespace GameEngine
{
    [Generate]
    public class ImguizmoExport : ScytheTpLib
    {
        public ImguizmoExport()
        {
            RootPath = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
            ProjectGlobals.ProjectDirectory = Path.Combine(RootPath, "..", "imguizmo");
            SourceRootPath = ProjectGlobals.ProjectDirectory;
            Name = "imguizmo";
            
            ExcludeFolderFromProjects(Util.GetCurrentSharpmakeFileInfo(), SourceRootPath + "/example/");
            ExcludeFolderFromProjects(Util.GetCurrentSharpmakeFileInfo(), SourceRootPath + "/vcpkg-example/");
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
            conf.IncludePaths.Add(SourceRootPath);
            conf.IncludePaths.Add(SourceRootPath + "/../imgui");
        }
    }
}
