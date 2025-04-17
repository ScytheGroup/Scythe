using System.Text;
using System.Text.RegularExpressions;
using ScytheReflector.Models;

namespace ScytheReflector.Generator;

public class ReflectionFilesGenerator
{
    public static class Template
    {
        public static class InterfaceUnit
        {
            public static string Import = @"export module [ProjectName]Generated:[FileName];
import Reflection;

[ClassFwd]

export namespace Reflection
{
";

            public static string ClassBody = @"
    template <>
    class Class<[ClassName]> : public IClass
    {
    public:
        static consteval StaticString GetName() { return ""[ClassName]""_ss; }
        
        std::vector<Field> GetFields() override;
        static Reference StaticGetField(Reference ref, StaticString name);
        static Reference StaticGetField([ClassName]& ref, StaticString name);

        Reference GetField(Reference ref, StaticString name) override;
        
        static std::vector<std::string> GetChildren();

        [[nodiscard(""Should not discard on CreateChild"")]]
        static [ClassName]* CreateChild(StaticString name, auto&... args); 
    };

    // extern inline struct [ClassName]_register [ClassName]_instance;

";

            /*        template <class T>
        static T& GetField([ClassName]& ref, StaticString name)
        {
            switch (uint32_t{name})
            {
            [FieldTemplateEntries]
            default:
                return {};
            }
        }*/
            public static string FileEnd = @"

}
";


            public static string FieldTemplateEntry =
                @"case ""[FieldName]""_ss:
static_assert(std::is_same_v<decltype(ref.[FieldName]), T>, ""Field type mismatch"");
return ref.[FieldName];
";
        }

        public static class CppFile
        {
            public static string Header = @"module;
#include <Common/Macros.hpp>
module [ProjectName]Generated:[FileName];
import Reflection;
import [ImportString];
import Scythe;
import <type_traits>;

#pragma warning(push)
#pragma warning(disable: 4065)

namespace Reflection
{
";

            public static string ClassContent = @"    std::vector<Field> Class<[ClassName]>::GetFields()
    {
        [FieldArray]
    }

    Reference Class<[ClassName]>::GetField(Reference ref, StaticString name)
    {
        return StaticGetField(ref, name);
    }

    Reference Class<[ClassName]>::StaticGetField(Reference ref, StaticString name)
    {
        [ClassName]* r = ref.Cast<[ClassName]>();
        if (!r)
            return {};
        
        switch (uint32_t{name})
        {
        [FieldGenericSwitchEntries]
        default:
            return {};
        }
    }

    Reference Class<[ClassName]>::StaticGetField([ClassName]& ref, StaticString name)
    {
        switch (uint32_t{name})
        {
        [FieldRefSwitchEntries]
        default:
            return {};
        }
    }

    std::vector<std::string> Class<[ClassName]>::GetChildren()
    {
        return std::vector<std::string> { [Children] };
    }

    [ClassName]* Class<[ClassName]>::CreateChild(StaticString name, auto&... args)
    {
        [ClassName]* result {nullptr};

        switch (uint32_t{name})
        {
        [ChildrenSwitch]
        default:
            return nullptr;
        }
    }

    ECS_REGISTER([ClassName]);

    namespace
    {
        struct [ClassName]_register
        {
            [ClassName]_register() { 
                DebugPrint(""Registering [ClassName]"");
                Reflection::Internal::Register<[ClassName]>(Reflection::Class<[ClassName]>::GetName());
            }
        } [ClassName]_instance();
        // [ClassName]_register* [ClassName]_instance = new [ClassName]_register{};
    }
";

            
            public static string FileEnd = @"}
#pragma warning(pop)";

            public static string FieldString = @"Field{""[FieldType]"", ""[FieldName]""},";

            public static string FieldGenericSwitchEntry = @"case ""[FieldName]""_ss:
            return {r->[FieldName]};
";

            public static string FieldRefSwitchEntry = @"case ""[FieldName]""_ss:
            return {ref.[FieldName]};
";
            public static string FieldArray => @"return std::vector<Field> { 
    [FieldStrings] 
};";
            
