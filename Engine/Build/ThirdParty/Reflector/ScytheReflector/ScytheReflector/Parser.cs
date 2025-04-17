using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using ClangSharp.Interop;
using ScytheReflector.Generator;
using ScytheReflector.Helpers;
using ScytheReflector.Models;

namespace ScytheReflector;

public struct FieldInfo
{
    public string Type;

    public string Name;
    // int offset
}

public class FileInfo
{
    // public string Path;
    [JsonInclude] public string ModuleNameOrHeaderFile = "";
    [JsonInclude] public bool IsHeaderFile = false;
    [JsonInclude] public string Checksum = "";
    [JsonInclude] public List<Class> classes = new();
    [JsonInclude] public string GeneratedFile = "";
    [JsonInclude] public string PartitionName = "";
    public bool HasChanged = false;

    // ToJson
    public override string ToString()
    {
        return JsonSerializer.Serialize(this, JsonSerializerOptions.Default);
    }

    public string ToJson()
    {
        return JsonSerializer.Serialize(this, JsonSerializerOptions.Default);
    }
}

public class ProjectDB
{
    public float Version { get; private set; }
    [JsonInclude] public Dictionary<string, FileInfo> Data;

    public ProjectDB(Dictionary<string, FileInfo> data)
    {
        Data = data;
        Version = ScytheReflectorMain.Version;
    }
}

public class Parser
{
    private StringBuilder _builder = new StringBuilder();
    public Dictionary<string, FileInfo> filesParsed = new();
    private string includesString = "";

    private string dbPath = "";
    private string precompiledModulesPath = "";
    private List<string> exportedModules = new();

    public List<Class> Classes => filesParsed.Values.SelectMany(x => x.classes).ToList().Distinct().ToList();
    private Dictionary<string, Class> DiscoveredTypes { get; set; } = new();
    public List<string> ResolvedTypes { get; set; } = new();

    private Dictionary<string, List<FieldInfo>> _fields = new Dictionary<string, List<FieldInfo>>();

    private Dictionary<string, HashSet<string>> _children = new Dictionary<string, HashSet<string>>();

    private string GeneratedFolder => Path.Combine(ScytheReflectorMain.OutPath, "Generated");

    public bool HasChanged { get; private set; } = false;

    public Parser()
    {
    }

    private string GetHashString(byte[] input)
    {
        StringBuilder sb = new StringBuilder();
        foreach (byte b in input)
            sb.Append(b.ToString("X2"));

        return sb.ToString();
    }

