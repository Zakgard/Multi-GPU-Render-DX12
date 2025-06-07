#include "BlurFilter.h"
#include "shaders.h"

// Choose between precompiled compute shader or compile shader at runtime with D3DCompile
//#define BLUR_PRECOMPILED_COMPUTE_SHADER

#ifndef BLUR_PRECOMPILED_COMPUTE_SHADER
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")
#define COMPUTE_SHADER_PATH L"BlurCS.hlsl"
#else
#include "shaders.h"
#endif

BlurFilter::BlurFilter ( ID3D12Device* device, ID3D12DescriptorHeap* pSrvHeap, UINT Width, UINT Height, DXGI_FORMAT Format, int descHandleIndex, D3D12_GPU_DESCRIPTOR_HANDLE gpu, D3D12_CPU_DESCRIPTOR_HANDLE cpu, UINT descIncr) :
    md3dDevice ( device ),
    md3dSrvHeap ( pSrvHeap ),
    mTextureWidth ( Width ),
    mTextureHeight ( Height ),
    mFormat ( Format ),
    mBlurDescriptorHandleIndex ( descHandleIndex ),
    md3dGPUdesc(gpu),
    md3dCPUdesc(cpu),
    descIncr(descIncr)
{
    InitializeComputeRootSignature ( );
    InitializeBlurPSO ( );

    BuildResources ( );
    AllocateDescriptors ( );
}

ID3D12Resource* BlurFilter::Output ( )
{
    return mBlurMap0;
}

void BlurFilter::OnResize ( ImVec2 position, ImVec2 region )
{
    auto NewWidth = static_cast< UINT >(region.x);
    auto NewHeight = static_cast< UINT >(region.y);

    if ( mBlurRegionWidth != NewWidth || mBlurRegionHeight != NewHeight )
    {
        mBlurRegionWidth = NewWidth;
        mBlurRegionHeight = NewHeight;
    }

    auto NewRegionX = static_cast< UINT >(position.x);
    auto NewRegionY = static_cast< UINT >(position.y);

    if ( mBlurPosX != NewRegionX || mBlurPosY != NewRegionY )
    {
        mBlurPosX = NewRegionX;
        mBlurPosY = NewRegionY;
    }
}

