using System;
using System.IO;
using System.Reflection;
using GameEngine;
using Sharpmake;

namespace [project_name];

[Generate]
public class [project_name]Proj : CommonGameProject
{
    public [project_name]Proj()
    {
        Name = "[project_name]";
    }
}

[Generate] 
public class [project_name]Sln : CommonSolution
{
    public [project_name]Sln()
    {
        Name = "[project_name]";
        AddTargets(CommonTarget.GetDefaultTargets());
    }

    public override void ConfigureAll(Configuration conf, CommonTarget target)
    {
        base.ConfigureAll(conf, target);
        conf.AddProject<[project_name]Proj>(target);
    }
}
