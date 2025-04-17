using System.Text.Json;
using System.Text.Json.Serialization;
using ClangSharp.Interop;

namespace ScytheReflector.Models;

public class Class
{
    // Defines what is contained in a class
    public enum Type : uint
    {
        Struct = 0,
        Class
    }

    public record Field
    {
        [JsonInclude]
        public string Type { get; set; } = "";
        [JsonInclude]
        public string Name { get; set; } = "";
    }

    [JsonInclude]
    public bool UsesFastType { get; set; } = false;
    [JsonInclude]
    public Type ClassType { get; set; } = Type.Struct;
    [JsonInclude] 
    public string Name { get; set; } = "";
    [JsonInclude]
    public List<Field> Fields { get; set; } = new();
    [JsonInclude]
    public bool EditorOnly { get; set; } = false;

    [JsonInclude] public string Parent = "";

    [JsonInclude] 
    public List<string> Children = new();
    
    [JsonInclude]

    public bool ManualEditor = false;

    public Class(Type classType, string name)
    {
        ClassType = classType;
        Name = name;
    }

    public Class(CXCursorKind classType, string name)
    {
        Name = name;
        switch (classType)
        {
            case CXCursorKind.CXCursor_StructDecl:
                ClassType = Type.Struct;
                break;
            case CXCursorKind.CXCursor_ClassDecl:
                ClassType = Type.Class;
                break;
        }
    }

    public Class()
    {
        
    }

    public void AddField(string type, string name)
    {
        Fields.Add(new Field { Type = type, Name = name });
    }

    public override string ToString()
    {
        return $"{ClassType} {Name}";
    }

    public string ToJson()
    {
        return JsonSerializer.Serialize(this, JsonSerializerOptions.Default);
    }
}