using GameEngine;
using Sharpmake;
using System.IO;
using System;
using System.Linq;

namespace GameEngine;

    // Add a dependency to this project to be able to use vcpkg packages in a project.
    //
    // This project then setup the necessary include and library paths to be able to reference any installed vcpkg package in
    // our local vcpackage installation.
    //
    // Note: The required vcpkg packages are installed by bootstrap-sample.bat
    //
    [Sharpmake.Export]
    public class VCPKG : ExportProject
    {
        protected VCPKG()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
        }
        
        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            conf.Options.Add(Options.Vc.General.WarningLevel.Level0);
        }

        public override void ConfigureRelease(Configuration conf, CommonTarget target)
        {
            base.ConfigureRelease(conf, target);

            // Add root include path for vcpkg packages.
            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\include");
            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows-static\include");

            // Add root lib path for vcpkg packages.
            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\lib");
            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows-static\lib");

            try {
                var fileInfo = Util.GetCurrentSharpmakeFileInfo();
                var dllFiles = Directory.GetFiles(@$"{fileInfo.DirectoryName}\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\bin", "*.dll");
                conf.TargetCopyFilesPath = @"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\bin";
                conf.TargetCopyFiles.AddRange(dllFiles);
            } catch (Exception) {
            }

            try {
                var fileInfo = Util.GetCurrentSharpmakeFileInfo();
                var libFiles = Directory.GetFiles(@$"{fileInfo.DirectoryName}\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows-static\lib", "*.lib");
                conf.LibraryFiles.AddRange(libFiles.Select(x => Path.GetFileName(x)));
            } catch (Exception) {
            }
        }

        public override void ConfigureDebug(Configuration conf, CommonTarget target)
        {
            base.ConfigureDebug(conf, target);

            // Add root include path for vcpkg packages.
            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\include");
            conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows-static\include");

            // Add root lib path for vcpkg packages.
            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\debug\lib");
            conf.LibraryPaths.Add(@"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows-static\debug\lib");

            try {
                var fileInfo = Util.GetCurrentSharpmakeFileInfo();
                var dllFiles = Directory.GetFiles(@$"{fileInfo.DirectoryName}\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\debug\bin", "*.dll");
                conf.TargetCopyFilesPath = @"[project.SharpmakeCsPath]\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows\debug\bin";
                conf.TargetCopyFiles.AddRange(dllFiles);
            } catch (Exception) {
                
            }

            try {
                var fileInfo = Util.GetCurrentSharpmakeFileInfo();
                var libFiles = Directory.GetFiles(@$"{fileInfo.DirectoryName}\..\ThirdParty\vcpkg\extern\vcpkg\installed\x64-windows-static\debug\lib", "*.lib");
                conf.LibraryFiles.AddRange(libFiles.Select(x => Path.GetFileName(x)));
            } catch (Exception) {
            }
        }

        public override void ConfigureDevelopment(Configuration conf, CommonTarget target)
        {
            base.ConfigureDevelopment(conf, target);
        }
    }

    [Sharpmake.Export]
    class assimp : VCPKG
    {
        public override void ConfigureDebug(Configuration conf, CommonTarget target)
        {
            base.ConfigureDebug(conf, target);
            
            conf.LibraryFiles.Add("assimp-vc143-mtd.lib");
        }
        
        public override void ConfigureRelease(Configuration conf, CommonTarget target)
        {
            base.ConfigureRelease(conf, target);
        
            conf.LibraryFiles.Add("assimp-vc143-mt.lib");
        }
    }