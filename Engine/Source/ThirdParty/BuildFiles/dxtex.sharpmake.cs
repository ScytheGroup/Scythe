using System.IO;
using Sharpmake;
using System;
using System.Linq;

namespace GameEngine
{
    [Generate]
    public class DXTexExport : ScytheTpLib
    {
        public DXTexExport()
        {
            ProjectGlobals.ProjectDirectory = Path.Combine(Util.GetCurrentSharpmakeFileInfo().DirectoryName, "..", "DirectXTex");
            SourceRootPath = ProjectGlobals.ProjectDirectory;
            RootPath = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
            Name = "DXTex";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            // Get all directories under SourceRootPath
            var allDirs = Directory.GetDirectories(SourceRootPath, "*", SearchOption.AllDirectories);

            // Define a list of directories to exclude
            var excludeDirs = new string[] {
                "intermediate",
            };

            // Filter directories, excluding any that match the excludeDirs list
            var includeDirs = allDirs.Where(dir => !excludeDirs.Any(excludeDir => dir.EndsWith(excludeDir, StringComparison.OrdinalIgnoreCase)));

            // Add the filtered directories to the include paths
            conf.IncludePaths.AddRange(includeDirs);

            // Also add the files in /Compiled to include path
            conf.IncludePaths.Add(Path.Combine(SourceRootPath, "Shaders", "Compiled"));

            conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);

            var compileShaderPath = Path.Combine(ProjectGlobals.ProjectDirectory, "Shaders", "CompileShaders.cmd");
            var workingDir = Path.Combine(ProjectGlobals.ProjectDirectory, "Shaders");
            conf.EventPreBuild.Add($"cd {workingDir} && " + compileShaderPath);

            conf.Options.Add(Options.Vc.Compiler.JumboBuild.Disable);
            conf.IsBlobbed = false;
            conf.IncludeBlobbedSourceFiles = false;
        }
    }
}
