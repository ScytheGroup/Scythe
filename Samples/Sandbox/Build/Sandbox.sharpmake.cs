using System;
using System.IO;
using System.Reflection;
using GameEngine;
using Sharpmake;

namespace Sandbox;

[Generate]
public class SandboxProj : CommonGameProject
{
    public SandboxProj()
    {
        Name = "Sandbox";
    }
}

[Generate] 
public class SandboxSln : CommonSolution
{
    public SandboxSln()
    {
        Name = "Sandbox";
        AddTargets(CommonTarget.GetDefaultTargets());
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddProject<SandboxProj>(target);
    }
}