            public static string ChildrenSwitch = @"case ""[ChildName]""_ss:
            result = new [ChildName](args...);
            break;";
        }

        public static string EditorOnlyBegin = "#ifdef EDITOR";
        public static string EditorOnlyEnd = "#endif // EDITOR";
    }

    public string ProjectName { get; set; }
    public string FileName { get; set; }
    public string FullFilePath { get; set; }
    public Class[] Classes { get; set; }
    public string OutputFolder { get; set; }
    private string ImportString { get; set; }
    
    private FileInfo FileInfo { get; set; }

    public string CreateCommentHeader(string path)
    {
        // put Generated file for [FileName] here in a boc made out of ///

        string firstMessage = "Reflection file for : " + FullFilePath;
        string secondMessage = "This file was auto-generated by ScytheReflector. Do not modify this file manually.";

        int sizeOfLongestMessage = int.Max(firstMessage.Length, secondMessage.Length);
        int beginningIndexOfFirstMessage = (sizeOfLongestMessage - firstMessage.Length) / 2;
        int beginningIndexOfSecondMessage = (sizeOfLongestMessage - secondMessage.Length) / 2;


        StringBuilder builder = new();
        builder.AppendLine("///" + new string('/', sizeOfLongestMessage) + "///");
        builder.AppendLine("///" + new string(' ', beginningIndexOfFirstMessage) + firstMessage +
                           new string(' ', beginningIndexOfFirstMessage) + "///");
        builder.AppendLine("///" + new string(' ', beginningIndexOfSecondMessage) + secondMessage +
                           new string(' ', beginningIndexOfSecondMessage) + "///");
        builder.AppendLine("///" + new string('/', sizeOfLongestMessage) + "///");

        return builder.ToString();
    }

    // file, fileInfo.classes.ToArray(), fileInfo.ModuleNameOrHeaderFile
    public ReflectionFilesGenerator(string filePath, FileInfo infos, string projectName = "",
        string outputFolder = "")
    {
        ProjectName = projectName;
        FullFilePath = filePath;
        FileName = Path.GetFileNameWithoutExtension(FullFilePath);
        Classes = infos.classes.ToArray();
        OutputFolder = Path.Combine(outputFolder ?? Directory.GetCurrentDirectory(), "Generated");

        ImportString = infos.ModuleNameOrHeaderFile;

        FileInfo = infos;
        
        if (!Directory.Exists(OutputFolder))
            Directory.CreateDirectory(OutputFolder);
    }

    private string GenerateIxx()
    {
        StringBuilder builder = new StringBuilder();
        builder.AppendLine(CreateCommentHeader(FullFilePath));
        
        var headerString = Template.InterfaceUnit.Import;
        if (ProjectName == "")
            headerString = headerString.Replace("[ProjectName]", "");
        else
            headerString = Template.InterfaceUnit.Import.Replace("[ProjectName]", $"{ProjectName}");
        headerString = headerString.Replace("[FileName]", Path.GetFileNameWithoutExtension(FileName));

        List<string> classNames = new();
        foreach (var c in Classes)
        {
            classNames.Add($"export {c.ClassType.ToString().ToLower()} {c.Name};");
        }

        headerString = headerString.Replace("[ClassFwd]", string.Join("\n", classNames));

        builder.AppendLine(headerString);
        
        foreach (var c in Classes)
        {
            if (c.EditorOnly)
            {
                builder.AppendLine(Template.EditorOnlyBegin);
            }
            
            var classBody = Template.InterfaceUnit.ClassBody.Replace("[ClassName]", c.Name);

            StringBuilder fieldTemplateEntries = new StringBuilder();
            StringBuilder fieldStrings = new StringBuilder();
            StringBuilder fieldGenericSwitchEntries = new StringBuilder();
            StringBuilder fieldRefSwitchEntries = new StringBuilder();

            foreach (var f in c.Fields)
            {
                fieldTemplateEntries.Append(Template.InterfaceUnit.FieldTemplateEntry
                    .Replace("[FieldName]", f.Name));

                fieldStrings.Append(Template.CppFile.FieldString
                    .Replace("[FieldName]", f.Name)
                    .Replace("[FieldType]", f.Type));

                fieldGenericSwitchEntries.Append(Template.CppFile.FieldGenericSwitchEntry
                    .Replace("[FieldName]", f.Name));

                fieldRefSwitchEntries.Append(Template.CppFile.FieldRefSwitchEntry
                    .Replace("[FieldName]", f.Name));
            }

            classBody = classBody.Replace("[FieldTemplateEntries]", fieldTemplateEntries.ToString());
            classBody = classBody.Replace("[FieldStrings]", fieldStrings.ToString());
            classBody = classBody.Replace("[FieldGenericSwitchEntries]", fieldGenericSwitchEntries.ToString());
            classBody = classBody.Replace("[FieldRefSwitchEntries]", fieldRefSwitchEntries.ToString());
            builder.AppendLine(classBody);
            
            if (c.EditorOnly)
            {
                builder.AppendLine(Template.EditorOnlyEnd);
            }
        }

        builder.AppendLine(Template.InterfaceUnit.FileEnd);
        return builder.ToString();
    }

