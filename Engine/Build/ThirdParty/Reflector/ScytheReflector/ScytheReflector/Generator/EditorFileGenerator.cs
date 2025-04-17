using System.Text;
using ScytheReflector.Models;

namespace ScytheReflector.Generator;

public class EditorFileGenerator
{
    public static class Template
    {
        public static class CppFile
        {
            public static string Header = 
"""
#ifdef EDITOR
module [ModulePartition];
import ImGui;
import Editor;

""";

            public static string MethodBody = 
"""
bool [ClassName]::DrawEditorInfo()
{
    Super::DrawEditorInfo();
    bool result = false;

[DrawFields]
    return result;
}
""";

            public static string FieldEntry = 
"""
    result |= DrawEditorEntry("[FieldNamePascal]", [FieldName]);
""";
        }
    }
 
    public string ProjectName { get; set; }
    public string FileName { get; set; }
    public string FullFilePath { get; set; }
    public Class[] Classes { get; set; }
    public string OutputFolder { get; set; }
    private FileInfo FileInfo { get; set; }
    
    public EditorFileGenerator(string filePath, FileInfo infos, string projectName = "",
        string outputFolder = "")
    {
        ProjectName = projectName;
        FullFilePath = filePath;
        FileName = Path.GetFileNameWithoutExtension(FullFilePath);
        Classes = infos.classes.Where(x => x.UsesFastType).ToArray();
        OutputFolder = Path.Combine(outputFolder ?? Directory.GetCurrentDirectory(), "Generated");

        FileInfo = infos;
        
        if (!Directory.Exists(OutputFolder))
            Directory.CreateDirectory(OutputFolder);
    }
    
    private string EstimateOutputPathInternal(string fileName)
    {
        var fileNameWithoutExtension = Path.GetFileNameWithoutExtension(fileName);
        var outputPath = Path.Combine(OutputFolder, $"{fileNameWithoutExtension}_Editor.generated");
        return outputPath;
    }

    public void Generate()
    {
        if (FileInfo.IsHeaderFile)
        {
            Console.WriteLine("Cannot generate editor file for a non module file.");
            return;
        }
        
        StringBuilder builder = new StringBuilder();

        string header = Template.CppFile.Header;
        string fullModulePartitionName = FileInfo.ModuleNameOrHeaderFile + FileInfo.PartitionName;
        header = header.Replace("[ModulePartition]", fullModulePartitionName);
        
        builder.AppendLine(header);
        
        foreach (var @class in Classes)
        {
            if (@class.ManualEditor)
                continue;
            
            var methodBody = Template.CppFile.MethodBody;
            methodBody = methodBody.Replace("[ClassName]", @class.Name);
            
            StringBuilder fieldEntries = new();
            foreach (var field in @class.Fields)
            {
                var fieldEntry = Template.CppFile.FieldEntry;
                var pascalFieldName = field.Name.First().ToString().ToUpper() + field.Name.Substring(1);
                fieldEntry = fieldEntry.Replace("[FieldNamePascal]", pascalFieldName);
                fieldEntry = fieldEntry.Replace("[FieldName]", field.Name);
                fieldEntries.AppendLine(fieldEntry);
            }

            methodBody = methodBody.Replace("[DrawFields]", fieldEntries.ToString());
            builder.AppendLine(methodBody);
        }
        builder.AppendLine("#endif");
        
        File.WriteAllText(EstimateOutputPathInternal(FileName) + ".cpp", builder.ToString());
        
    }
    
    public bool GeneratedFilesExists(string fileName)
    {
        var estimatedFileName = EstimateOutputPathInternal(fileName) + ".cpp";
        return File.Exists(estimatedFileName);
    } 
}