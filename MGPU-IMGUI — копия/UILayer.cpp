#include "UILayer.h"

#include "ComputePSO.h"
#include <utility>
#include "Window.h"
#include "GRootSignature.h"
#include "pch.h"
#include "RenderModeFactory.h"
#include "DirectXTex.h"
#include "GRootSignature.h"
#include "GCommandList.h"
#include "GDescriptorHeap.h"
#include <string>
#include "UIPictruresLoader.h"
#include "imgui_internal.h"
#include "d3dcompiler.h"
#include <fstream>
#include "Data.h"
#include "BlurFilter.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const char* name = "ddd";

LRESULT UILayer::MsgProc(const HWND hwnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
}

void UILayer::SetupRenderBackends()
{
    srvMemory = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources);

    srvPircutre = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources);

    loader = std::make_shared<UIPictruresLoader>(UIPictruresLoader());

    displaySize = ImGui::GetIO().DisplaySize;

    SetTexture();

    //DrawBlurShit();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(this->hwnd);

    ImGui_ImplDX12_Init(device->GetDXDevice().Get(), 1,
                        DXGI_FORMAT_R8G8B8A8_UNORM, srvMemory.GetDescriptorHeap()->GetDirectxHeap(),
                        srvMemory.GetCPUHandle(), srvMemory.GetGPUHandle(), device->GetCommandQueue()->GetD3D12CommandQueue().Get());

  //  ImGuiIO& io = ImGui::GetIO();
   // io.Fonts->AddFontFromFileTTF("Data\\Fonts\\Fanti.ttf", 34);


    // Setup Dear ImGui style
  //  ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
}

void UILayer::Initialize()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    SetStyle();
    //ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //

    SetupRenderBackends();
}

void UILayer::SetStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.3f;
    style.FrameRounding = 2.3f;
    style.ScrollbarRounding = 0;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 4.0f;

    ImGui::StyleColorsDark();
    style.WindowRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.6f; // Полупрозрачный фон
    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // Синяя граница
}

void UILayer::DrawGlowingPicture() const
{
}

void UILayer::CreateDeviceObject()
{
    ImGui_ImplDX12_CreateDeviceObjects();
}

void UILayer::Invalidate()
{
    ImGui_ImplDX12_InvalidateDeviceObjects();
}

void UILayer::SetFPS(float FPS)
{
    fps = FPS;
}

void UILayer::InitializeBlurResources()
{
    
}

void UILayer::InitCircleBar(float value, float radius = 40.0f, const ImVec2& centrePos = ImVec2(0,0), const ImVec4& fillColor = ImVec4(0, 0, 1, 1), const ImVec4& emptyColor = ImVec4(0, 0, 0, 0.5f), float thickness = 6.0f)
{
}

void UILayer::DrawTime(uint16_t mins, uint16_t hours)
{
    std::string strMins = std::to_string(mins);
    const char* cStrMinsValue = strMins.c_str();

    std::string strHrs = std::to_string(hours);
    const char* cStrHrsValue = strHrs.c_str();

    int len = strlen(cStrMinsValue) + strlen(cStrHrsValue) + 2;

    char* finalStr = new char[len];

    snprintf(finalStr, len, "%s:%s", cStrMinsValue, cStrHrsValue);

    ImGui::Text(finalStr);

    delete[] finalStr;
}

void UILayer::DrawExpLine(float_t val)
{

}

void UILayer::DrawBlurShit()
{

}