    private string GenerateCpp()
    {
        StringBuilder builder = new StringBuilder();
        builder.AppendLine(CreateCommentHeader(FullFilePath));

        var headerString = Template.CppFile.Header;
        if (ProjectName == "")
            headerString = headerString.Replace("[ProjectName]", "");
        else
            headerString = Template.CppFile.Header.Replace("[ProjectName]", $"{ProjectName}");
        headerString = headerString.Replace("[FileName]", Path.GetFileNameWithoutExtension(FileName));

        if (ImportString != "")
        {
            if (FileInfo.IsHeaderFile)
                headerString = headerString.Replace("[ImportString]",
                    $"\"{Path.GetRelativePath(OutputFolder, ImportString).Replace("\\", "/")}\"");
            else
                headerString = headerString.Replace("[ImportString]", ImportString);
            headerString = headerString.Replace("[ImportString]", ImportString);
        }
        else
            headerString = headerString.Replace("[ImportString]", "Common");

        builder.AppendLine(headerString);
        
        foreach (var c in Classes)
        {
            if (c.EditorOnly)
            {
                builder.AppendLine(Template.EditorOnlyBegin);
            }
            var classContent = Template.CppFile.ClassContent.Replace("[ClassName]", c.Name);

            {
                StringBuilder fieldStrings = new StringBuilder();
                StringBuilder fieldGenericSwitchEntries = new StringBuilder();
                StringBuilder fieldRefSwitchEntries = new StringBuilder();

                if (c.Fields.Count == 0)
                    classContent = classContent.Replace("[FieldArray]", "return std::vector<Field>{};");
                else
                {
                    classContent = classContent.Replace("[FieldArray]", Template.CppFile.FieldArray);
                    foreach (var f in c.Fields)
                    {
                        fieldStrings.Append(Template.CppFile.FieldString
                            .Replace("[FieldName]", f.Name)
                            .Replace("[FieldType]", f.Type));

                        fieldGenericSwitchEntries.Append(Template.CppFile.FieldGenericSwitchEntry
                            .Replace("[FieldName]", f.Name));

                        fieldRefSwitchEntries.Append(Template.CppFile.FieldRefSwitchEntry
                            .Replace("[FieldName]", f.Name));
                    }
                }

                classContent = classContent.Replace("[FieldStrings]", fieldStrings.ToString());
                classContent =
                    classContent.Replace("[FieldGenericSwitchEntries]", fieldGenericSwitchEntries.ToString());
                classContent = classContent.Replace("[FieldRefSwitchEntries]", fieldRefSwitchEntries.ToString());
            }

            {
                StringBuilder children = new("");
                StringBuilder childrenSwitch = new("");
                foreach (var cChild in c.Children)
                {
                    children.Append($"\"{cChild}\", \n");
                    childrenSwitch.Append(Template.CppFile.ChildrenSwitch.Replace("[ChildName]", cChild));
                }

                classContent = classContent.Replace("[Children]", children.ToString());
                classContent = classContent.Replace("[ChildrenSwitch]", childrenSwitch.ToString());
            }
            
            builder.AppendLine(classContent);
            
            if (c.EditorOnly)
            {
                builder.AppendLine(Template.EditorOnlyEnd);
            }
        }

        builder.AppendLine(Template.CppFile.FileEnd);

        // builder.AppendLine(postNamespace.ToString());
        
        return builder.ToString();
    }

