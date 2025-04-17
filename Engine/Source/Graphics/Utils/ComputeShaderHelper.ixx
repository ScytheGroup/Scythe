module;
#include <d3d11.h>
export module Graphics:ComputeShaderHelper;

import Common;

void ValidateThreadPerGroupCount(const UIntVector3& numThreads)
{
    if (numThreads.x >= D3D11_CS_THREAD_GROUP_MAX_X
        || numThreads.y >= D3D11_CS_THREAD_GROUP_MAX_Y
        || numThreads.z >= D3D11_CS_THREAD_GROUP_MAX_Z)
    {
        DebugPrintError("Invalid thread count per group: {}", numThreads);
    }
}

void ValidateDispatchSize(const UIntVector3& dispatchCount)
{
    if (dispatchCount.x >= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION 
        || dispatchCount.y >= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION 
        || dispatchCount.z >= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION )
    {
        DebugPrintError("Dispatch count is too big: {}", dispatchCount);
    }
}

export UIntVector3 GetThreadCountsForComputeSize(const UIntVector3& computeSize, const UIntVector3& numThreads)
{
    ValidateThreadPerGroupCount({ numThreads.x, numThreads.y, 1 });
    
    UIntVector3 dispatchCount{
        (computeSize.x + numThreads.x - 1) / numThreads.x,
        (computeSize.y + numThreads.y - 1) / numThreads.y,
        (computeSize.z + numThreads.z - 1) / numThreads.z,
    };

    ValidateDispatchSize(dispatchCount);

    return dispatchCount;
}

export UIntVector3 GetThreadCountsForComputeSize(const Vector3& computeSize, const UIntVector3& numThreads)
{
    return GetThreadCountsForComputeSize(UIntVector3{
        static_cast<uint32_t>(computeSize.x),
        static_cast<uint32_t>(computeSize.y),
        static_cast<uint32_t>(computeSize.z)
    }, { numThreads.x, numThreads.y, numThreads.z });
}

export UIntVector3 GetThreadCountsForComputeSize(const UIntVector2& pixelSize, const UIntVector2& numThreads)
{
    return GetThreadCountsForComputeSize(UIntVector3{ pixelSize.x, pixelSize.y, 1 }, { numThreads.x, numThreads.y, 1 });
}

export UIntVector3 GetThreadCountsForComputeSize(const Vector2& pixelSize, const UIntVector2& numThreads)
{
    return GetThreadCountsForComputeSize(UIntVector3{
        static_cast<uint32_t>(pixelSize.x),
        static_cast<uint32_t>(pixelSize.y),
        1
    }, { numThreads.x, numThreads.y, 1 });
}


export UIntVector3 GetThreadCountsForComputeSize(uint32_t computeSize, uint32_t numThreads)
{
    ValidateThreadPerGroupCount({ numThreads, 1, 1 });
    
    UIntVector3 dispatchCount{ (computeSize + numThreads - 1) / numThreads, 1, 1 };

    ValidateDispatchSize(dispatchCount);

    return dispatchCount;
}