void UILayer::DrawLeftBar()
{
    ImGui::SetNextWindowPos(ImVec2(paddingX, paddingY));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 250.0f));
    ImGui::Begin("LeftTopPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    // 1.1 Картинка с иконкой персонажа

    ImGui::ImageButton(name, (ImTextureID)(intptr_t)skillPicturesDescriptors[36].GetGPUHandle().ptr, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на реальную текстуру
    ImGui::SameLine();
    // 1.2 Бар ХП
    ImGui::ProgressBar(400, ImVec2(140.0f, 60), "HP");
    // 1.5 Никнейм
    ImGui::Text("%s", L"Zakgar");
    // 1.3 Бар Маны
    ImGui::ProgressBar(400, ImVec2(140.0f, 60), "Mana");
    // 1.4 Уровень
    ImGui::Text("Уровень: %d", 52);
    // 1.6 Список состояний
    for (uint16_t i = 0; i < 5; i++) {
        ImGui::ImageButton(name, (ImTextureID)(intptr_t)skillPicturesDescriptors[36].GetGPUHandle().ptr, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на иконки
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", L"Goida");
        ImGui::SameLine();
    }
    ImGui::End();
}

void UILayer::DrawLeftBottomPanel()
{
    ImGui::SetNextWindowPos(ImVec2(paddingX, displaySize.y - paddingY - buttonSize + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(buttonSize*10+100, buttonSize + 20));
    ImGui::Begin("HotbarLeft", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 10; ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::ImageButton(name, (ImTextureID)(intptr_t)uiNavigationPicturesDescriptors[0].GetGPUHandle().ptr, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на иконки
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Слот %zu", i + 1);
        if (i < 9) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::End();
}

void UILayer::DrawRightBar()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - paddingX - iconSize * 10 - 100, displaySize.y - paddingY - iconSize + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(iconSize*10+100, iconSize + 20));
    ImGui::Begin("HotbarRight", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 10; ++i) {
        ImGui::PushID(static_cast<int>(i + 22));
        ImGui::ImageButton(name, (ImTextureID)(intptr_t)uiNavigationPicturesDescriptors[1].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на иконки
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Слот %zu", i + 1);
        if (i < 9) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::End();
}

void UILayer::DrawRightTopBar()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - paddingX - 200.0f, paddingY));
    ImGui::SetNextWindowSize(ImVec2(400, 450));
    ImGui::Begin("RightTopPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    // 5.1 Миникарта
    ImGui::ImageButton(name, (ImTextureID)(intptr_t)skillPicturesDescriptors[36].GetGPUHandle().ptr, ImVec2(180.0f, 60.0f), ImVec2(0, 0), ImVec2(1, 1)); // Замени на текстуру миникарты
    // 5.2 Текущее время
    ImGui::Text("Время: %s", L"22:34");
    ImGui::End();
}

void UILayer::DrawRightMiddlePanel()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - iconSize - 100, displaySize.y - iconSize * 15));
    ImGui::SetNextWindowSize(ImVec2((iconSize + 10)*2, iconSize * 10 + 100));

    ImGui::Begin("HotbarRightMiddle", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 20; ++i) {
        ImGui::PushID(static_cast<int>(i + 22));
        ImGui::ImageButton(name, (ImTextureID)(intptr_t)uiNavigationPicturesDescriptors[1].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на иконки
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Slot %zu", i + 1);
        if (i % 2 == 0) ImGui::SameLine();
        ImGui::PopID();
    }

    ImGui::End();
}

void UILayer::DrawBottomPanel()
{
    uint16_t skillCounter = 0;

    float centerX = (displaySize.x - 17 * iconSize) / 2.0f;
    ImGui::SetNextWindowPos(ImVec2(centerX, displaySize.y - paddingY - 2 * iconSize - barHeight + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(17 * iconSize, 2 * iconSize + barHeight + sizeOffset));
    ImGui::Begin("CenterPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    // 3.2 Полоска опыта
    
    // 3.3 Ряд иконок-способностей (верхний)
    for (size_t i = 0; i < 15; ++i) {
        ImGui::PushID(static_cast<int>(i + 12));
        ImGui::ImageButton(name, (ImTextureID)(intptr_t)skillPicturesDescriptors[skillCounter].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на иконки
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Способность %zu", i + 1);
        if (i < 14) ImGui::SameLine();
        ImGui::PopID();

        skillCounter++;
    }

    if (exbBarModifier > 0)
    {
        exbBarModifier = expBarValue < 1.0f ? 1.0f : -1.0f;
    }
    else 
    {
        exbBarModifier = expBarValue > 0.0f ? -1.0f : 1.0f;
    }

    expBarValue += 0.01f * exbBarModifier;

    ImGui::ProgressBar(expBarValue, ImVec2(16 * iconSize, 20), "Exp");

    // 3.1 Ряд иконок-способностей (нижний)
    for (size_t i = 0; i < 15; ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::ImageButton(name, (ImTextureID)(intptr_t)skillPicturesDescriptors[skillCounter].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1)); // Замени на иконки
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Способность %zu", i + 1);
        if (i < 14) ImGui::SameLine();
        ImGui::PopID();

        skillCounter++;
    }
    ImGui::End();
}

void UILayer::Update(const std::shared_ptr<GCommandList>& cmdList)
{

}

UILayer::UILayer(const std::shared_ptr<GDevice>& device, const HWND hwnd, std::shared_ptr<Window> wnd) : hwnd(hwnd), device((device))
{
    Initialize();
    CreateDeviceObject();

/*    blurFilter = std::make_shared<BlurFilter>(
        device->GetDXDevice().Get(),
        srvPircutre.GetDescriptorHeap()->GetDirectxHeap(),
        wnd->GetClientWidth(),
        wnd->GetClientHeight(),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        globalCountFrameResources,
        srvPircutre.GetGPUHandle(),
        srvPircutre.GetCPUHandle(),
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    );
    */
}

UILayer::~UILayer()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void UILayer::SetTexture()
{
    for (uint16_t i = 0; i < sizeof(skillPicturesDescriptors) / sizeof(skillPicturesDescriptors[0]); i++)
    {
   //     skillPicturesDescriptors[i].~GDescriptor();
    }

    for (uint16_t i = 0; i < sizeof(uiNavigationPicturesDescriptors) / sizeof(uiNavigationPicturesDescriptors[0]); i++)
    {
  //      uiNavigationPicturesDescriptors[i].~GDescriptor();
    }

    for (uint16_t i = 0; i < SKILL_PICTURES_COUNT; i++)
    {
        skillPicturesDescriptors[i] = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources);
        loader.get()->LoadTextureFromFile(SKILL_PICURES_PLACEMENTS[i], device->GetDXDevice().Get(), skillPicturesDescriptors[i].GetCPUHandle(), &my_textures[i], &pictureSizeX, &pictureSizeY);
    }

    for (uint16_t i = 0; i < NAVIGATION_ICONS_COUNT; i++)
    {
        uiNavigationPicturesDescriptors[i] = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources);
        loader.get()->LoadTextureFromFile(NAVIGATION_ICONS_PLACEMENTS[i], device->GetDXDevice().Get(), uiNavigationPicturesDescriptors[i].GetCPUHandle(), &my_textures[i], &pictureSizeX, &pictureSizeY);
    }
}

void UILayer::ChangeDevice(const std::shared_ptr<GDevice>& device, std::shared_ptr<Window> wnd)
{
    if (this->device == device)
    {
        return;
    }
    this->device = device;

/*    blurFilter->CleanupBackBuffer();
    blurFilter->CleanupBlurMap();

    blurFilter.~shared_ptr();

    srvPircutre.~GDescriptor();

    srvPircutre = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources);

    blurFilter = std::make_shared<BlurFilter>(
        device->GetDXDevice().Get(),
        srvPircutre.GetDescriptorHeap()->GetDirectxHeap(),
        wnd->GetClientWidth(),
        wnd->GetClientHeight(),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        globalCountFrameResources,
        srvPircutre.GetGPUHandle(),
        srvPircutre.GetCPUHandle(),
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    );
    */
    Invalidate();
    SetupRenderBackends();
    CreateDeviceObject();
  //  SetTexture();
}

void UILayer::Render(const std::shared_ptr<GCommandList>& cmdList, std::shared_ptr<Window> wnd)
{ 
    cmdList->SetDescriptorsHeap(&srvMemory);
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    displaySize = ImGui::GetIO().DisplaySize;

    DrawLeftBar();
    DrawLeftBottomPanel();
    DrawRightBar();
    DrawRightTopBar();
    DrawBottomPanel();
    DrawRightMiddlePanel();
 
    // === Рендеринг ImGui ===
    ImGui::ShowDemoWindow();
    ImGui::Render();

  //  blurFilter->OnResize(ImVec2(0.0f, 0.0f), ImVec2(1920, 1080));

 //   auto buffer = blurFilter->GetBackBufferResource(wnd->GetSwapChain().Get(), wnd->GetCurrentBackBufferIndex());
//    blurFilter->Execute(cmdList->GetGraphicsCommandList().Get(), buffer, 40);
    

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList->GetGraphicsCommandList().Get());
}
