#include "AnyTexBlur.h"
#include "d3dcompiler.h"
#include <stdexcept>

AnyTexBlur::AnyTexBlur(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, UINT descriptorSize)
    : md3dDevice(device), md3dSrvHeap(srvHeap), mDescriptorSize(descriptorSize)
{
    InitializeComputeRootSignature();
    InitializeBlurPSO();
}

AnyTexBlur::~AnyTexBlur()
{
}

void AnyTexBlur::InitializeComputeRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = 3; // Константы, SRV, UAV
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    D3D12_ROOT_PARAMETER rootParams[3];
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].Constants.Num32BitValues = 4; // w, h, radius, pass
    rootParams[0].Constants.ShaderRegister = 0;
    rootParams[0].Constants.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParams[1].Descriptor.ShaderRegister = 0;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParams[2].Descriptor.ShaderRegister = 0;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootSigDesc.pParameters = rootParams;

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to serialize root signature");
    }

    hr = md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mComputeRootSignature));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create root signature");
    }
}

void AnyTexBlur::InitializeBlurPSO()
{

    // Определение путей к файлам шейдеров
    const WCHAR* horzShaderPath = L"Shaders/HorzBlurCS.hlsl";
    const WCHAR* vertShaderPath = L"Shaders/VertBlurCS.hlsl";

    // Переменные для хранения скомпилированных шейдеров и ошибок
    ComPtr<ID3DBlob> horzShaderBlob;
    ComPtr<ID3DBlob> vertShaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    // Компиляция горизонтального шейдера
    HRESULT hr = D3DCompileFromFile(
        horzShaderPath,        // Путь к файлу шейдера
        nullptr,               // Макросы (не используются)
        nullptr,               // Инклюды (не используются)
        "CSMain",              // Имя входной точки шейдера
        "cs_5_0",              // Целевая версия шейдера (Compute Shader 5.0)
        0,                     // Флаги компиляции (без дополнительных опций)
        0,                     // Флаги эффектов (не используются)
        &horzShaderBlob,       // Указатель на blob с скомпилированным шейдером
        &errorBlob             // Указатель на blob с сообщениями об ошибках
    );
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile horizontal blur shader");
    }

    // Компиляция вертикального шейдера
    hr = D3DCompileFromFile(
        vertShaderPath,        // Путь к файлу шейдера
        nullptr,               // Макросы (не используются)
        nullptr,               // Инклюды (не используются)
        "CSMain",              // Имя входной точки шейдера
        "cs_5_0",              // Целевая версия шейдера (Compute Shader 5.0)
        0,                     // Флаги компиляции (без дополнительных опций)
        0,                     // Флаги эффектов (не используются)
        &vertShaderBlob,       // Указатель на blob с скомпилированным шейдером
        &errorBlob             // Указатель на blob с сообщениями об ошибках
    );
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile vertical blur shader");
    }

    // Создание PSO для горизонтального размытия
    D3D12_COMPUTE_PIPELINE_STATE_DESC horzPsoDesc = {};
    horzPsoDesc.pRootSignature = mComputeRootSignature.Get();
    horzPsoDesc.CS = { horzShaderBlob->GetBufferPointer(), horzShaderBlob->GetBufferSize() };
    hr = md3dDevice->CreateComputePipelineState(&horzPsoDesc, IID_PPV_ARGS(&mHorzBlurPso));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create horizontal blur PSO");
    }

    // Создание PSO для вертикального размытия
    D3D12_COMPUTE_PIPELINE_STATE_DESC vertPsoDesc = {};
    vertPsoDesc.pRootSignature = mComputeRootSignature.Get();
    vertPsoDesc.CS = { vertShaderBlob->GetBufferPointer(), vertShaderBlob->GetBufferSize() };
    hr = md3dDevice->CreateComputePipelineState(&vertPsoDesc, IID_PPV_ARGS(&mVertBlurPso));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertical blur PSO");
    }
}

void AnyTexBlur::BuildDescriptors(ID3D12Resource* inputTexture, ID3D12Resource* outputTexture)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    // Получаем первый доступный дескриптор в куче
    mInputSrvCpu = md3dSrvHeap->GetCPUDescriptorHandleForHeapStart();
    mInputSrvGpu = md3dSrvHeap->GetGPUDescriptorHandleForHeapStart();
    mOutputUavCpu = { mInputSrvCpu.ptr + mDescriptorSize };
    mOutputUavGpu = { mInputSrvGpu.ptr + mDescriptorSize };

    md3dDevice->CreateShaderResourceView(inputTexture, &srvDesc, mInputSrvCpu);
    md3dDevice->CreateUnorderedAccessView(outputTexture, nullptr, &uavDesc, mOutputUavCpu);
}

void AnyTexBlur::BlurTexture(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* inputTexture, ID3D12Resource* outputTexture, int blurCount)
{
    D3D12_RESOURCE_DESC inputDesc = inputTexture->GetDesc();
    UINT width = (UINT)inputDesc.Width;
    UINT height = inputDesc.Height;

    BuildDescriptors(inputTexture, outputTexture);

    cmdList->SetComputeRootSignature(mComputeRootSignature.Get());

    // Переход входной текстуры в состояние SRV
    D3D12_RESOURCE_BARRIER barriers[2];
    barriers[0] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = inputTexture;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    // Переход выходной текстуры в состояние UAV
    barriers[1] = {};
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = outputTexture;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    cmdList->ResourceBarrier(2, barriers);

    // Установка констант (ширина, высота, радиус размытия, проход)
    UINT constants[4] = { width, height, 4, 0 }; // Радиус 4 — примерное значение
    cmdList->SetComputeRoot32BitConstants(0, 4, constants, 0);
    cmdList->SetComputeRootDescriptorTable(1, mInputSrvGpu);
    cmdList->SetComputeRootDescriptorTable(2, mOutputUavGpu);

    // Горизонтальное размытие
    cmdList->SetPipelineState(mHorzBlurPso.Get());
    cmdList->Dispatch((width + 15) / 16, height, 1);

    // Вертикальное размытие (можно добавить промежуточную текстуру для нескольких проходов)
    cmdList->SetPipelineState(mVertBlurPso.Get());
    cmdList->Dispatch(width, (height + 15) / 16, 1);

    // Возвращаем состояния обратно
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(2, barriers);
}