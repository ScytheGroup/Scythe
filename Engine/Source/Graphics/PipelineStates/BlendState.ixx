export module Graphics:BlendState;
import Common;
import :Device;

enum class BlendConstant { ZERO, ONE };
enum class BlendType { COLOR, INV_COLOR, ALPHA, INV_ALPHA };
enum class BlendOrigin { SOURCE, DESTINATION };
enum class BlendOperation
{
    ADD = D3D11_BLEND_OP_ADD,
    SUBSTRACT = D3D11_BLEND_OP_SUBTRACT,
    REVERSE_SUBSTRACT = D3D11_BLEND_OP_REV_SUBTRACT,
    MINIMUM = D3D11_BLEND_OP_MIN,
    MAXIMUM = D3D11_BLEND_OP_MAX
};

struct BlendOption
{
    D3D11_BLEND blend;
    constexpr BlendOption(BlendConstant constant);
    constexpr BlendOption(BlendOrigin origin, BlendType type);
};

struct BlendFormula
{
    D3D11_BLEND srcBlend;
    D3D11_BLEND destBlend;
    D3D11_BLEND_OP blendOp;
    constexpr BlendFormula(BlendOption sourceBlendOptions, BlendOperation blendOperation, BlendOption destBlendOptions);

};

export class BlendState
{
public:
    BlendState() = default;
    BlendState(const BlendState&) = default;
    BlendState(BlendState&&) = default;
    BlendState& operator=(const BlendState&) = default;
    BlendState& operator=(BlendState&&) = default;
    
    BlendState(Device& device, BlendFormula colorBlending, BlendFormula alphaBlending, uint8_t blendMask = D3D11_COLOR_WRITE_ENABLE_ALL);
private:
    ComPtr<ID3D11BlendState> blendState;
    
    friend class Device;
};

struct BlendPreset
{
    BlendFormula color;
    BlendFormula alpha;
};

export struct BlendStates
{
    template<BlendPreset Preset>
    static BlendState& Get(Device& device)
    {
        static BlendState bs{device, Preset.color, Preset.alpha };
        return bs;
    }
};

constexpr BlendOption::BlendOption(BlendConstant constant)
    : blend{ constant == BlendConstant::ONE ? D3D11_BLEND_ONE : D3D11_BLEND_ZERO }
{
}

constexpr BlendOption::BlendOption(BlendOrigin origin, BlendType type)
{
    if (origin == BlendOrigin::SOURCE)
    {
        switch (type)
        {
        case BlendType::COLOR:
            blend = D3D11_BLEND_SRC_COLOR;
            break;
        case BlendType::INV_COLOR:
            blend = D3D11_BLEND_INV_SRC_COLOR;
            break;
        case BlendType::ALPHA:
            blend = D3D11_BLEND_SRC_ALPHA;
            break;
        case BlendType::INV_ALPHA:
            blend = D3D11_BLEND_INV_SRC_ALPHA;
            break;
        }
    }
    else // DESTINATION
    {
        switch (type)
        {
        case BlendType::COLOR:
            blend = D3D11_BLEND_DEST_COLOR;
            break;
        case BlendType::INV_COLOR:
            blend = D3D11_BLEND_INV_DEST_COLOR;
            break;
        case BlendType::ALPHA:
            blend = D3D11_BLEND_DEST_ALPHA;
            break;
        case BlendType::INV_ALPHA:
            blend = D3D11_BLEND_INV_DEST_ALPHA;
            break;
        }
    }
}

constexpr BlendFormula::BlendFormula(BlendOption sourceBlendOptions, BlendOperation blendOperation, BlendOption destBlendOptions)
{
    srcBlend = sourceBlendOptions.blend;
    destBlend = destBlendOptions.blend;
    blendOp = static_cast<D3D11_BLEND_OP>(blendOperation); 
}

export namespace BlendPresets
{

constexpr BlendPreset Opaque = {
    { { BlendConstant::ONE }, BlendOperation::ADD, { BlendConstant::ZERO } },
    { { BlendConstant::ONE }, BlendOperation::ADD, { BlendConstant::ZERO } }
};

constexpr BlendPreset Alpha = {
    // color = src.rgb * src.a + dst.rbg * (1 - src.a)
    { { BlendOrigin::SOURCE, BlendType::ALPHA,  }, BlendOperation::ADD, { BlendOrigin::SOURCE, BlendType::INV_ALPHA } },
    // alpha = src.a * 1 + dst.a + 1
    { { BlendConstant::ONE }, BlendOperation::ADD, { BlendConstant::ONE } }
};

constexpr BlendPreset PreMultAlpha{
    // color = src.rgb * src.a + dst.rbg * (1 - src.a)
    { {  BlendOrigin::SOURCE, BlendType::ALPHA, }, BlendOperation::ADD, { BlendOrigin::SOURCE, BlendType::INV_ALPHA } },
    // alpha = src.a * (1 - dst.a) + dst.a + 1 
    { { BlendOrigin::DESTINATION, BlendType::INV_ALPHA,  }, BlendOperation::ADD, { BlendConstant::ONE } }
};

}

