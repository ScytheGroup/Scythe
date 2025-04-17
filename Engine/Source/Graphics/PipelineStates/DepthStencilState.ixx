export module Graphics:DepthStencilState;

import Common;
import :Device;

export struct DepthStencilPreset
{
    D3D11_COMPARISON_FUNC comparisonFunc;
    D3D11_DEPTH_WRITE_MASK depthWriteMask;
};

export class DepthStencilState
{
public:
    DepthStencilState() = default;
    DepthStencilState(DepthStencilState&&) = default;
    DepthStencilState(const DepthStencilState&) = default;
    DepthStencilState& operator=(DepthStencilState&&) = default;
    DepthStencilState& operator=(const DepthStencilState&) = default;

    DepthStencilState(Device& device, D3D11_COMPARISON_FUNC comparisonFunc, D3D11_DEPTH_WRITE_MASK depthWriteMask);
private:
    ComPtr<ID3D11DepthStencilState> depthStencilState;
    
    friend class Device;
};

export struct DepthStencilStates
{
    template<DepthStencilPreset Preset>
    static DepthStencilState& Get(Device& device)
    {
        static DepthStencilState bs{device, Preset.comparisonFunc, Preset.depthWriteMask };
        return bs;
    }
};

export namespace DepthStencilPresets
{
    constexpr DepthStencilPreset LessEqualWriteAll{ D3D11_COMPARISON_LESS_EQUAL, D3D11_DEPTH_WRITE_MASK_ALL };
    constexpr DepthStencilPreset LessEqualNoWrite{ D3D11_COMPARISON_LESS_EQUAL, D3D11_DEPTH_WRITE_MASK_ZERO };
}