    public void ParseDir(string path, bool force = false)
    {
        // if path is file throw
        if (File.Exists(path))
            throw new ArgumentException("Path is a file, expected a directory");

        {
            var dbName = MD5.Create().ComputeHash(Encoding.ASCII.GetBytes(path));
            var exeDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            dbPath = Path.Combine(exeDir!, "db", $"{GetHashString(dbName) ?? ""}.json");
            if (!File.Exists(dbPath) && Directory.Exists(GeneratedFolder))
                Directory.Delete(GeneratedFolder, true);
        }

        if (!Path.Exists(ScytheReflectorMain.OutPath))
        {
            Directory.CreateDirectory(ScytheReflectorMain.OutPath);
        }

        CreateIncludesString(path);

        Dictionary<string, FileInfo> oldDb = force ? new Dictionary<string, FileInfo>() : ReadDb();

        if (Directory.Exists(GeneratedFolder) && oldDb.Count == 0)
            Directory.Delete(GeneratedFolder, true);

        var excludedPaths = ScytheReflectorMain.ExcludedPaths;
        List<string> excludedFiles = new();
        if (Directory.Exists(GeneratedFolder))
            excludedFiles.AddRange(Directory.GetFiles(GeneratedFolder, "*", SearchOption.AllDirectories));
        foreach (var excludedPath in excludedPaths)
        {
            // if path is a file add it to the list
            if (File.Exists(excludedPath))
                excludedFiles.Add(excludedPath);
            else if (Directory.Exists(excludedPath))
            {
                // if path is a directory add all files in it to the list
                excludedFiles.AddRange(Directory.GetFiles(excludedPath, "*", SearchOption.AllDirectories));
            }
        }

        List<string> moduleFiles = new List<string>(Directory.GetFiles(path, "*.ixx", SearchOption.AllDirectories));
        moduleFiles.AddRange(Directory.GetFiles(path, "*.cppm", SearchOption.AllDirectories));

        var files = new List<string>(moduleFiles);
        files.AddRange(Directory.GetFiles(path, "*.h", SearchOption.AllDirectories));
        files.AddRange(Directory.GetFiles(path, "*.hpp", SearchOption.AllDirectories));
        files.AddRange(Directory.GetFiles(path, "*.hxx", SearchOption.AllDirectories));
        files.AddRange(Directory.GetFiles(path, "*.inl", SearchOption.AllDirectories));

        files = files.Except(excludedFiles).ToList();

        // if some files were removed compared to last run then we need to delete the generated folder
        //  and regenerate everything
        if (oldDb.Count != 0 && oldDb.Count > files.Count)
        {
            HasChanged = true;
            if (Directory.Exists(GeneratedFolder))
                Directory.Delete(GeneratedFolder, true);
        }

        uint filesVisited = 0;
        
        foreach (var file in files)
        {
            var fileInfo = PreparseFile(file);
            if (fileInfo.ModuleNameOrHeaderFile == "EXCLUDED")
            {
                // check if there's an already generated file for this module and delete it
                var moduleFile = Path.Combine(ScytheReflectorMain.OutPath, "Generated",
                    $"{Path.GetFileNameWithoutExtension(file)}.generated.ixx");
                var cppFile = Path.Combine(ScytheReflectorMain.OutPath, "Generated",
                    $"{Path.GetFileNameWithoutExtension(file)}.generated.cpp");
                if (File.Exists(moduleFile))
                    File.Delete(moduleFile);
                if (File.Exists(cppFile))
                    File.Delete(cppFile);
                continue;
            }
            
            fileInfo.HasChanged = true;
            if (oldDb.TryGetValue(file, out var output))
            {
                if (output.Checksum.Equals(fileInfo.Checksum))
                {
                    filesParsed.Add(file, output);
                    ScytheReflectorMain.DebugWriteLine($"file {Path.GetFileName(file)} Unchanged");
                    fileInfo = output;
                    fileInfo.HasChanged = false;

                    foreach (var c in Classes)
                    {
                        if (c.Parent != "")
                        {
                            if (!_children.ContainsKey(c.Parent))
                                _children[c.Parent] = new HashSet<string>();
                            _children[c.Parent].Add(c.Name);
                        }
                    }

                }

                fileInfo.GeneratedFile = output.GeneratedFile;
            }

            if (fileInfo.HasChanged)
            {
                HasChanged = true;
                Console.WriteLine($"Parsing {Path.GetFileName(file)}");
                Parse(file, fileInfo);
                filesParsed.Add(file, fileInfo);
            }

            if (fileInfo.classes.Count == 0)
                continue;

            exportedModules.Add(file);
            filesVisited++;
        }

        foreach (var file in exportedModules)
        {
            var fileInfo = filesParsed[file];

            foreach (var c in fileInfo.classes)
            {
                if (_children.ContainsKey(c.Name))
                {
                    // compare children with c's
                    var children = _children[c.Name].Order().ToList();

                    if (children.Count != c.Children.Count)
                    {
                        c.Children = children;
                        fileInfo.HasChanged = true;
                        HasChanged = true;
                        continue;
                    }
                    
                    // if the two lists are different reset
                    if (!c.Children.SequenceEqual(children))
                    {
                        c.Children = children.ToList();
                        fileInfo.HasChanged = true;
                        HasChanged = true;
                    }
                    
                    // if the generated file does not exist, regenerate
                    if (fileInfo.GeneratedFile == "")
                    {
                        fileInfo.HasChanged = true;
                        HasChanged = true;
                    }
                }
            }
            
            if (!ScytheReflectorMain.Dump)
            {
                var fileGenerator = new ReflectionFilesGenerator(file, fileInfo, ScytheReflectorMain.ProjectName,
                    ScytheReflectorMain.OutPath);
                var estimatedOut = ReflectionFilesGenerator.EstimateOutputPath(file, ScytheReflectorMain.OutPath);
                var editorGenerator = new EditorFileGenerator(file, fileInfo, ScytheReflectorMain.ProjectName,
                    ScytheReflectorMain.OutPath);
                if (!fileGenerator.GeneratedFilesExists(file) || fileInfo.HasChanged)
                {
                    fileInfo.GeneratedFile = fileGenerator.Generate();

                    if ((ScytheReflectorMain.GenerateEditorFiles &&
                         fileInfo.classes.Where(x => x.UsesFastType).Count() > 0) ||
                        !editorGenerator.GeneratedFilesExists(file))
                    {
                        editorGenerator.Generate();
                    }
                }

                filesParsed[file] = fileInfo;
            }
        }
        
        if (HasChanged && !ScytheReflectorMain.NoMainFile)
            MasterFileGenerator.Generate(exportedModules.ToArray(), GetAllUniqueClasses().ToArray(),
                ScytheReflectorMain.ProjectName, ScytheReflectorMain.OutPath);
        else if (ScytheReflectorMain.NoMainFile)
        {
            if (File.Exists(
                    Path.Combine(ScytheReflectorMain.OutPath, ScytheReflectorMain.ProjectName + "Generated.cpp")))
                File.Delete(
                    Path.Combine(ScytheReflectorMain.OutPath, ScytheReflectorMain.ProjectName + "Generated.cpp"));
        }

        if (ScytheReflectorMain.Dump)
        {
            // clear the console
            Console.Clear();
            // dump all classes that should be generated
            HashSet<string> visited = new();
            StringBuilder builder = new();
            foreach (var file in exportedModules)
            {
                string fileName = Path.GetFileNameWithoutExtension(file);
                if (visited.Contains(file))
                    fileName += "_1"; // Does not handle more than 1 file with the same name
                Console.WriteLine($"{fileName}.generated.ixx");
                Console.WriteLine($"{fileName}.generated.cpp");
                builder.AppendLine($"{fileName}.generated.ixx");
                builder.AppendLine($"{fileName}.generated.cpp");
                visited.Add(file);
            }

            File.WriteAllText(Path.Combine(ScytheReflectorMain.OutPath, "dump.txt"), builder.ToString());
        }

        //  write new db
        if (HasChanged)
            WriteDb();

        ScytheReflectorMain.DebugWriteLine($"Visited {filesVisited} files");
    }

