module;
#include "../Utils/EventScopeMacros.h"
module Graphics:RenderPass;

import :RenderPass;
import :Scene;
import :Device;
import :Texture;
import :RasterizerState;
import :BlendState;

import Components;
import Core;
import Graphics;

#ifdef IMGUI_ENABLED
import ImGui;

#ifdef EDITOR
import Editor;
import :DebugShapes;
#endif

void ImGuiPass::Render(GraphicsScene& scene, ShaderResourceLoader& shaderLoader)
{
    SC_SCOPED_GPU_EVENT("ImGui");

    ImGuiManager::Get().NewFrame();

#ifdef EDITOR
    DrawEditorImGui(scene);
#endif
    
    shaderLoader.HandleShaderErrors();

    ImGuiManager::Get().DrawImGui();
    
    scene.device.SetFrameBuffer(scene.backBuffer);
    scene.device.SetRasterizerState(RasterizerStates::Get<RasterizerPresets::BackfaceCulling>(scene.device));
    
    ImGuiManager::Get().Render();
}

#ifdef EDITOR

void ImGuiPass::DrawEditorViewport(GraphicsScene& scene)
{
    if (!editor)
        return;

    if (editor->isViewportHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        ImGui::SetNextWindowFocus();
    }
    
    if (ImGui::Begin("Viewport"))
    {
        editor->viewportRect = editor->imGuiManager.GetCurrentWindowBounds();
        editor->isViewportHovered = ImGui::IsWindowHovered();
        if (ImGui::IsWindowFocused())
        {
            editor->UpdateEditorFocus(Editor::EditorWindows::Viewport);
        }
        
        ImGui::GetWindowDrawList()->AddImage(scene.editorBuffer.renderTextures[0].GetSRV().Get(), editor->viewportRect.Min, editor->viewportRect.Max);
        editor->DrawSelectionWidget();
    }

    ImGui::End();
}

void ImGuiPass::DrawEditorImGui(GraphicsScene& scene)
{
    if (!editor)
        return;

    scene.device.SetRenderTargetsAndDepthStencilView(scene.editorBuffer.GetRTVs(), scene.gBuffer.depthStencil.GetDSV());
    
    // Make depth read only
    scene.device.SetDepthStencilState(DepthStencilStates::Get<DepthStencilPresets::LessEqualNoWrite>(scene.device));
    DrawEditorSprites(scene, editor->ecs);
    // Make depth writable again
    scene.device.SetDepthStencilState(DepthStencilStates::Get<DepthStencilPresets::LessEqualWriteAll>(scene.device));

    DrawEditorGizmos(scene, editor->ecs);

    editor->BeginDrawImGui();
    
    editor->DrawEditorWindows();

    DrawEditorViewport(scene);

    editor->EndDrawImGui();
}

