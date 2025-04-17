module;
// We use macros for compile flags :(
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dcompiler.inl>
module Graphics:Shader;

import :Shader;
import :Device;
import :ResourceManager;
import :InputLayout;
import Common;
import Systems;

Shader::Shader(ShaderKey shaderKey, const std::string& entryPoint, const std::string& target)
    : key{std::move(shaderKey)}
    , entryPoint{ entryPoint }
    , target{ target }
{
    Compile();
}

std::optional<std::string> Shader::Compile()
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;

#ifdef SC_DEV_VERSION
    compileFlags |= D3DCOMPILE_DEBUG;
    compileFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif

    ComPtr<ID3DBlob> tempShaderBlob = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    std::filesystem::path finalPath;
    if (key.absolutePath)
    {
        finalPath = key.filepath;
    }
    else
    {
        finalPath = ProjectSettings::Get().engineRoot / ShadersFolder / key.filepath;
    }

    // Ensure the defines are null-terminated, don't use this outside of here to avoid having to manage this in resourceLoader's hash
    auto defines = key.defines; 
    if (!defines.empty() && (defines.back().Name != nullptr || defines.back().Definition != nullptr))
    {
        defines.push_back({ nullptr, nullptr });
    }

    HRESULT res = D3DCompileFromFile(finalPath.c_str(), defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), target.c_str(), compileFlags, 0, &tempShaderBlob, &errorBlob);

    if (FAILED(res))
    {
        std::string fullError = "Unknown Error...";
        
        const std::string hresultDescription = HRToError(res);
        if (!hresultDescription.empty())
        {
            DebugPrint("HResult: {}", hresultDescription);
        }
        
        if (errorBlob != nullptr)
        {
            fullError = static_cast<const char*>(errorBlob->GetBufferPointer());
            DebugPrint("Error compiling {}: {}", key.filepath.string(), fullError);
        }
        
        DebugPrint("D3D11: Failed to compile shader from file.");
        return fullError;
    }

    GenerateReflection(tempShaderBlob);

    shaderBytecode = std::move(tempShaderBlob);

    return {};
}

int Shader::GetStructuredBufferSlot(const std::string& bufferSlotName)
{
    if (!structuredBufferMapping.contains(bufferSlotName))
    {
        DebugPrint("Slot name {} was not in the reflection", bufferSlotName);
        return 0;
    }

    return structuredBufferMapping[bufferSlotName];
}

int Shader::GetConstantBufferSlot(const std::string& bufferSlotName)
{
    if (!constantBufferMapping.contains(bufferSlotName))
    {
        DebugPrint("Slot name {} was not in the reflection", bufferSlotName);
        return 0;
    }

    return constantBufferMapping[bufferSlotName];
}

int Shader::GetTextureSlot(const std::string& textureSlotName)
{
    if (!textureMapping.contains(textureSlotName))
    {
        DebugPrint("Slot name {} was not in the reflection", textureSlotName);
        return 0;
    }

    return textureMapping[textureSlotName];
}

std::optional<std::string> Shader::RecompileShader(Device&)
{
    return Compile();
}

void Shader::GenerateReflection(const ComPtr<ID3DBlob>& bytecode)
{
    constantBufferMapping.clear();
    textureMapping.clear();

    ID3D11ShaderReflection* reflection;
    chk << D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), IID_ID3D11ShaderReflection, reinterpret_cast<void**>(&reflection));

    D3D11_SHADER_DESC shaderDesc;
    chk << reflection->GetDesc(&shaderDesc);
    
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC desc;
        chk << reflection->GetResourceBindingDesc(i, &desc);

        switch (desc.Type)
        {
        case D3D_SIT_TEXTURE:
        case D3D_SIT_UAV_RWTYPED:
            textureMapping[desc.Name] = desc.BindPoint;
            break;
        case D3D_SIT_CBUFFER:
            constantBufferMapping[desc.Name] = desc.BindPoint;
            break;
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_STRUCTURED:
            structuredBufferMapping[desc.Name] = desc.BindPoint;
            break;
        default:
            break;
        }
    }
}

std::string Shader::GetName()
{
    return key.filepath.filename().string();
}

VertexShader::VertexShader(ShaderKey shaderKey, Device& device)
    : Shader(std::move(shaderKey), "main", "vs_5_0")
{
    VertexShader::RecompileShader(device);
}

std::optional<std::string> VertexShader::RecompileShader(Device& device)
{
    if (auto err = Shader::RecompileShader(device))
        return err;

    vertexShader = device.CreateVertexShader(shaderBytecode);
    if (key.createInputLayout)
        inputLayout = device.CreateInputLayout(*this, InputLayouts::Default);

    return {};
}

PixelShader::PixelShader(ShaderKey shaderKey, Device& device)
    : Shader(std::move(shaderKey), "main", "ps_5_0")
{
    PixelShader::RecompileShader(device);
}

std::optional<std::string> PixelShader::RecompileShader(Device& device)
{ 
    if (auto err = Shader::RecompileShader(device))
        return err;

    pixelShader = device.CreatePixelShader(shaderBytecode);

    return {};
}

ComputeShader::ComputeShader(ShaderKey shaderKey, Device& device)
    : Shader(std::move(shaderKey), "main", "cs_5_0")
{
    ComputeShader::RecompileShader(device);
}