    // Generates the ixx and cpp file and returns the generated file name
    public string Generate()
    {
        var ixxContent = GenerateIxx();
        var cppContent = GenerateCpp();

        
        var estimatedFileName = EstimateOutputPathInternal(FullFilePath);
        if (FileInfo.GeneratedFile != "")
        {
            estimatedFileName = FileInfo.GeneratedFile;
        }
        else
        {
            var counter = 0;
            var originalName = estimatedFileName;
            while (File.Exists(Path.Combine(OutputFolder, $"{estimatedFileName}.ixx")) || File.Exists(Path.Combine(OutputFolder, $"{estimatedFileName}.cpp")))
            {
                counter++;
                estimatedFileName = $"{originalName}_{counter}";
            }
        }
        
        var ixxPath = Path.Combine(OutputFolder, $"{estimatedFileName}.ixx");
        var cppPath = Path.Combine(OutputFolder, $"{estimatedFileName}.cpp");

        Console.WriteLine($"Writing to {ixxPath}");
        File.WriteAllText(ixxPath, ixxContent);
        Console.WriteLine($"Writing to {cppPath}");
        File.WriteAllText(cppPath, cppContent);
        return estimatedFileName;
    }

    public bool ValidateFileToOverwrite(string path)
    {
        if (!File.Exists(path))
            return true;
        var fileContent = File.ReadAllText(path);
        // Find the path of the original file within the header comment
        var start = fileContent.IndexOf("Reflection file for : ");
        var end = fileContent.IndexOf("This file was auto-generated by ScytheReflector. Do not modify this file manually.", StringComparison.Ordinal);
        if (start == -1 || end == -1)
            return true;
        var originalFileNameMatch = Regex.Match(fileContent, @"Reflection file for : (.+)///");
        var originalFileName = originalFileNameMatch.Groups[1].Value;
        if (originalFileName == FullFilePath)
            return true;
        return false;
    }
    
    private string EstimateOutputPathInternal(string fileName)
    {
        var fileNameWithoutExtension = Path.GetFileNameWithoutExtension(fileName);
        var outputPath = Path.Combine(OutputFolder, $"{fileNameWithoutExtension}.generated");
        return outputPath;
    }
    
    public static string EstimateOutputPath(string fileName, string outputFolder)
    {
        var fileNameWithoutExtension = Path.GetFileNameWithoutExtension(fileName);
        var outputPath = Path.Combine(outputFolder, "Generated", $"{fileNameWithoutExtension}.generated");
        return outputPath;
    }
    
    public bool GeneratedFilesExists(string fileName)
    {
        var estimatedFileName = EstimateOutputPathInternal(fileName);
        var cppFile = estimatedFileName + ".cpp";
        var ixxFile = estimatedFileName + ".ixx";
        Console.WriteLine($"{ixxFile} exists : {File.Exists(ixxFile)}");
        Console.WriteLine($"{cppFile} exits : {File.Exists(cppFile)}");
        return File.Exists(ixxFile) && File.Exists(cppFile);
    }
    
    public static bool GeneratedFilesExists(string fileName, string outputFolder)
    {
        var estimatedFileName = EstimateOutputPath(fileName, outputFolder);
        var cppFile = estimatedFileName + ".cpp";
        var ixxFile = estimatedFileName + ".ixx";
        return File.Exists(ixxFile) && File.Exists(cppFile);
    }
}