    private static string[] moduleExtensions = new[]
    {
        ".ixx",
        ".cppm",
    };

    public void Parse(string file, FileInfo fileInfo)
    {
        if (File.ReadAllText(file).Contains("REFLECTOR_IGNORE"))
            return;

        var index = CXIndex.Create(false, false);

        bool isModule = moduleExtensions.Any(x => file.EndsWith(x));

        string[] args = new[]
        {
            // c++ 23
            // "-std=c++2c",
            isModule ? "-xc++-module" : "",
            // "-cc1",
            isModule ? "--precompile" : "",
            "-fms-extensions",
            "-fms-compatibility",
            "-fms-compatibility-version=19",
        };

        args = args.Append(includesString).ToArray();
        CXTranslationUnit unit = CXTranslationUnit.Parse(index, file, args, null,
            CXTranslationUnit_Flags.CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_Flags.CXTranslationUnit_SingleFileParse |
            CXTranslationUnit_Flags.CXTranslationUnit_KeepGoing | CXTranslationUnit_Flags.CXTranslationUnit_Incomplete |
            CXTranslationUnit_Flags.CXTranslationUnit_SkipFunctionBodies);
        if (unit == null)
            return;

        ScytheReflectorMain.DebugWriteLine($"Parsing file {Path.GetFullPath(file)}");

        CXCursor cursor = unit.Cursor;
        CXCursor previous = cursor;
        unsafe
        {
            cursor.VisitChildren(((cxCursor, parent, data) =>
            {
                if (!cxCursor.Location.IsFromMainFile)
                    return CXChildVisitResult.CXChildVisit_Continue;

                // if you find a comment called REFLECTOR_IGNORE, skip this file
                if (cxCursor.Spelling.ToString() == "REFLECTOR_IGNORE")
                    return CXChildVisitResult.CXChildVisit_Break;

                // if last was a template or concept, skip. Also don't include templated types and stuff.
                if (previous.IsTypeConcept || cxCursor.IsAnonymous ||
                    cxCursor.Type.Spelling.ToString().Contains('<') ||
                    cxCursor.Type.Spelling.ToString().Contains("::") || cxCursor.IsTemplated
                    || !cxCursor.IsThisDeclarationADefinition)
                    return CXChildVisitResult.CXChildVisit_Continue;

                switch (cxCursor.kind)
                {
                    case CXCursorKind.CXCursor_ClassDecl:
                    case CXCursorKind.CXCursor_StructDecl:
                        ResolvedTypes.Add(cxCursor.Type.Spelling.ToString());
                        fileInfo.classes.Add(new Class(cxCursor.kind, cxCursor.Type.Spelling.ToString()));

                        if (!ScytheReflectorMain.Dump)
                        {
                            ScytheReflectorMain.DebugWriteLine($"Found class {cxCursor.Type.Spelling}");
                            ParseClass(cxCursor, fileInfo.classes.Last());
                            if (fileInfo.classes.Last().Name == "REMOVE")
                                fileInfo.classes.RemoveAt(fileInfo.classes.Count - 1);
                        }

                        return CXChildVisitResult.CXChildVisit_Continue;
                        break;
                }

                previous = cxCursor;

                return CXChildVisitResult.CXChildVisit_Recurse;
            }), new CXClientData());
        }

        unit.Dispose();
        index.Dispose();

        // remove temp.cppm
        File.Delete("temp.cppm");
    }