std::optional<std::string> ComputeShader::RecompileShader(Device& device)
{
    if (auto err = Shader::RecompileShader(device))
        return err;

    computeShader = device.CreateComputeShader(shaderBytecode);
    
    return {};
}

GeometryShader::GeometryShader(ShaderKey shaderKey, Device& device)
    : Shader(std::move(shaderKey), "main", "gs_5_0")
{
    GeometryShader::RecompileShader(device);
}

std::optional<std::string> GeometryShader::RecompileShader(Device& device)
{
    if (auto err = Shader::RecompileShader(device))
        return err;

    geometryShader = device.CreateGeometryShader(shaderBytecode);

    return {};
}

void ShaderResourceLoader::RecompileDirty(Device& device)
{
    bool recompileShortcut = InputSystem::IsKeyHeld(Inputs::Keys::LeftControl) && InputSystem::IsKeyPressed(Inputs::Keys::OemPeriod);

    if (recompileShortcut)
    {
        SubscribeAllForRecompile();
    }

    if (recompileList.empty())
        return;

    DebugPrint("Recompiling shaders...");

    for (auto&& shader : recompileList)
    {
        if (auto err = shader->RecompileShader(device))
        {
            SubmitFailedCompilation(shader, *err);
        }
        else
        {
            DebugPrint("Recompiled: {}", shader->key);
        }
    }

    recompileList.clear();
}

void ShaderResourceLoader::SubscribeForRecompile(Shader* shader)
{
    recompileList.push_back(shader);
}

void ShaderResourceLoader::SubmitFailedCompilation(Shader* shader, std::string errMsg)
{
    for (auto& [s, msg] : failedToCompile)
    {
        if (s == shader) 
        { 
            msg = errMsg;
            return;
        }
    }
    failedToCompile.emplace_back(shader, errMsg);
}

void ShaderResourceLoader::SubscribeAllForRecompile()
{
    recompileList.clear();
    for (auto&& shader : cache)
    {
        recompileList.push_back(shader.second.get());
    }
}

#ifdef IMGUI_ENABLED

import ImGui;

bool Shader::ImGuiDraw()
{
    bool result = false;
    if (ImGui::Button("Recompile")) 
    {
        ResourceManager::Get().shaderLoader.SubscribeForRecompile(this);
    }

    ScytheGui::Header( "Defines:");
    if (key.defines.empty()) 
    {
        ImGui::Bullet();
        ImGui::Text("None");
    }
    else
    {
        for (auto&& define : key.defines)
        {
            if (!define.Name || !define.Definition)
                break;

            ImGui::Bullet();
            ImGui::Text(std::format("{}={}", define.Name, define.Definition).c_str());
        }
    }

    ScytheGui::Header( "Constant Buffers:");
    if (constantBufferMapping.empty())
    {
        ImGui::Bullet();
        ImGui::Text("None");
    }
    else
    {
        for (auto&& [name, index] : constantBufferMapping) 
        {
            ImGui::Bullet();
            ImGui::Text(std::format("{}: {}", name, index).c_str());
        }
    }

    ScytheGui::Header("Texture Slots:");
    if (textureMapping.empty())
    {
        ImGui::Bullet();
        ImGui::Text("None");
    }
    else
    {
        for (auto&& [name, index] : textureMapping)
        {
            ImGui::Bullet();
            ImGui::Text(std::format("{}: {}", name, index).c_str());
        }
    }

    return result;
}

bool ShaderResourceLoader::ImGuiDraw()
{
    if (ImGui::Button("Recompile All"))
    {
        SubscribeAllForRecompile();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        ImGui::SetTooltip("Left CTRL+.");
    }

    std::vector<std::pair<const ShaderKey*, Shader*>> shaderMap(cache.size());
    std::ranges::transform(cache, shaderMap.begin(), [](auto&& pair)
    {
        return std::pair{ &pair.first, pair.second.get() }; 
    });

    std::ranges::sort(shaderMap, [](auto&& pairs1, auto&& pairs2)
    {
        return std::format("{}", *pairs1.first) < std::format("{}", *pairs2.first);
    });
    
    int i = 0;
    for (auto&& [key, val] : shaderMap)
    {
        ImGui::PushID(i);
        if (ImGui::TreeNode(std::format("{}", *key).c_str()))
        {
            val->ImGuiDraw();
            ImGui::TreePop();
        }
        ImGui::PopID();

        i++;
    }
    return false;
}

void ShaderResourceLoader::HandleShaderErrors()
{
    if (ScytheGui::BeginModal("Shaders failed to compile", !failedToCompile.empty()))
    {
        ImGui::Text("These shaders failed to compile");
        int i = 0;
        for (auto& [shader, errMsg] : failedToCompile)
        {
            ImGui::PushID(i++);
            if (ImGui::CollapsingHeader(std::format("{}", shader->key).c_str())) { ImGui::TextWrapped(errMsg.c_str()); }
            ImGui::PopID();
        }
        ImGui::Text("Would you like to retry compiling them?");

        if (auto result = ScytheGui::EndModal(ScytheGui::ModalType::YES_NO))
        {
            if (result == ScytheGui::YES) std::ranges::transform(failedToCompile, std::back_inserter(recompileList), [](auto& val) { return val.first; });
            failedToCompile.clear();
        }
    }
}

#endif
