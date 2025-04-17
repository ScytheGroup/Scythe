module;

#include "Graphics/Utils/EventScopeMacros.h"

module Graphics:RenderPass;

import :RenderPass;
import :ResourceManager;
import :ComputeShaderHelper;
import :Scene;

import Components;
import Core;
import Graphics;

#if VXGI_ENABLED

Vector3 VoxelizationPass::GetVoxelBoundsCenter() const
{
    return (data.maxBounds + data.minBounds) / 2.0f;
}

Vector3 VoxelizationPass::GetVoxelBoundsSize() const
{
    return (data.maxBounds - data.minBounds);
}

#ifdef EDITOR

void VoxelizationPass::ShowDebugVoxels(GraphicsScene& scene)
{
    scene.device.SetFrameBuffer(scene.sceneColorBuffer);
    
    scene.device.SetGeometryShader(*voxelisationDebugGeomShader);
    scene.device.SetPixelShader(*voxelisationDebugPixelShader);
    scene.device.SetVertexShader(*voxelisationDebugVertexShader);

    scene.device.SetInputLayout({});

    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(scene.device));

    int voxelSlot = voxelisationDebugGeomShader->GetStructuredBufferSlot("Voxels");
    scene.device.SetShaderResource(voxelSlot, voxelData, GEOMETRY_SHADER);

    // int normalsSlot = voxelisationDebugGeomShader->GetStructuredBufferSlot("Normals");
    // scene.device.SetShaderResource(normalsSlot, &normalData, GEOMETRY_SHADER);

    scene.device.SetRenderTargetsAndDepthStencilView(scene.sceneColorBuffer.GetRTVs(), scene.gBuffer.depthStencil.GetDSV());

    scene.device.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    scene.device.Draw(data.resolution * data.resolution * data.resolution, 0);

    scene.device.UnsetGeometryShader();
    scene.device.UnsetShaderResource(voxelSlot, GEOMETRY_SHADER);
    //scene.device.UnsetShaderResource(normalsSlot, GEOMETRY_SHADER);
}

#endif

void VoxelizationPass::VoxelizeScene(GraphicsScene& scene, EntityComponentSystem& ecs, SceneShadows& shadowsData)
{
    SC_SCOPED_GPU_EVENT("Voxelization");
    
    ResourceManager& rm = ResourceManager::Get();

    dirShadowMap.SetBounds(GetVoxelBoundsCenter(), GetVoxelBoundsSize());
    dirShadowMap.Render(scene, ecs);
    
    scene.device.SetConstantBuffers(VoxelizationDataCBuffer, ViewSpan{ &voxelizationBuffer }, GEOMETRY_SHADER | PIXEL_SHADER | VERTEX_SHADER);
    
    scene.device.SetGeometryShader(*voxelisationGeomShader);
    
    voxelizationBuffer.Update(scene.device, data);

    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::NoCulling>(scene.device));
    
    scene.device.SetRenderTargetsAndUnorderedAccessViews({}, nullptr, { &voxelData });
    
    Array<std::tuple<GlobalTransform*, MeshComponent*>> meshInstances = ecs.GetArchetype<GlobalTransform, MeshComponent>();
    for (auto [transform, meshInstance] : meshInstances)
    {
        scene.device.SetVertexBuffer<GenericVertex>(0, &meshInstance->mesh->geometry->vertexBuffer);
        scene.device.SetIndexBuffer(meshInstance->mesh->geometry->indexBuffer);
        
        for (auto subMesh : meshInstance->mesh->subMeshes)
        {
            AssetRef<Material> material = meshInstance->mesh->GetMaterial(subMesh.materialIndex, meshInstance->materialOverrides);
            if (!material.HasAsset() || !material->IsLoaded())
                continue;

            const ShadingModel& shadingModel = *rm.shadingModelStore.GetShadingModel(ShadingModelStore::VOXELIZATION, material);
            
            if (dirShadowMap.shadowMap)
            {
                scene.device.SetShaderResource(shadingModel.pixelShader->GetTextureSlot("DirectionalProjectedShadowMap"), *dirShadowMap.shadowMap, PIXEL_SHADER);
                scene.device.SetConstantBuffers(shadingModel.pixelShader->GetConstantBufferSlot("DirectionalProjectedData"), ViewSpan{ &dirShadowMap.viewProjMatrixBuffer }, PIXEL_SHADER);
            }
            
            if (shadowsData.pointShadowMaps)
                scene.device.SetShaderResource(shadingModel.pixelShader->GetTextureSlot("PointShadowMaps"), *shadowsData.pointShadowMaps, PIXEL_SHADER);
            if (shadowsData.pointLightData)
                scene.device.SetShaderResource(shadingModel.pixelShader->GetStructuredBufferSlot("PointLightDataBuffer"), *shadowsData.pointLightData, PIXEL_SHADER);
            if (shadowsData.spotShadowMaps)
                scene.device.SetShaderResource(shadingModel.pixelShader->GetTextureSlot("SpotShadowMaps"), *shadowsData.spotShadowMaps, PIXEL_SHADER);
            if (shadowsData.pointShadowMaps)
                scene.device.SetShaderResource(shadingModel.pixelShader->GetStructuredBufferSlot("SpotLightMatrices"), *shadowsData.spotLightViewMatrices, PIXEL_SHADER);

            scene.device.SetConstantBuffers(Graphics::FrameCBufferIndex, ViewSpan{ &scene.frameDataBuffer }, VERTEX_SHADER | PIXEL_SHADER);
            scene.device.SetShaderResource(Graphics::PunctualLightSRVIndex, scene.lightDataBuffer, VERTEX_SHADER | PIXEL_SHADER);
   
            Graphics::DrawMesh(scene.device, transform->transform, *meshInstance->mesh, subMesh, *material, shadingModel);
        }
    }

    scene.device.UnsetRenderTargets();
    scene.device.UnsetGeometryShader();
}

