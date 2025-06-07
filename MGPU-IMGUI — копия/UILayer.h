#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "GDescriptor.h"

#include "Data.h"
#include "UIPictruresLoader.h"
#include "GTexture.h"
#include "MemoryAllocator.h"
#include "Window.h"
#include "BlurFilter.h"
using namespace PEPEngine;
using namespace Graphics;
using namespace Allocator;
using namespace DirectX;
using namespace Common;

class UILayer
{
    struct Data
    {
        float f;
    };

    GDescriptor srvMemory;
    GDescriptor srvPircutre;
    GDescriptor blurPircutre;
    GDescriptor uavPicture;
    GDescriptor fireTexDesc;

    GDescriptor primeUavDescriptor;
    GDescriptor srvDescriptor;
    GDescriptor secondUavDescriptor;

    GDescriptor skillPicturesDescriptors[SKILL_PICTURES_COUNT];
    GDescriptor uiNavigationPicturesDescriptors[38];

    std::shared_ptr<GTexture> pictureToBlur;

    UINT groupCountWidth{};
    UINT groupCountHeight{};

    GDescriptor descriptorHeap;

    ComputePSO blurPSO;
    GRootSignature blurCloudGeneratedSignature;

    HWND hwnd;
    std::shared_ptr<GDevice> device;
    std::shared_ptr<GTexture> textureResource;

    std::shared_ptr<UIPictruresLoader> loader;

    void SetupRenderBackends();
    void Initialize();
    void SetStyle();
    void DrawGlowingPicture() const;

    int pictureSizeX = 512;
    int pictureSizeY = 4256;

    GResource* my_res;
    GShader blurShader;
    ID3D12Resource* my_textures[200];

    ID3D12Resource* bBackText;
    D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle;
    ID3D12RootSignature* rootSignature;
    std::shared_ptr<BlurFilter> blurFilter;

    std::shared_ptr<Window> mainWindow;

    UINT64 gpuHandlerPtr;
    UINT64 fireTexGpuHandlerPtr;

    ID3D12PipelineState* computePipelineState;

    float fps = 0;

    int blurPasses = 40;

    void InitializeBlurResources();
    void InitCircleBar(float value, float radius, const ImVec2& centrePos, const ImVec4& fillColor, const ImVec4& emptyColor, float thickness);

    void DrawTime(uint16_t mins, uint16_t hours);

    void DrawExpLine(float_t val);

    void DrawBlurShit();
    void DrawLeftBar();
    void DrawLeftBottomPanel();
    void DrawRightBar();
    void DrawRightTopBar();
    void DrawRightMiddlePanel();
    void DrawBottomPanel();

    void Update(const std::shared_ptr<GCommandList>& cmdList);

    float expBarValue = 0.0f;
    float exbBarModifier = 1.0f;
    ImVec2 displaySize;
    ImVec2 blurPos;
    ImVec2 blurSize;
    float paddingX = 10.0f;
    float paddingY = 10.0f;
    float buttonSize = 40.0f;
    float iconSize = 40.0f;
    int intIconSize = 35;
    float barHeight = 60.0f;
    float sizeOffset = 50.0f;
    float bottomPosOffset = -20.0f;
public:
    UILayer(const std::shared_ptr<GDevice>& device, HWND hwnd, std::shared_ptr<Window> wnd);

    ~UILayer();
    void SetTexture();
    void CreateDeviceObject();
    void Invalidate();
    void SetFPS(float FPS);

    void Render(const std::shared_ptr<GCommandList>& cmdList, std::shared_ptr<Window> wnd);

    void ChangeDevice(const std::shared_ptr<GDevice>& device, std::shared_ptr<Window> wn);

    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
