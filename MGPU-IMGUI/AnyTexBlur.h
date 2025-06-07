#pragma once
#include <d3d12.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

class AnyTexBlur {
public:
    AnyTexBlur(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, UINT descriptorSize);
    ~AnyTexBlur();

    void BlurTexture(
        ID3D12GraphicsCommandList* cmdList,
        ID3D12Resource* inputTexture,
        ID3D12Resource* outputTexture,
        int blurCount);

private:
    void InitializeComputeRootSignature();
    void InitializeBlurPSO();
    void BuildDescriptors(ID3D12Resource* inputTexture, ID3D12Resource* outputTexture);

    ID3D12Device* md3dDevice;
    ID3D12DescriptorHeap* md3dSrvHeap;
    UINT mDescriptorSize;

    ComPtr<ID3D12RootSignature> mComputeRootSignature;
    ComPtr<ID3D12PipelineState> mHorzBlurPso;
    ComPtr<ID3D12PipelineState> mVertBlurPso;

    // Дескрипторы
    D3D12_CPU_DESCRIPTOR_HANDLE mInputSrvCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE mInputSrvGpu;
    D3D12_CPU_DESCRIPTOR_HANDLE mOutputUavCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE mOutputUavGpu;

    // Промежуточная текстура для пинг-понга
    ComPtr<ID3D12Resource> mTempBlurMap;
};