void VoxelizationPass::BuildIlluminationVolume(GraphicsScene& scene, StructuredBuffer<uint32_t>& source)
{
    SC_SCOPED_GPU_EVENT("Build Illumination Volume");
    
    scene.device.SetConstantBuffers(VoxelizationDataCBuffer, ViewSpan{ &voxelizationBuffer }, COMPUTE_SHADER);

    int voxelsSlot = clearVoxelsShader->GetStructuredBufferSlot("Voxels");
    scene.device.SetShaderResource(voxelsSlot, source, COMPUTE_SHADER);

    int illumSlot = illumVolumeShader->GetTextureSlot("IlluminationVolume");
    scene.device.SetUnorderedAccessView(illumSlot, *illuminationVolume);

    scene.device.SetComputeShader(*illumVolumeShader);
    
    auto [x, y, z] = GetThreadCountsForComputeSize(UIntVector3{ data.resolution, data.resolution, data.resolution }, UIntVector3{ 8, 8, 8 });
    scene.device.Dispatch(x, y, z);

    illuminationVolume->GenerateMips(scene.device);

    scene.device.UnsetUnorderedAccessView(illumSlot);
    scene.device.UnsetShaderResource(voxelsSlot, COMPUTE_SHADER);
}

void VoxelizationPass::ClearVoxels(GraphicsScene& scene)
{
    SC_SCOPED_GPU_EVENT("Clear Voxels");

    scene.device.SetConstantBuffers(VoxelizationDataCBuffer, ViewSpan{ &voxelizationBuffer }, COMPUTE_SHADER);
    
    scene.device.SetComputeShader(*clearVoxelsShader);
    
    int voxelSlot = clearVoxelsShader->GetStructuredBufferSlot("Voxels");
    scene.device.SetUnorderedAccessView(voxelSlot, voxelData);
    
    // int normalsSlot = clearVoxelsShader->GetStructuredBufferSlot("Normals");
    // scene.device.SetUnorderedAccessView(normalsSlot, &normalData);
    
    auto [x, y, z] = GetThreadCountsForComputeSize(UIntVector3{ data.resolution, data.resolution, data.resolution }, UIntVector3{ 8, 8, 8 });
    scene.device.Dispatch(x, y, z);
    
    scene.device.UnsetUnorderedAccessView(voxelSlot);
    //scene.device.UnsetUnorderedAccessView(normalsSlot);
}

void VoxelizationPass::ComputeLightBounce(GraphicsScene& scene, StructuredBuffer<uint32_t>& destBuffer)
{
    SC_SCOPED_GPU_EVENT("Compute Light Bounce");
    
    scene.device.SetComputeShader(*bouncesShader);

    scene.device.SetConstantBuffers(bouncesShader->GetConstantBufferSlot("BounceInformation"), ViewSpan{ &bounceInfoBuffer }, COMPUTE_SHADER);

    int volumeSlot = bouncesShader->GetTextureSlot("IlluminationVolume");
    scene.device.SetShaderResource(volumeSlot, *illuminationVolume, COMPUTE_SHADER);

    // int normalsSlot = bouncesShader->GetStructuredBufferSlot("NormalsBuffer");
    // scene.device.SetShaderResource(normalsSlot, &normalData, COMPUTE_SHADER);

    int destSlot = bouncesShader->GetStructuredBufferSlot("DestinationBuffer");
    scene.device.SetUnorderedAccessView(destSlot, destBuffer);

    auto [x, y, z] = GetThreadCountsForComputeSize(UIntVector3{ data.resolution, data.resolution, data.resolution }, UIntVector3{ 8, 8, 8 });
    scene.device.Dispatch(x, y, z);

    scene.device.UnsetUnorderedAccessView(destSlot);
    scene.device.UnsetShaderResource(volumeSlot, COMPUTE_SHADER);
    //scene.device.UnsetShaderResource(normalsSlot, COMPUTE_SHADER);
}

