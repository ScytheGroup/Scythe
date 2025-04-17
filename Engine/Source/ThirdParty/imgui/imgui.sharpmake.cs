// Copyright (c) Ubisoft. All Rights Reserved.
// Licensed under the Apache 2.0 License. See LICENSE.md in the project root for license information.

using System.IO;
using Sharpmake;
using System;

namespace GameEngine
{
    [Generate]
    public class ImguiExport : ScytheTpLib
    {
        public ImguiExport()
        {
            Name = @"imgui";
            SourceRootPath = Util.GetCurrentSharpmakeFileInfo().DirectoryName;
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
            conf.IncludePaths.Add(SourceRootPath);
        }
    }
}