void ImGuiPass::DrawEditorSprites(GraphicsScene& scene, EntityComponentSystem& ecsRef)
{
    scene.device.SetBlendState(BlendStates::Get<BlendPresets::Alpha>(scene.device));
    
    if (EditorIcons::AreSpriteIconsLoaded())
    {
        // Draw lights
        {
            Array<std::tuple<PunctualLightComponent*, GlobalTransform*>> punctualLightComponents = ecsRef.GetArchetype<PunctualLightComponent, GlobalTransform>();
            CameraComponent* mainCamera = ecsRef.GetSystem<CameraSystem>().GetActiveCamera();
            GlobalTransform* cameraTransform = ecsRef.GetComponent<GlobalTransform>(mainCamera->GetOwningEntity());

            auto orderedLights = SortLightSpritesBackToFront(CameraSystem::GetViewMatrix(*cameraTransform), punctualLightComponents);

            for (auto [punctualLightComponent, transformComponent] : orderedLights)
            {
                // Draw the editor sprite
                {
                    Transform spriteTransform = *transformComponent;
                    spriteTransform.scale = Vector3::One;
                    spriteTransform.rotation = Quaternion::Identity;
                    if (mainCamera->projection.type == CameraProjection::ProjectionType::PERSPECTIVE)
                    {
                        spriteTransform = MakeTransformScreenSpaceSizedBillboard(spriteTransform, cameraTransform->position, mainCamera->projection.FOV, scene.frameData.screenSize / 40.0f,
                                                                                 scene.frameData.screenSize);
                    }
                    else { spriteTransform = MakeTransformBillboard(spriteTransform, cameraTransform->position); }

                    Graphics::DrawSprite(
                        scene.device, 
                        (punctualLightComponent->type == PunctualLightComponent::LightType::POINT) 
                        ? *EditorIcons::PointLightIcon
                        : *EditorIcons::SpotLightIcon,
                        spriteTransform
                    );
                }
            }
        }

        // Draw cameras
        {
            Array<std::tuple<CameraComponent*, GlobalTransform*>> cameras = ecsRef.GetArchetype<CameraComponent, GlobalTransform>();
            CameraComponent* mainCamera = ecsRef.GetSystem<CameraSystem>().GetActiveCamera();
            GlobalTransform* cameraTransform = ecsRef.GetComponent<GlobalTransform>(mainCamera->GetOwningEntity());

            auto orderedCameras = SortCameraSpritesBackToFront(CameraSystem::GetViewMatrix(*cameraTransform), cameras);

            for (auto [camera, transformComp] : orderedCameras)
            {
                // Don't draw gizmo for the view we are viewing from.
                if (camera == mainCamera) continue;

                // Draw the editor sprite
                {
                    Transform transform = *transformComp;
                    transform.scale = Vector3::One;
                    transform.rotation = Quaternion::Identity;
                    if (mainCamera->projection.type == CameraProjection::ProjectionType::PERSPECTIVE)
                    {
                        transform =
                          MakeTransformScreenSpaceSizedBillboard(transform, cameraTransform->position, mainCamera->projection.FOV, scene.frameData.screenSize / 40.0f, scene.frameData.screenSize);
                    }
                    else { transform = MakeTransformBillboard(transform, cameraTransform->position); }

                    Graphics::DrawSprite(scene.device, *EditorIcons::CameraIcon, transform);
                }
            }
        }
    }
}

void ImGuiPass::DrawEditorGizmos(GraphicsScene& scene, EntityComponentSystem& ecsRef)
{
    CameraComponent* mainCamera = ecsRef.GetSystem<CameraSystem>().GetActiveCamera();
    
    // Draw arrow for all camera components
    {
        Array<std::tuple<CameraComponent*, GlobalTransform*>> cameras = ecsRef.GetArchetype<CameraComponent, GlobalTransform>();

        for (auto [camera, transformComp] : cameras)
        {
            if (camera == mainCamera) continue;

            Transform transform = *transformComp;
            DebugShapes::AddDebugArrow(transform.position, transform.position + 2.0f * transform.GetForwardVector(), Color(1, 1, 1, 1));
        }
    }

    // Draw additional information about the selected entity.
    if (editor->selection.selectedEntity)
    {
        Handle selectedEntityHandle = *editor->selection.selectedEntity;
        GlobalTransform* transformComponent = ecsRef.GetComponent<GlobalTransform>(selectedEntityHandle);
        PunctualLightComponent* punctualLightComponent = ecsRef.GetComponent<PunctualLightComponent>(selectedEntityHandle);

        if (transformComponent && punctualLightComponent)
        {
            if (punctualLightComponent->type == PunctualLightComponent::LightType::POINT) 
            { 
                // Draw debug lines for the light's radius
                DebugShapes::AddDebugSphereRadius(transformComponent->position, punctualLightComponent->attenuationRadius, Color(1, 1, 1, 1)); 
            }
            else if (punctualLightComponent->type == PunctualLightComponent::LightType::SPOT) 
            {
                // Draw debug lines for the light's spot cone
                DebugShapes::AddDebugCone(transformComponent->position, transformComponent->rotation, punctualLightComponent->attenuationRadius, punctualLightComponent->innerAngle,
                                          punctualLightComponent->outerAngle,
                                          Color(1, 1, 1, 1));
            }
        }

        CameraComponent* cameraComponent = ecsRef.GetComponent<CameraComponent>(selectedEntityHandle);

        if (transformComponent && cameraComponent && cameraComponent != mainCamera)
        {
            if (cameraComponent->projection.type == CameraProjection::ProjectionType::PERSPECTIVE) 
            {
                Matrix invViewProjection = (CameraSystem::GetViewMatrix(Transform(*transformComponent)) * CameraSystem::GetProjectionMatrix(cameraComponent->projection)).Invert();
                DebugShapes::AddDebugFrustumPerspective(invViewProjection, Color(1, 1, 1, 1));
            }
        }
    }
}

#endif

void ImGuiPass::Init(Editor* newEditor)
{
    editor = newEditor;
}

#endif