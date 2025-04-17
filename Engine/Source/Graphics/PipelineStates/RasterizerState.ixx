module;

#include <d3d11.h>

export module Graphics:RasterizerState;

import Common;
import :Device;

enum class Culling
{
    BACKFACE_CULLING = D3D11_CULL_BACK,
    FRONTFACE_CULLING = D3D11_CULL_FRONT,
    NO_CULLING = D3D11_CULL_NONE
};
    
enum class FillMode
{
    SOLID = D3D11_FILL_SOLID,
    WIREFRAME = D3D11_FILL_WIREFRAME
};
    
enum class TriangleWinding
{
    CLOCKWISE,
    COUNTER_CLOCKWISE
};

// For more info, read this: https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
struct DepthBiasInfo
{
    int depthBias{};
    float depthBiasClamp{};
    float slopeScaledDepthBias{};
    bool depthClipEnable{};
};

export struct RasterizerStatePreset
{
    Culling culling;
    FillMode fillmode;
    TriangleWinding winding;
    DepthBiasInfo depthBiasInfo;
};

export class RasterizerState
{
    ComPtr<ID3D11RasterizerState> rasterizerState;
public:
    RasterizerState(Device& device, Culling culling, FillMode fillMode = FillMode::SOLID, TriangleWinding winding = TriangleWinding::COUNTER_CLOCKWISE, const DepthBiasInfo& biasInfo = {});
private:
    friend class Device;
};

export struct RasterizerStates
{
    template<RasterizerStatePreset Preset>
    static RasterizerState& Get(Device& device)
    {
        static RasterizerState rs{device, Preset.culling, Preset.fillmode, Preset.winding, Preset.depthBiasInfo};
        return rs;
    }
};

export namespace RasterizerPresets
{
    constexpr RasterizerStatePreset BackfaceCulling{ Culling::BACKFACE_CULLING, FillMode::SOLID, TriangleWinding::COUNTER_CLOCKWISE };
    constexpr RasterizerStatePreset FrontfaceCulling{ Culling::FRONTFACE_CULLING, FillMode::SOLID, TriangleWinding::COUNTER_CLOCKWISE };
    constexpr RasterizerStatePreset NoCulling{ Culling::NO_CULLING, FillMode::SOLID, TriangleWinding::COUNTER_CLOCKWISE };
    constexpr RasterizerStatePreset NoCullingDirectionalShadows{ Culling::NO_CULLING, FillMode::SOLID, TriangleWinding::COUNTER_CLOCKWISE, DepthBiasInfo{ .depthBias = -1 } }; 
}