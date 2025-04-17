// See https://aka.ms/new-console-template for more information

using System.Diagnostics;
using ScytheReflector.Generator;
using ScytheReflector.Helpers;

namespace ScytheReflector;

class ScytheReflectorMain
{
    public static float Version { get; } = 0.2f;
    private static Parser _parser = new Parser();
    public static List<Tuple<string, string[]>>? Arguments;
    private static string SourceDir { get; set; }
    public static string OutPath { get; private set; } = Directory.GetCurrentDirectory();
    private static bool Verbose { get; set; } = false;
    internal static string ProjectName { get; set; } = "";

    private static bool ForceParsing { get; set; } = false;
    
    public static bool Dump { get; set; } = false;
    public static List<string> ExcludedPaths { get; set; } = new List<string>();
    public static bool FastTypeOnly = false;
    public static bool GenerateEditorFiles = false;
    public static bool NoMainFile = false;
    public static string DependsOn = "";
    
    public static void Main(string[] args)
    {
        var parsedArgs = ParseArgs(args);

        foreach (var parsedArg in parsedArgs)
        {
            switch (parsedArg.Item1)
            {
                case "-source":
                    SourceDir = parsedArg.Item2[0];
                    break;
                case "-verbose":
                case "-v":
                    Verbose = true;
                    break;
                case "-o":
                case "-output":
                    // set output folder
                    OutPath = parsedArg.Item2[0];
                    break;
                case "-force":
                    ForceParsing = true;
                    break;
                case "-name":
                    ProjectName = parsedArg.Item2[0];
                    break;
                case "-exclude":
                    ExcludedPaths.AddRange(parsedArg.Item2);
                    break;
                case "-dump":
                    Dump = true;
                    break;
                case "-fasttypeonly":
                    FastTypeOnly = true;
                    break;
                case "-editor":
                    GenerateEditorFiles = true;
                    break;
                case "-nomain":
                    NoMainFile = true;
                    break;
                case "-dependson":
                    DependsOn = parsedArg.Item2[0];
                    break;
            }
        }
        
        _parser.ParseDir(SourceDir, ForceParsing);
        if (!_parser.HasChanged)
            return;
        
        // ReflectionFileGenerator fileGenerator = new ReflectionFileGenerator();
        // fileGenerator.ProjectName = ProjectName;
        // fileGenerator.Classes = _parser.GetAllUniqueClasses().ToArray();
        // var content = fileGenerator.Generate();
        // var outPath = ScytheReflectorMain.OutPath;
        //
        // if (Directory.Exists(outPath))
        //     outPath = Path.Combine(outPath, "Generated.ixx");
        // File.WriteAllText(outPath, content);
    }

    protected static List<Tuple<string, string[]>> ParseArgs(string[] args)
    {
        List<Tuple<string, string[]>> paramsAndValues = new List<Tuple<string, string[]>>();
        // Tuple<string, string[]>? lastTuple = null;
        List<string> argValues;
        for (int i = 0; i < args.Length; ++i)
        {
            argValues = new List<string>();
            var arg = args[i];
            if (arg.StartsWith("-"))
            {
                // lastTuple = new Tuple<string, string[]>(arg, );
                int j = i + 1;
                while (j < args.Length && !args[j].StartsWith("-"))
                {
                    argValues.Add(args[j++]);
                }

                paramsAndValues.Add(new Tuple<string, string[]>(arg, argValues.Count != 0 ? argValues.ToArray() : []));
            }
        }

        return paramsAndValues;
    }

    public static void DebugWriteLine(string message)
    {
        if (Verbose)
            Console.WriteLine(message);
    }
}