void BlurFilter::BuildResources ( )
{
    // Note, compressed formats cannot be used for UAV.  We get error like:
    // ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
    // cannot be bound as an UnorderedAccessView, or cast to a format that
    // could be bound as an UnorderedAccessView.  Therefore this format 
    // does not support D3D11_BIND_UNORDERED_ACCESS.

    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory ( &texDesc, sizeof ( D3D12_RESOURCE_DESC ) );
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Width = mTextureWidth;
    texDesc.Height = mTextureHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES ( D3D12_HEAP_TYPE_DEFAULT );

    auto hr = md3dDevice->CreateCommittedResource ( &heapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS ( &mBlurMap0 ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create mBlurMap0 resource\n" );
        return;
    }

    hr = md3dDevice->CreateCommittedResource ( &heapProperties, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS ( &mBlurMap1 ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create mBlurMap1 resource\n" );
        return;
    }
}

void BlurFilter::AllocateDescriptors ( )
{
    uint32_t handle_increment = md3dDevice->GetDescriptorHandleIncrementSize ( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle = md3dCPUdesc;
    my_texture_srv_cpu_handle.ptr += (descIncr * mBlurDescriptorHandleIndex);

    auto gpuTextureHandle = md3dGPUdesc;
    gpuTextureHandle.ptr += (descIncr * mBlurDescriptorHandleIndex);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle ( gpuTextureHandle );
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle ( my_texture_srv_cpu_handle );

    BuildDescriptors ( cpuHandle, gpuHandle, handle_increment );
}

void BlurFilter::BuildDescriptors ( CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, UINT descriptorSize )
{
    mBlur0CpuSrv = hCpuDescriptor;
    mBlur0CpuUav = hCpuDescriptor.Offset ( 1, descriptorSize );
    mBlur1CpuSrv = hCpuDescriptor.Offset ( 1, descriptorSize );
    mBlur1CpuUav = hCpuDescriptor.Offset ( 1, descriptorSize );

    mBlur0GpuSrv = hGpuDescriptor;
    mBlur0GpuUav = hGpuDescriptor.Offset ( 1, descriptorSize );
    mBlur1GpuSrv = hGpuDescriptor.Offset ( 1, descriptorSize );
    mBlur1GpuUav = hGpuDescriptor.Offset ( 1, descriptorSize );

    BuildDescriptors ( );
}

D3D12_BOX BlurFilter::GetTargetBox ( )
{
    D3D12_BOX srcBox = { };
    srcBox.left = mBlurPosX;
    srcBox.top = mBlurPosY;
    srcBox.front = 0;
    srcBox.right = mBlurPosX + mBlurRegionWidth;
    srcBox.bottom = mBlurPosY + mBlurRegionHeight;
    srcBox.back = 1;

    return srcBox;
}

bool BlurFilter::IsTargetOutOfBounds ( ID3D12Resource* pTargetTexture, D3D12_BOX srcBox )
{
    D3D12_RESOURCE_DESC srcDesc = pTargetTexture->GetDesc ( );

    if ( srcBox.left < 0 || srcBox.top < 0 || srcBox.right > srcDesc.Width || srcBox.bottom > srcDesc.Height || srcBox.right <= srcBox.left || srcBox.bottom <= srcBox.top )
    {
        // If the srcBox is out of bounds, return true       
        return true;
    }

    return false;
}

void BlurFilter::SetBackBufferResource(ID3D12Resource* res)
{
    mBackBuffer = res;
}

void BlurFilter::GetBackBufferResource (IDXGISwapChain4* swapChain, UINT bufferIndex )
{
    CleanupBackBuffer ( );

    HRESULT hr = swapChain->GetBuffer ( bufferIndex, IID_PPV_ARGS ( &mBackBuffer ) );
    if (SUCCEEDED(hr))
    {

    }

    if ( FAILED ( hr ) )
        return;
}

/*ID3D12Resource* BlurFilter::GetBackBufferResource(IDXGISwapChain3* swapChain, UINT bufferIndex)
{
    CleanupBackBuffer();

    HRESULT hr = swapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&mBackBuffer));
    if (FAILED(hr))
        return nullptr;

    return mBackBuffer;
}*/

void BlurFilter::Execute ( ID3D12GraphicsCommandList* cmdList, ID3D12Resource* inputTexture,
    ID3D12Resource* outputTexture,
    int blurCount)
{
    if ( cmdList == nullptr ||
        inputTexture == nullptr ||
        blurCount < 0 || blurCount > 40)
    {
        return;
    }

    // Проверка объектов состояния пайплайна
    ID3D12PipelineState* horzBlurPso = mPSOs[HorzBlurPso];
    if (horzBlurPso == nullptr)
        return;

    ID3D12PipelineState* verBlurPso = mPSOs[VertBlurPso];
    if (verBlurPso == nullptr)
        return;

    // Проверка области размытия
    auto srcBox = GetTargetBox();
    if (IsTargetOutOfBounds(inputTexture, srcBox))
        return;

    // Инициализация настроек константного буфера (однократно)
    static std::unique_ptr<CBSettings> cbSettings = nullptr;
    if (!cbSettings)
    {
        cbSettings = std::make_unique<CBSettings>(mSigma, 50, 100);
    }

    cbSettings->gBlurStart[0] = mBlurPosX;
    cbSettings->gBlurStart[1] = mBlurPosY;

    // Установка корневой сигнатуры и констант
    cmdList->SetComputeRootSignature(mComputeRootSignature);
    cmdList->SetComputeRoot32BitConstants(0, sizeof(CBSettings) / 4, cbSettings.get(), 0);

    // Определение исходной и целевой локации для копирования
    D3D12_TEXTURE_COPY_LOCATION inputTextureLocation = {};
    inputTextureLocation.pResource = inputTexture;
    inputTextureLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    inputTextureLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION blurMapLocation = {};
    blurMapLocation.pResource = mBlurMap0;
    blurMapLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    blurMapLocation.SubresourceIndex = 0;

    // Переход ресурсов в нужные состояния перед копированием
    auto inputTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        inputTexture,
        D3D12_RESOURCE_STATE_COMMON, // Предполагаем, что входная текстура в состоянии COMMON
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    cmdList->ResourceBarrier(1, &inputTransition);

    auto blurMap0Transition = CD3DX12_RESOURCE_BARRIER::Transition(
        mBlurMap0,
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &blurMap0Transition);

    // Копирование области из входной текстуры в mBlurMap0
    cmdList->CopyTextureRegion(&blurMapLocation, mBlurPosX, mBlurPosY, 0, &inputTextureLocation, &srcBox);

    // Переход ресурсов для вычислений
    {
        auto inputTransitionBack = CD3DX12_RESOURCE_BARRIER::Transition(
            inputTexture,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_COMMON); // Возвращаем в исходное состояние
        cmdList->ResourceBarrier(1, &inputTransitionBack);

        auto blurMap0Transition = CD3DX12_RESOURCE_BARRIER::Transition(
            mBlurMap0,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ);
        cmdList->ResourceBarrier(1, &blurMap0Transition);

        auto blurMap1Transition = CD3DX12_RESOURCE_BARRIER::Transition(
            mBlurMap1,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        cmdList->ResourceBarrier(1, &blurMap1Transition);
    }

    // Цикл размытия
    for (int i = 0; i < blurCount; ++i)
    {
        // Горизонтальный проход
        {
            cmdList->SetPipelineState(horzBlurPso);
            cmdList->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);
            cmdList->SetComputeRootDescriptorTable(2, mBlur1GpuUav);

            // Запуск вычислений для горизонтального размытия
            UINT numGroupsX = (UINT)ceilf(mBlurRegionWidth / 256.f);
            cmdList->Dispatch(numGroupsX, mBlurRegionHeight, 1);

            // Переход ресурсов
            auto blurMap0Transition = CD3DX12_RESOURCE_BARRIER::Transition(
                mBlurMap0,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdList->ResourceBarrier(1, &blurMap0Transition);

            auto blurMap1Transition = CD3DX12_RESOURCE_BARRIER::Transition(
                mBlurMap1,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_GENERIC_READ);
            cmdList->ResourceBarrier(1, &blurMap1Transition);
        }

        // Вертикальный проход
        {
            cmdList->SetPipelineState(verBlurPso);
            cmdList->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);
            cmdList->SetComputeRootDescriptorTable(2, mBlur0GpuUav);

            // Запуск вычислений для вертикального размытия
            UINT numGroupsY = (UINT)ceilf(mBlurRegionHeight / 256.f);
            cmdList->Dispatch(mBlurRegionWidth, numGroupsY, 1);

            // Переход ресурсов
            auto blurMap0Transition = CD3DX12_RESOURCE_BARRIER::Transition(
                mBlurMap0,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_GENERIC_READ);
            cmdList->ResourceBarrier(1, &blurMap0Transition);

            auto blurMap1Transition = CD3DX12_RESOURCE_BARRIER::Transition(
                mBlurMap1,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdList->ResourceBarrier(1, &blurMap1Transition);
        }
    }

    // Переход ресурсов для копирования результата
    {
        auto blurMap0Transition = CD3DX12_RESOURCE_BARRIER::Transition(
            mBlurMap0,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            D3D12_RESOURCE_STATE_COPY_SOURCE);
        cmdList->ResourceBarrier(1, &blurMap0Transition);

        auto outputTransition = CD3DX12_RESOURCE_BARRIER::Transition(
            outputTexture,
            D3D12_RESOURCE_STATE_COMMON, // Предполагаем, что выходная текстура в состоянии COMMON
            D3D12_RESOURCE_STATE_COPY_DEST);
        cmdList->ResourceBarrier(1, &outputTransition);
    }

    // Копирование размытой области в выходную текстуру
    D3D12_TEXTURE_COPY_LOCATION outputTextureLocation = {};
    outputTextureLocation.pResource = outputTexture;
    outputTextureLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    outputTextureLocation.SubresourceIndex = 0;

    cmdList->CopyTextureRegion(&outputTextureLocation, mBlurPosX, mBlurPosY, 0, &blurMapLocation, &srcBox);

    // Восстановление состояний ресурсов
    {
        auto outputTransitionBack = CD3DX12_RESOURCE_BARRIER::Transition(
            outputTexture,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON); // Возвращаем в исходное состояние
        cmdList->ResourceBarrier(1, &outputTransitionBack);

        auto blurMap0Transition = CD3DX12_RESOURCE_BARRIER::Transition(
            mBlurMap0,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_COMMON);
        cmdList->ResourceBarrier(1, &blurMap0Transition);

        auto blurMap1Transition = CD3DX12_RESOURCE_BARRIER::Transition(
            mBlurMap1,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON);
        cmdList->ResourceBarrier(1, &blurMap1Transition);
    }
}

void BlurFilter::BuildDescriptors ( )
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    md3dDevice->CreateShaderResourceView ( mBlurMap0, &srvDesc, mBlur0CpuSrv );
    md3dDevice->CreateUnorderedAccessView ( mBlurMap0, nullptr, &uavDesc, mBlur0CpuUav );

    md3dDevice->CreateShaderResourceView ( mBlurMap1, &srvDesc, mBlur1CpuSrv );
    md3dDevice->CreateUnorderedAccessView ( mBlurMap1, nullptr, &uavDesc, mBlur1CpuUav );
}

void BlurFilter::InitializeComputeRootSignature ( )
{
    // Compute stuff
    CD3DX12_DESCRIPTOR_RANGE SrvTable;
    SrvTable.Init ( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1 );
    CD3DX12_DESCRIPTOR_RANGE UavTable;
    UavTable.Init ( D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1 );

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER ComputeSlotRootParameter[5];

    ComputeSlotRootParameter[0].InitAsConstants ( 14, 0 );
    ComputeSlotRootParameter[1].InitAsDescriptorTable ( 1, &SrvTable );
    ComputeSlotRootParameter[2].InitAsDescriptorTable ( 1, &UavTable );
    ComputeSlotRootParameter[3].InitAsShaderResourceView ( 0 );
    ComputeSlotRootParameter[4].InitAsUnorderedAccessView ( 0 );

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC CompRootSigDesc ( 5, ComputeSlotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ID3DBlob* pSerializedRootSig = nullptr;
    HRESULT hr = D3D12SerializeRootSignature ( &CompRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializedRootSig, nullptr );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to serialize root signature\n" );
        return;
    }        

    hr = md3dDevice->CreateRootSignature ( 0, pSerializedRootSig->GetBufferPointer ( ), pSerializedRootSig->GetBufferSize ( ), IID_PPV_ARGS ( &mComputeRootSignature ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create root signature\n" );
        return;
    }
}

void BlurFilter::InitializeBlurPSO ( )
{
#ifndef BLUR_PRECOMPILED_COMPUTE_SHADER
    auto hr = D3DCompileFromFile ( COMPUTE_SHADER_PATH, nullptr, nullptr, "HorzBlurCS", "cs_5_0", 0, 0, &mShaders[HorzBlurCS], nullptr );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to compile HorzBlurCS\n" );
        return;
    }

    hr = D3DCompileFromFile ( COMPUTE_SHADER_PATH, nullptr, nullptr, "VertBlurCS", "cs_5_0", 0, 0, &mShaders[VertBlurCS], nullptr );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to compile VertBlurCS\n" );
        return;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC HorzBlurPsoDesc = {};
    HorzBlurPsoDesc.pRootSignature = mComputeRootSignature;
    HorzBlurPsoDesc.CS = { reinterpret_cast< BYTE* >(mShaders[HorzBlurCS]->GetBufferPointer ( )),mShaders[HorzBlurCS]->GetBufferSize ( ) };
    HorzBlurPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    hr = md3dDevice->CreateComputePipelineState ( &HorzBlurPsoDesc, IID_PPV_ARGS ( &mPSOs[HorzBlurPso] ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create HorzBlurPso from compiled shader\n" );
        return;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC VertBlurPsoDesc = {};
    VertBlurPsoDesc.pRootSignature = mComputeRootSignature;
    VertBlurPsoDesc.CS = { reinterpret_cast< BYTE* >(mShaders[VertBlurCS]->GetBufferPointer ( )),mShaders[VertBlurCS]->GetBufferSize ( ) };
    VertBlurPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    hr = md3dDevice->CreateComputePipelineState ( &VertBlurPsoDesc, IID_PPV_ARGS ( &mPSOs[VertBlurPso] ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create VertBlurPso from compiled shader\n" );
        return;
    }
#else

    D3D12_COMPUTE_PIPELINE_STATE_DESC HorzBlurPsoDesc = {};
    HorzBlurPsoDesc.pRootSignature = mComputeRootSignature;
    HorzBlurPsoDesc.CS = { g_HorzBlur_Shader_ByteCode, sizeof ( g_HorzBlur_Shader_ByteCode ) };
    HorzBlurPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    auto hr = md3dDevice->CreateComputePipelineState ( &HorzBlurPsoDesc, IID_PPV_ARGS ( &mPSOs[HorzBlurPso] ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create HorzBlurPso from pre-compiled shader\n" );
        return;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC VertBlurPsoDesc = {};
    VertBlurPsoDesc.pRootSignature = mComputeRootSignature;
    VertBlurPsoDesc.CS = { g_VertBlur_Shader_ByteCode, sizeof ( g_VertBlur_Shader_ByteCode ) };
    VertBlurPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    hr = md3dDevice->CreateComputePipelineState ( &VertBlurPsoDesc, IID_PPV_ARGS ( &mPSOs[VertBlurPso] ) );
    if ( FAILED ( hr ) )
    {
        printf ( "[!] Failed to create VertBlurPso from pre-compiled shader\n" );
        return;
    }
#endif
}

void BlurFilter::CleanupBackBuffer ( )
{
    if ( mBackBuffer )
    {
        mBackBuffer->Release ( );
        mBackBuffer = nullptr;
    }
}

void BlurFilter::CleanupBlurMap ( )
{
    if ( mBlurMap0 )
    {
        mBlurMap0->Release ( );
        mBlurMap0 = nullptr;
    }

    if ( mBlurMap1 )
    {
        mBlurMap1->Release ( );
        mBlurMap1 = nullptr;
    }

    if ( mShaders[VertBlurCS] )
    {
        mShaders[VertBlurCS]->Release ( );
        mShaders[VertBlurCS] = nullptr;
    }

    if ( mShaders[HorzBlurCS] )
    {
        mShaders[HorzBlurCS]->Release ( );
        mShaders[HorzBlurCS] = nullptr;
    }

    if ( mPSOs[HorzBlurPso] )
    {
        mPSOs[HorzBlurPso]->Release ( );
        mPSOs[HorzBlurPso] = nullptr;
    }

    if ( mPSOs[VertBlurPso] )
    {
        mPSOs[VertBlurPso]->Release ( );
        mPSOs[VertBlurPso] = nullptr;
    }

    if ( mComputeRootSignature )
    {
        mComputeRootSignature->Release ( );
        mComputeRootSignature = nullptr;

        mWindowResized = true;
    }
}

std::vector<float> CBSettings::CalcGaussWeights ( float sigma )
{
    float twoSigma2 = 2.0f * sigma * sigma;

    int blurRadius = static_cast< int >(ceil ( 2.0f * sigma ));
    const int MaxBlurRadius = 5;  // Define or use the appropriate max blur radius

    if ( blurRadius > MaxBlurRadius )
    {
        blurRadius = MaxBlurRadius;
    }

    std::vector<float> weights ( 2 * blurRadius + 1 );
    float weightSum = 0.0f;

    for ( int i = -blurRadius; i <= blurRadius; ++i )
    {
        float x = static_cast< float >(i);
        weights[i + blurRadius] = expf ( -x * x / twoSigma2 );
        weightSum += weights[i + blurRadius];
    }

    // Normalize weights
    for ( float& weight : weights )
    {
        weight /= weightSum;
    }

    return weights;
}