    // Macro names which are used to declare fast types
    private static readonly string[] FastTypeDeclares = new[] { "SCLASS", "SSTRUCT" };
    private static readonly string ManualEditorMacro = "MANUALLY_DEFINED_EDITOR";
    private static readonly string EditorOnlyMacro = "EDITOR_ONLY";

    public unsafe void ParseClass(CXCursor cxCursor, Class @class)
    {
        if (cxCursor.IsDefinition)
        {
            var typeName = cxCursor.Type.Spelling.ToString();
            // Visit children to find fields

            if (cxCursor.Location.IsFromMainFile)
            {
                // get the line number of the class
                CXFile file = default;
                uint lineNum = 0;
                cxCursor.Location.GetFileLocation(out file, out lineNum, out var col, out var offset);
                if (TryFindBaseClass(file.Name.ToString(), lineNum, out var parentName))
                {
                    @class.Parent = parentName;
                    if (!_children.ContainsKey(parentName))
                        _children[parentName] = new HashSet<string>();
                    _children[parentName].Add(typeName);
                }
            }

            cxCursor.VisitChildren((fieldCursor, fieldParent, fieldData) =>
            {
                if (!fieldCursor.Location.IsFromMainFile)
                    return CXChildVisitResult.CXChildVisit_Continue;

                // // if base class found, store
                // if (fieldCursor.kind == CXCursorKind.CXCursor_CXXBaseSpecifier)
                // {
                //     var parentName = fieldCursor.Type.Spelling.ToString();
                // }

                // if the class contains one of the fast type declares, set the UsesFastType to true
                foreach (var dec in FastTypeDeclares)
                {
                    if (fieldCursor.Spelling.ToString().Contains(dec))
                    {
                        @class.UsesFastType = true;
                        break;
                    }
                }

                // if the class contains the manual editor macro, set the ManualEditor to true
                if (fieldCursor.Spelling.ToString().Contains(ManualEditorMacro))
                {
                    @class.ManualEditor = true;
                }

                // if the class contains the manual editor macro, set the ManualEditor to true
                if (fieldCursor.Spelling.ToString().Contains(EditorOnlyMacro))
                {
                    @class.EditorOnly = true;
                }

                switch (fieldCursor.kind)
                {
                    case CXCursorKind.CXCursor_CXXMethod:
                        if (fieldCursor.CXXAccessSpecifier == CX_CXXAccessSpecifier.CX_CXXPublic)
                        {
                            var methodName = fieldCursor.Spelling.ToString();
                            var methodType = fieldCursor.Type.Spelling.ToString();
                            ScytheReflectorMain.DebugWriteLine($"Found method {methodType} {methodName} in {typeName}");
                        }

                        break;
                    case CXCursorKind.CXCursor_UnionDecl:
                    case CXCursorKind.CXCursor_EnumDecl:
                        return CXChildVisitResult.CXChildVisit_Recurse;
                    case CXCursorKind.CXCursor_FieldDecl:
                    case CXCursorKind.CXCursor_VarDecl:
                    {
                        if (fieldCursor.CXXAccessSpecifier != CX_CXXAccessSpecifier.CX_CXXPublic)
                            return CXChildVisitResult.CXChildVisit_Continue;

                        var fieldName = fieldCursor.Spelling.ToString();
                        var fieldType = fieldCursor.Type.Spelling.ToString();
                        @class.Fields.Add(new Class.Field { Name = fieldName, Type = fieldType });
                        ScytheReflectorMain.DebugWriteLine($"Found field {fieldType} {fieldName} in {typeName}");
                        // get the line number of the field
                        CXFile file = default;
                        uint lineNum = 0;
                        fieldCursor.Location.GetFileLocation(out file, out lineNum, out var col, out var offset);
                        if (!ValidateField(file.Name.ToString(), lineNum, @class.Fields.Last()))
                        {
                            ScytheReflectorMain.DebugWriteLine($"Field {fieldName} in {typeName} is invalid");
                            @class.Fields.RemoveAt(@class.Fields.Count - 1);
                        }

                        break;
                    }
                    case CXCursorKind.CXCursor_ClassDecl:
                    case CXCursorKind.CXCursor_StructDecl:
                    {
                        var nestedClass = new Class(fieldCursor.kind, fieldCursor.Type.Spelling.ToString());
                        DiscoveredTypes.TryAdd(nestedClass.Name, nestedClass);
                        break;
                    }
                }

                return CXChildVisitResult.CXChildVisit_Continue;
            }, new CXClientData());
        }

        if (ScytheReflectorMain.FastTypeOnly && @class.UsesFastType == false)
        {
            // if the class does not use fast types, remove it
            @class.Fields.Clear();
            @class.Name = "REMOVE";
        }
    }