void VoxelizationPass::ComputeLightBounces(GraphicsScene& scene)
{
    for (int i = 0; i < numAdditionnalBounces; ++i)
    {
        bounceInfoBuffer.Update(scene.device, i);
        ComputeLightBounce(scene, voxelData);
        BuildIlluminationVolume(scene, voxelData);
    }
}

void VoxelizationPass::Init(Device& device)
{
    ResourceManager& rm = ResourceManager::Get();
    
    voxelisationGeomShader = rm.shaderLoader.Load<GeometryShader>({"Voxelization.gs.hlsl"}, device);
    voxelisationDebugGeomShader = rm.shaderLoader.Load<GeometryShader>({"VoxelizationDebug.gs.hlsl"}, device);
    voxelisationDebugPixelShader = rm.shaderLoader.Load<PixelShader>({"VoxelizationDebug.ps.hlsl"}, device);
    voxelisationDebugVertexShader = rm.shaderLoader.Load<VertexShader>({"VoxelizationDebug.vs.hlsl"}, device);
    clearVoxelsShader = rm.shaderLoader.Load<ComputeShader>({"ClearVoxels.cs.hlsl"}, device);
    illumVolumeShader = rm.shaderLoader.Load<ComputeShader>({"BuildIlluminationVolume.cs.hlsl"}, device);
    bouncesShader = rm.shaderLoader.Load<ComputeShader>({"VoxelBounces.cs.hlsl"}, device);
    
    std::vector<uint32_t> emptyVoxelData(data.resolution * data.resolution * data.resolution, {});
    voxelData = StructuredBuffer(device, emptyVoxelData, true);

    //std::vector<Vector2> emptyVoxelNormalData(data.resolution * data.resolution * data.resolution, {});
    //normalData = StructuredBuffer(device, emptyVoxelNormalData, true);

    auto desc = Texture::Description::CreateTexture3D("Voxel Illumination Volume", data.resolution, data.resolution, data.resolution, DXGI_FORMAT_R16G16B16A16_FLOAT, Texture::TEXTURE_UAV | Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, true);
    illuminationVolume = Texture(desc, device);

    dirShadowMap.Init(device);
}

void VoxelizationPass::Render(GraphicsScene& scene, EntityComponentSystem& ecs, SceneShadows& shadowData)
{
    SC_SCOPED_GPU_EVENT("Voxelization Pass");

    scene.device.UnsetRenderTargets();

#ifdef EDITOR
    if (shouldRecomputeVoxels)
    {
        ClearVoxels(scene);

        VoxelizeScene(scene, ecs, shadowData);
        BuildIlluminationVolume(scene, voxelData);

        ComputeLightBounces(scene);
    }
    
    if (showVoxels)
        ShowDebugVoxels(scene);
    
    if (showBounds)
        DebugShapes::AddDebugCubeFullExtents((data.minBounds + data.maxBounds) / 2.0f, data.maxBounds - data.minBounds, Colors::Cyan);
#endif
    
    scene.device.CleanupDraw();
    scene.device.UnsetRenderTargets();
}

Texture* VoxelizationPass::GetIlluminationVolume()
{
    if (!illuminationVolume)
        return nullptr;
    
    return &*illuminationVolume;
}

#ifdef IMGUI_ENABLED

import ImGui;

bool VoxelizationPass::ImGuiDraw()
{
    bool result = false;
#ifdef EDITOR
    result |= ImGui::Checkbox("Recompute Voxels", &shouldRecomputeVoxels);
    result |= ImGui::Checkbox("Show Debug", &showVoxels);
    result |= ImGui::Checkbox("Show Bounds", &showBounds);
#endif
    
    ImGui::Separator();
    
    ImGui::Text(std::format("Resolution: {}", data.resolution).c_str());
    result |= ImGui::DragFloat3("Min Bounds", &data.minBounds.x);
    result |= ImGui::DragFloat3("Max Bounds", &data.maxBounds.x);
    
    result |= ImGui::InputInt("Num Bounces", &numAdditionnalBounces);
    numAdditionnalBounces = std::max(0, numAdditionnalBounces);

    ImGui::Separator();
    
    result |= ImGui::DragFloat("Cone Tracing Steps", &data.coneTracingStep, 0.005, 0.1, 2);
    result |= ImGui::DragFloat("Cone Tracing Angle", &data.coneTracingAngle, 0.1, 0.01, 3.1459);
    result |= ImGui::DragFloat("HDR Packing Factor", &data.HDRPackingFactor, 0.5f, 1, 50);
    result |= ImGui::DragFloat("Max cone tracing distance", &data.maxConeTraceDistance, 1, 1, 100);
    result |= ImGui::DragFloat("Temporal smoothing alpha", &data.temporalUpdateAlpha, 0, 0.05, 1);
    return result;
}

#endif

#endif // VXGI_ENABLED
