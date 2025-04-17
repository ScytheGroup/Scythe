export module Profiling:GPUScope;

import Common;

export struct GPUScope
{
    GPUScope(std::wstring eventName);
    GPUScope(const std::string& eventName);
    ~GPUScope();

    std::wstring name;

private:
    ComPtr<ID3D11Query> beginTimestampQuery;
    ComPtr<ID3D11Query> endTimestampQuery;
    uint8_t scopeLevel;
    int reservedScopeIndex;
};

export struct RawGPUScope
{
    std::wstring name;
    ComPtr<ID3D11Query> beginTimestampQuery;
    ComPtr<ID3D11Query> endTimestampQuery;
    uint8_t scopeLevel;
    bool HasFinished() const noexcept { return !name.empty(); }
};