using System.IO;
using Sharpmake;
using System;
using System.Linq;

namespace GameEngine
{
    [Generate]
    public class DXTKExport : ScytheTpLib
    {
        public DXTKExport()
        {
            ProjectGlobals.ProjectDirectory = Path.Combine(Util.GetCurrentSharpmakeFileInfo().DirectoryName, "..", "DirectXTK");
            SourceRootPath = ProjectGlobals.ProjectDirectory;
            RootPath = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
            Name = "DXTK";
            
            var xboxFiles = Directory.GetFiles(Path.Combine(ProjectGlobals.ProjectDirectory), "Xbox*", SearchOption.AllDirectories);
        
            foreach (var file in xboxFiles)
            {
                SourceFilesBuildExclude.Add(Path.GetRelativePath(ProjectGlobals.ProjectDirectory, file));
            }
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
            conf.IncludePaths.Add(Path.Combine(ProjectGlobals.ProjectDirectory, "Src", "Shaders", "Compiled"));

            conf.Options.Add(Options.Vc.Compiler.RTTI.Enable);

            var compileShaderPath = Path.Combine(ProjectGlobals.ProjectDirectory, "Src", "Shaders", "CompileShaders.cmd");
            var workingDir = Path.Combine(ProjectGlobals.ProjectDirectory, "Src", "Shaders");
            conf.EventPreBuild.Add($"cd {workingDir} && " + compileShaderPath);

            conf.AddPublicDependency<DirectMath>(target);
            
            conf.Options.Add(Options.Vc.Compiler.JumboBuild.Disable);
            conf.IsBlobbed = false;
            conf.IncludeBlobbedSourceFiles = false;

        }
    }
}
