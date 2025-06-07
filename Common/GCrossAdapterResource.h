#pragma once
#include <d3d12.h>
#include <string>
#include <memory>
#include "GResource.h"


using namespace PEPEngine;
using namespace Graphics;
using namespace Allocator;
using namespace Utils;


class GCrossAdapterResource
{
    std::shared_ptr<GResource> primeResource;
    std::shared_ptr<GResource> sharedResource;
    ComPtr<ID3D12Heap> crossAdapterResourceHeap[2];

    bool isInit = false;

public:
    bool IsInit() const;

    GCrossAdapterResource() = default;

    GCrossAdapterResource(D3D12_RESOURCE_DESC& desc, const std::shared_ptr<GDevice>& primeDevice,
                          const std::shared_ptr<GDevice>& sharedDevice, const std::wstring& name = L"",
                          D3D12_RESOURCE_FLAGS primeTextureExtraFlags = D3D12_RESOURCE_FLAG_NONE);

    inline const GResource& GetPrimeResource() const noexcept
    {
        return *primeResource.get();
    }

    inline const GResource& GetSharedResource() const noexcept
    {
        return *sharedResource.get();
    }

    void Reset();

    void Resize(UINT newWidth, UINT newHeight);
};