    private bool ValidateField(string file, uint line, Class.Field field)
    {
        var lineContent = File.ReadAllLines(file)[line - 1];

        var typeMatch = Regex.Match(lineContent,
            @$"([_a-zA-Z:][_a-zA-Z:0-9<>(,\s*)]+\s*[\*&]*)\s+([_a-zA-Z][0-9_A-Za-z]*,\W*)*({field.Name})");
        if (typeMatch.Success)
        {
            var type = typeMatch.Groups[1].Value;
            if (type != field.Type)
            {
                field.Type = type.Trim();
                DiscoveredTypes.TryAdd(type, new Class { Name = type });
            }

            return true;
        }

        return false;
    }

    public bool TryFindBaseClass(string file, uint line, out string ParentName)
    {
        var lineContent = File.ReadAllLines(file)[line - 1];
        var baseClassMatch = Regex.Match(lineContent,
            @"class\s+([_a-zA-Z][0-9_A-Za-z]*)\s*:\s*public\s+([_a-zA-Z][0-9_A-Za-z]*)");
        if (baseClassMatch.Success)
        {
            ParentName = baseClassMatch.Groups[2].Value;
            return true;
        }

        ParentName = "";
        return false;
    }

    public FileInfo PreparseFile(string filePath)
    {
        using var sha256 = SHA256.Create();
        var fileBytes = File.ReadAllBytes(filePath);
        var checksumBytes = sha256.ComputeHash(fileBytes);
        var checksum = BitConverter.ToString(checksumBytes).Replace("-", "").ToLowerInvariant();

        var fileInfo = new FileInfo
        {
            Checksum = checksum
        };

        var content = File.ReadAllText(filePath);

        // if extension is header file (.h*), set the module name to the file name
        if (filePath.EndsWith(".h") || filePath.EndsWith(".hpp") || filePath.EndsWith(".hxx"))
        {
            fileInfo.ModuleNameOrHeaderFile = filePath;
            fileInfo.IsHeaderFile = true;
        }
        else
        {
            // Check if the file has an export module declaration
            var match = Regex.Match(content, @"export\s+module\s+([a-zA-Z0-9_\.]+)(:[a-zA-Z0-9_]+)?\s*;",
                RegexOptions.Multiline);
            if (match.Success)
            {
                fileInfo.ModuleNameOrHeaderFile = match.Groups[1].Value;
                if (match.Groups.Count > 2)
                    fileInfo.PartitionName = match.Groups[2].Value;
            }
        }

        var EXCLUDE_MATCH = @"\/\/\s*REFLECTOR_IGNORE";
        if (Regex.IsMatch(content, EXCLUDE_MATCH))
        {
            fileInfo.ModuleNameOrHeaderFile = "EXCLUDED";
        }

        return fileInfo;
    }

// Write db as a proper json document
    public void WriteDb()
    {
        if (ScytheReflectorMain.Dump)
            return;

        if (!Directory.Exists(Path.GetDirectoryName(dbPath)))
            Directory.CreateDirectory(Path.GetDirectoryName(dbPath));
        var json = JsonSerializer.Serialize(new ProjectDB(filesParsed),
            new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(dbPath, json);
    }

    public Dictionary<string, FileInfo> ReadDb()
    {
        if (File.Exists(dbPath))
        {
            try
            {
                var json = File.ReadAllText(dbPath);
                var db = JsonSerializer.Deserialize<ProjectDB>(json);
                if (db.Version == ScytheReflectorMain.Version)
                    return db.Data;
            }
            catch (Exception)
            {
                ScytheReflectorMain.DebugWriteLine("Failed to read db");
            }

            File.Delete(dbPath);
        }

        return new Dictionary<string, FileInfo>();
    }

    // Parses all classes and remove any of them that has the same name plus merges fields that are not common to both
    public List<Class> GetAllUniqueClasses()
    {
        var classes = Classes;
        var uniqueClasses = new List<Class>();
        foreach (var @class in classes)
        {
            if (uniqueClasses.Any(x => x.Name == @class.Name))
            {
                var existing = uniqueClasses.First(x => x.Name == @class.Name);
                var fields = @class.Fields;
                var existingFields = existing.Fields;
                var newFields = new List<Class.Field>();
                foreach (var field in fields)
                {
                    if (existingFields.Any(x => x.Name == field.Name))
                    {
                        var existingField = existingFields.First(x => x.Name == field.Name);
                        if (existingField.Type == field.Type)
                            newFields.Add(existingField);
                    }
                    else
                    {
                        newFields.Add(field);
                    }
                }

                existing.Fields = newFields;
            }
            else
            {
                uniqueClasses.Add(@class);
            }
        }

        return uniqueClasses;
    }

    private void CreateIncludesString(string sourceDir)
    {
        StringBuilder builder = new StringBuilder();
        builder.Append($"-I\"{sourceDir}\"");
        foreach (var dir in Directory.GetDirectories(sourceDir, "*", SearchOption.AllDirectories))
        {
            builder.Append($" -I\"{dir}\"");
        }

        // if windows, add windows include path
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            var windowsIncludePath = WindowsUtils.GetWindowsIncludePath();
            if (windowsIncludePath != "")
                builder.Append($" -I\"{windowsIncludePath}\"");

            var msvcIncludePath = WindowsUtils.GetMSVCIncludePath();
            if (msvcIncludePath != "")
                builder.Append($" -I\"{msvcIncludePath}\"");
        }

        builder.Append($" -I{precompiledModulesPath}");

        includesString = builder.ToString();

        ScytheReflectorMain.DebugWriteLine($"Includes string is : {includesString}");
    }
}