﻿#include "UILayer.h"

#include "ComputePSO.h"
#include "AnyTexBlur.h"
#include <utility>
#include "Window.h"
#include "GResourceStateTracker.h"
#include "dxgiformat.h"
#include <iostream>
#include <clocale>
#include <random>
#include "GCrossAdapterResource.h"
#include "GCrossAdapterResource.h"
#include "d2d1effects.h"
#include "GRootSignature.h"

#include "pch.h"
#include "RenderModeFactory.h"
#include "DirectXTex.h"
#include "GRootSignature.h"
#include "GCommandList.h"
#include "GDescriptorHeap.h"
#include "d2d1_1.h"
#include <string>
#include "UIPictruresLoader.h"
#include "imgui_internal.h"
#include "d3dcompiler.h"
#include <chrono>
#include <fstream>
#include "Data.h"
#include "BlurFilter.h"

uint16_t skillCounter = 0;
static int mod = 1;
int UI_NAVIGATION_ATLAS_SIZE = 4096;
int UI_MAP_SIZE = 800;
static bool isShared = false;
static float currentTime = 0.0f;
static float borderSize = 1.0f;
static int uiDescriptorCounter = 0;
static double timee = 0.016f;
static double deltatime = 0;
static std::vector<ImVec4> baseColors;
static std::vector<ImVec4> endColors;
static float totalTime = 0;
static int currentColorIndex = 0;
static int maxColors = 100;
static float glow_phase = 0.0f;
static bool initialized = false;
static float glow_speed = 0.05f;
static std::wstring currentDeviceName;
static int postDeviceChangeFpsCounter = 0;
static int fpsToInitBlur = 10;
static float gradAnimSpeed = 100.6f;
static auto currentDeltaTime = std::chrono::high_resolution_clock::now();
static auto lastDeltaTime = currentDeltaTime;

ImVec2 pointPos;
bool isMapPointInited = false;
int currentDetailsLevel = 0;
float MinStarCount = 10;
float MaxStarCount = 15;
float hpBarDispathSize = 512;
float hbBarUpgradeMullty = 1.2f;
std::vector<float> glows = std::vector<float>(20);

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

    loader = std::make_shared<UIPictruresLoader>(UIPictruresLoader());

    displaySize = ImGui::GetIO().DisplaySize;
    ImGui_ImplWin32_Init(this->hwnd);

    ImGui_ImplDX12_Init(device->GetDXDevice().Get(), 1,
        DXGI_FORMAT_R8G8B8A8_UNORM, srvMemory.GetDescriptorHeap()->GetDirectxHeap(),
        srvMemory.GetCPUHandle(), srvMemory.GetGPUHandle());
}

void UILayer::Initialize()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    assets = std::make_shared<AssetsLoader>(device);
    SetTexture();
    SetStyle();

    SetupRenderBackends();
}

void UILayer::SetStyle()
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> color(0, 255);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.3f;
    style.FrameRounding = 2.3f;
    style.ScrollbarRounding = 0;
    style.FrameBorderSize = borderSize;
    style.WindowBorderSize = 4.0f;

    ImGui::StyleColorsDark();
    style.WindowRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.4f; // Полупрозрачный фон
    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // Синяя граница

    for (uint16_t i = 0; i < maxColors - 1; i++)
    {
        float r = static_cast<float>(color(rng)) / 255.0f;
        float g = static_cast<float>(color(rng)) / 255.0f;
        float b = static_cast<float>(color(rng)) / 255.0f;
        baseColors.push_back(ImVec4(r, g, b, 0.0f));

        r = static_cast<float>(color(rng)) / 255.0f;
        g = static_cast<float>(color(rng)) / 255.0f;
        b = static_cast<float>(color(rng)) / 255.0f;
        endColors.push_back(ImVec4(r, g, b, 0.5f));
    }

    ImGuiIO& io = ImGui::GetIO();
  //  io.DisplaySize = ImVec2(7680, 4320); // 8K разрешение
    io.DisplayFramebufferScale = ImVec2(16.0f, 16.0f); // Суперсэмплинг
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
    
}

void UILayer::UpHpBarQuality()
{
    MaxStarCount *= hbBarUpgradeMullty;
    MinStarCount *= hbBarUpgradeMullty;
    currentDetailsLevel++;
   // hpBarDispathSize *= hbBarUpgradeMullty;
}

void UILayer::DownHpBarQuality()
{
    MaxStarCount /= hbBarUpgradeMullty;
    MinStarCount /= hbBarUpgradeMullty;
    currentDetailsLevel--;
  //  hpBarDispathSize /= hbBarUpgradeMullty;
}

void UILayer::InitializeBlurResources()
{

}

void UILayer::InitCircleBar(float value, float radius = 40.0f, const ImVec2& centrePos = ImVec2(0, 0), const ImVec4& fillColor = ImVec4(0, 0, 1, 1), const ImVec4& emptyColor = ImVec4(0, 0, 0, 0.5f), float thickness = 6.0f)
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

    ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(36)->GetD3D12Resource()->GetGPUVirtualAddress(), ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на реальную текстуру
    //ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
    currentColorIndex++;
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
        ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(36)->GetD3D12Resource()->GetGPUVirtualAddress(), ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", L"Goida");
        ImGui::SameLine();
    }
    ImGui::End();
}

void UILayer::DrawMapPoint(const ImVec2& size)
{
    if (size.x <= 0 || size.y <= 0) return;

    if (!isMapPointInited)
    {
        // Инициализация генератора случайных чисел
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // Распределение координат в пределах размера миникарты
        std::uniform_real_distribution<float> distX(100.0f, 350.0f);
        std::uniform_real_distribution<float> distY(100.0f, 350.0f);

        // Генерация случайной позиции (относительно окна)
        const float x = distX(gen);
        const float y = distY(gen);

        // Получаем позицию окна для перевода в абсолютные координаты
        const ImVec2 windowPos = ImGui::GetWindowPos();
        pointPos = ImVec2(windowPos.x + x, windowPos.y + y);

        isMapPointInited = true;
    }


    // Рисуем закрашенный кружок (точку)
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddCircleFilled(pointPos, 5.0f, IM_COL32(0, 0, 0, 255));
}

void UILayer::DrawLeftBottomPanel()
{
    ImGui::SetNextWindowPos(ImVec2(paddingX, displaySize.y - paddingY - uiNavSize + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(uiNavSize * 9 + 20, uiNavSize + 20));
    ImGui::Begin("HotbarLeft", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 8; ++i) {
        ImGui::PushID(static_cast<int>(i));
        auto index = assets->GetTextureIndex(L"UI_NAVIGATION_ATLAS");
        ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(index)->GetD3D12Resource()->GetGPUVirtualAddress(), ImVec2(uiNavSize, uiNavSize), ImVec2(0.25f * (i % 4), 0.25f * (i / 4)), ImVec2(0.25f + 0.25f * (i%4), 0.25f + 0.25f * (i / 4)), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(uiNavSize, uiNavSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Слот %zu", i + 1);
        if (i < 9) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::End();
}

void UILayer::DrawRightBar()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - paddingX - iconSize * 11 - 130, displaySize.y - paddingY - uiNavSize + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(uiNavSize * 9 + 20, uiNavSize + 20));
    ImGui::Begin("HotbarRight", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 8; ++i) {
        ImGui::PushID(static_cast<int>(i + 22));
        auto index = assets->GetTextureIndex(L"UI_NAVIGATION_ATLAS");
        ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(index)->GetD3D12Resource()->GetGPUVirtualAddress(), ImVec2(uiNavSize, uiNavSize), ImVec2(0.25f * (i % 4), 0.5f + 0.25f * (i / 4)), ImVec2(0.25f + 0.25f * (i % 4), 0.75f + 0.25f * (i / 4)), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(uiNavSize, uiNavSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Слот %zu", i + 1);
        if (i < 9) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::End();
}

void UILayer::DrawRightTopBar()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - paddingX - 442.0f, paddingY));
    ImGui::SetNextWindowSize(ImVec2(430.0f, 430.0f));
    ImGui::Begin("RightTopPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    // 5.1 Миникарта
    auto index = assets->GetTextureIndex(L"UI_MAP");
    ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(index)->GetD3D12Resource()->GetGPUVirtualAddress(), ImVec2(410.0f, 410.0f), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на текстуру миникарты
    DrawMapPoint(ImVec2(410.0f, 410.0f));
    currentColorIndex++;
    // 5.2 Текущее время
  //  ImGui::Text("Время: %s", L"22:34");
    ImGui::End();
}

void UILayer::DrawRightMiddlePanel()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - iconSize - 90, displaySize.y - iconSize * 15 + 25));
    ImGui::SetNextWindowSize(ImVec2((iconSize + 14) * 2, iconSize * 10 + 52));

    ImGui::Begin("HotbarRightMiddle", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 20; ++i) {
        ImGui::PushID(static_cast<int>(i + 22));
        auto index = assets->GetTextureIndex(std::to_wstring(skillCounter));
        ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(index)->GetD3D12Resource()->GetGPUVirtualAddress(), ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        skillCounter++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Slot %zu", i + 1);
        if (i % 2 == 0) ImGui::SameLine();
        ImGui::PopID();
    }

    ImGui::End();
}

void UILayer::DrawBottomPanel()
{
    float centerX = (displaySize.x - 17 * iconSize) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(centerX, displaySize.y - paddingY - 2 * iconSize - barHeight + bottomPosOffset - 50));
    ImGui::SetNextWindowSize(ImVec2(17 * iconSize, 2 * iconSize + barHeight + sizeOffset + 50));
    ImGui::Begin("CenterPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    // 3.2 Полоска опыта

    // 3.3 Ряд иконок-способностей (верхний)
    for (size_t i = 0; i < 15; ++i) {
        ImGui::PushID(static_cast<int>(i + 12));
        ImGui::BeginGroup();
        {
            // Фоновое изображение (под кнопкой)
              auto index = assets->GetTextureIndex(std::to_wstring(skillCounter));
            ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(index)->GetD3D12Resource()->GetGPUVirtualAddress(),
                ImVec2(iconSize, iconSize),
                ImVec2(0, 0));

            // Кнопка поверх изображения (с отступом или без)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - iconSize - 4.0f); // Смещаем назад на высоту изображения
            ImGui::ImageButton((void*)uiSRVs[skillCounter].GetGPUHandle().ptr,
                ImVec2(iconSize, iconSize),
                ImVec2(0, 0),
                ImVec2(1, 1),
                0);
        }
        ImGui::EndGroup();
        //ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Способность %zu", i + 1);
        if (i < 14) ImGui::SameLine();
        ImGui::PopID();

        skillCounter++;
    }

    ImGui::Image((void*)hbBarSRV.GetGPUHandle().ptr, ImVec2(750, 100), ImVec2(0, 0), ImVec2(1, 1));
    //ApplyGradientAndStarsEffect(ImVec2(750, 200), baseColors[currentColorIndex], endColors[currentColorIndex], 0.05f, 100, 1);

    // 3.1 Ряд иконок-способностей (нижний)
    for (size_t i = 0; i < 15; ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginGroup();
        {
            // Фоновое изображение (под кнопкой)
            auto index = assets->GetTextureIndex(std::to_wstring(skillCounter));
            ImGui::ImageButton((void*)(intptr_t)assets->GetTexture(index)->GetD3D12Resource()->GetGPUVirtualAddress(),
                ImVec2(iconSize, iconSize),
                ImVec2(0, 0),
                ImVec2(1, 1));

            // Кнопка поверх изображения (с отступом или без)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - iconSize - 4.0f); // Смещаем назад на высоту изображения
            ImGui::ImageButton((void*)uiSRVs[skillCounter].GetGPUHandle().ptr,
                ImVec2(iconSize, iconSize),
                ImVec2(0, 0),
                ImVec2(1, 1),
                0);
        }
        ImGui::EndGroup();
        //ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Способность %zu", i + 1);
        if (i < 14) ImGui::SameLine();
        ImGui::PopID();

        skillCounter++;
    }
    ImGui::End();
}

void UILayer::AddColorToButton(float glow)
{

}

void UILayer::BlurIcon(ImVec2 iconPos, ImVec2 size)
{
}

ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device, UINT64 size)
{
    ComPtr<ID3D12Resource> resource;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource));

    return resource;
}

void UILayer::Update(const std::shared_ptr<GCommandList>& cmdList)
{
    GradientNoiseCB par;
    const auto &baseCol = baseColors[uiDescriptorCounter];
    const auto &ednCol = endColors[uiDescriptorCounter];
    par.ColorStart = XMFLOAT4(baseCol.x, baseCol.y, baseCol.z, baseCol.w);
    par.ColorEnd = XMFLOAT4(ednCol.x, ednCol.y, ednCol.z, ednCol.w);
    par.Smoothness = 2.0f;
    par.TextureSize = XMFLOAT2(128, 128);
    par.Time = totalTime;

    par.ColorShiftSpeed = 1.95f; // Скорость перехода
    par.NoiseIntensity = 0.1f; // Слабая текстура

    cmdList->TransitionBarrier(uiTexs[uiDescriptorCounter].GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->FlushResourceBarriers();

    cmdList->SetDescriptorsHeap(&uiUAVs[uiDescriptorCounter]);
    cmdList->SetPipelineState(uiPSOs[uiDescriptorCounter]);

    // cmdList->SetRootConstantBufferView(0, params);
    cmdList->SetRoot32BitConstants(0, sizeof(GradientNoiseCB) / sizeof(float), &par, 0);
    cmdList->SetRootDescriptorTable(1, &uiUAVs[uiDescriptorCounter]);

    uint32_t dispatchX = (128 + 8 - 1) / 8; // 32 groups (256 / 8)
    uint32_t dispatchY = (128 + 8 - 1) / 8; // 32 groups (256 / 8)
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    cmdList->TransitionBarrier(uiTexs[uiDescriptorCounter].GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);
    cmdList->FlushResourceBarriers();

    // device->GetCommandQueue(GQueueType::Compute)->ExecuteCommandList(cmd);

    uiDescriptorCounter++;
}


void UILayer::InitializeGradientResources()
{
    uiSRVs[uiDescriptorCounter] = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    uiUAVs[uiDescriptorCounter] = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    D3D12_RESOURCE_DESC gradientText = {};
    gradientText.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    gradientText.Alignment = 0;
    gradientText.Width = 128;
    gradientText.Height = 128;
    gradientText.DepthOrArraySize = 1;
    gradientText.MipLevels = 1;
    gradientText.Format = rtvFormat;
    gradientText.SampleDesc.Count = 1;
    gradientText.SampleDesc.Quality = 0;
    gradientText.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    gradientText.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    uiTexs[uiDescriptorCounter] = GTexture(device, gradientText, L"Gradied tex");

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = rtvFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.PlaneSlice = 0;
    uavDesc.Texture2D.MipSlice = 0;

    uiTexs[uiDescriptorCounter].CreateUnorderedAccessView(&uavDesc, &uiUAVs[uiDescriptorCounter]);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.MipLevels = 1;

    uiTexs[uiDescriptorCounter].CreateShaderResourceView(&srvDesc, &uiSRVs[uiDescriptorCounter]);

    GRootSignature gradientSignature;

    // 1. Используем 32-bit константы вместо CBV
    gradientSignature.AddConstantParameter(sizeof(GradientNoiseCB) / sizeof(float), 0);  // 12 значений float

    // 2. Создаем диапазон для UAV
    CD3DX12_DESCRIPTOR_RANGE uavRange;
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    // 3. Добавляем UAV таблицу
    gradientSignature.AddDescriptorParameter(&uavRange, 1);

    // 4. Инициализируем
    gradientSignature.Initialize(device);

    uiShaders[uiDescriptorCounter] = GShader(L"Shaders\\GradientCompute.hlsl", ComputeShader, nullptr, "CSMain", "cs_5_1");
    uiShaders[uiDescriptorCounter].LoadAndCompile();

    uiPSOs[uiDescriptorCounter] = ComputePSO();
    uiPSOs[uiDescriptorCounter].SetRootSignature(gradientSignature);
    uiPSOs[uiDescriptorCounter].SetShader(&uiShaders[uiDescriptorCounter]);
    uiPSOs[uiDescriptorCounter].Initialize(device);

    uiDescriptorCounter++;
}

void UILayer::InitializeHPBar()
{
    hbBarSRV = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    hbBarUAV = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

    D3D12_RESOURCE_DESC gradientText = {};
    gradientText.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    gradientText.Alignment = 0;
    gradientText.Width = hpBarDispathSize;
    gradientText.Height = hpBarDispathSize;
    gradientText.DepthOrArraySize = 1;
    gradientText.MipLevels = 1;
    gradientText.Format = rtvFormat;
    gradientText.SampleDesc.Count = 1;
    gradientText.SampleDesc.Quality = 0;
    gradientText.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    gradientText.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hbBarTex = GTexture(device, gradientText, L"HP bar");

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = rtvFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.PlaneSlice = 0;
    uavDesc.Texture2D.MipSlice = 0;

    hbBarTex.CreateUnorderedAccessView(&uavDesc, &hbBarUAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.MipLevels = 1;

    hbBarTex.CreateShaderResourceView(&srvDesc, &hbBarSRV);

    GRootSignature gradientSignature;

    // 1. Используем 32-bit константы вместо CBV
    gradientSignature.AddConstantParameter(sizeof(HPBarCB) / sizeof(float), 0);  // 12 значений float

    // 2. Создаем диапазон для UAV
    CD3DX12_DESCRIPTOR_RANGE uavRange;
    uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    // 3. Добавляем UAV таблицу
    gradientSignature.AddDescriptorParameter(&uavRange, 1);

    // 4. Инициализируем
    gradientSignature.Initialize(device);

    hbBarShader = GShader(L"Shaders\\HPBarComputeShader.hlsl", ComputeShader, nullptr, "CSMain", "cs_5_1");
    hbBarShader.LoadAndCompile();

    hbBarPso = ComputePSO();
    hbBarPso.SetRootSignature(gradientSignature);
    hbBarPso.SetShader(&hbBarShader);
    hbBarPso.Initialize(device);
}

void UILayer::UpdateHBBar(const std::shared_ptr<GCommandList>& cmdList, float value = 0.2f)
{
    HPBarCB par = {};
    const auto &baseCol = baseColors[uiDescriptorCounter];
    const auto &ednCol = endColors[uiDescriptorCounter];
    par.ColorStart = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f); // Зелёный

    const auto endR = 1.0f - 1.0f * value * 11;
    const auto endG = 0.0f + 1.0f * value * 11;

    par.ColorEnd = XMFLOAT4(endR, endG, 0.0f, 1.0f);   // Красный

    par.Health = value * 11 * (1024 / hpBarDispathSize);
    par.TextureSize = XMFLOAT2(hpBarDispathSize, hpBarDispathSize);
    par.Time = totalTime;
    par.SparkleIntensity = 0.0009f;
    par.SparkleSpeed = 0.0008f;
    par.MinStarCount = MinStarCount;
    par.MaxStarCount = MaxStarCount;
    // Область спавна - по всему HP бару
    par.StarPosMin = XMFLOAT2(0.0f, 0.0f);
    par.StarPosMax = XMFLOAT2(value * 10, 1.0f);
    par.DistortionPower = 0.5f;
    par.GlowIntensity = 1.8f;
    par.FractalScale = 3.8f;
    par.FillThreshold = 1.7f;
    par.CrackThickness = 0.005;
    par.CrackRoughness = 0.3;

    cmdList->TransitionBarrier(hbBarTex.GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->FlushResourceBarriers();

    cmdList->SetDescriptorsHeap(&hbBarUAV);
    cmdList->SetPipelineState(hbBarPso);

    // cmdList->SetRootConstantBufferView(0, params);
    cmdList->SetRoot32BitConstants(0, sizeof(HPBarCB) / sizeof(float), &par, 0);
    cmdList->SetRootDescriptorTable(1, &hbBarUAV);

    uint16_t dispatchX = (hpBarDispathSize + 7) / 8;  // Округляем вверх (1012 групп)
    uint16_t dispatchY = (hpBarDispathSize + 7) / 8;   // Округляем вверх (13 групп)
    cmdList->Dispatch(dispatchX, dispatchY, 1);

    cmdList->TransitionBarrier(hbBarTex.GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);
    cmdList->FlushResourceBarriers();

    // device->GetCommandQueue(GQueueType::Compute)->ExecuteCommandList(cmd);
}

UILayer::UILayer(const std::shared_ptr<GDevice>& device, const HWND hwnd) : hwnd(hwnd), device((device))
{
    Initialize();
    CreateDeviceObject();

    /* for (uint16_t i = 0; i < 0; i++)
      {
          blurDescriptors.push_back(device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources));

          blurFilters.push_back(std::make_shared<BlurFilter>(
              device->GetDXDevice().Get(),
              blurDescriptors[i].GetDescriptorHeap()->GetDirectxHeap(),
              wnd->GetClientWidth(),
              wnd->GetClientHeight(),
              DXGI_FORMAT_R8G8B8A8_UNORM,
              globalCountFrameResources,
              blurDescriptors[i].GetGPUHandle(),
              blurDescriptors[i].GetCPUHandle(),
              device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
          ));

          blurFilters[i]->OnResize(ImVec2(0.0f, 0.0f), ImVec2(1920, 1080));

          currentBlurIndex = i;
      }*/

      //  currentDeviceName = device->GetName();
}



UILayer::~UILayer()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void UILayer::RenderEffects(const std::shared_ptr<GCommandQueue>& queue) noexcept
{
    uiDescriptorCounter = 0;
    currentDeltaTime = std::chrono::high_resolution_clock::now();
    const double deltaTime = std::chrono::duration<double>(currentDeltaTime - lastDeltaTime).count();
    lastDeltaTime = currentDeltaTime;

    const auto &cmdList = queue->GetCommandList();

    for (uint16_t i = 0; i < 30; i++)
    {
        Update(cmdList);
    }

    if (totalTime * (740.0f / 8096.0f) > (740.0f / 8096.0f))
    {
        mod = -1;
    }
    else if (totalTime * (740.0f / 8096.0f) < 0)
    {
        mod = 1;
    }

    totalTime += deltaTime * mod;

    UpdateHBBar(cmdList, totalTime * (740.0f / 8096.0f));
    
    queue->ExecuteCommandList(cmdList);
  //  queue->Close();
}

void UILayer::SetTexture()
{
    uiDescriptorCounter = 0;
    skillCounter = 0;

    auto queue = device->GetCommandQueue(GQueueType::Compute);

    const auto cmdList = queue->GetCommandList();

    for (uint16_t i = 0; i < 30; i++)
    {
     //   uiSRVs[i].~GDescriptor();
      //  uiUAVs[i].~GDescriptor();
        //    uiTexs[i].~GTexture();
        uiTexs[i].Reset();
        InitializeGradientResources();
    }

    for (uint16_t i = 0; i < 63; i++)
    {
      //  skillPicturesDescriptors[i].~GDescriptor();
      //  skillPicturesDescriptors[i] = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

        std::wstring wideStr(SKILL_PICURES_PLACEMENTS[i], SKILL_PICURES_PLACEMENTS[i] + strlen(SKILL_PICURES_PLACEMENTS[i]));
        auto tempText = GTexture::LoadTextureFromFile(wideStr, cmdList);
        tempText->SetName(std::to_wstring(i));
        assets->AddTexture(tempText);

      //  loader.get()->LoadTextureFromFile(SKILL_PICURES_PLACEMENTS[i], device->GetDXDevice().Get(), skillPicturesDescriptors[i].GetCPUHandle(), &my_textures[i], &pictureSizeX, &pictureSizeY);
    }

    for (uint16_t i = 0; i < 1; i++)
    {
        // InitializeGradientResources();
    //    uiNavigationPicturesDescriptors[i].~GDescriptor();
     //   uiNavigationPicturesDescriptors[i] = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
        
        std::wstring wideStr(NAVIGATION_ICONS_PLACEMENTS[i], NAVIGATION_ICONS_PLACEMENTS[i] + strlen(NAVIGATION_ICONS_PLACEMENTS[i]));
        auto tempText = GTexture::LoadTextureFromFile(wideStr, cmdList);
        tempText->SetName(L"Navigation Atlas");
        tempText->CreateShaderResourceView();
        assets->AddTexture(tempText);
    //    loader.get()->LoadTextureFromFile(NAVIGATION_ICONS_PLACEMENTS[i], device->GetDXDevice().Get(), uiNavigationPicturesDescriptors[i].GetCPUHandle(), &my_textures[i], &pictureSizeX, &pictureSizeY);
    }

    uiNavigation = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    loader.get()->LoadTextureFromFile(UI_NAVIGATION_ATLAS, device->GetDXDevice().Get(), uiNavigation.GetCPUHandle(), &uiNavRes, &UI_NAVIGATION_ATLAS_SIZE, &UI_NAVIGATION_ATLAS_SIZE);

    std::wstring wideStr(UI_NAVIGATION_ATLAS, UI_NAVIGATION_ATLAS + strlen(UI_NAVIGATION_ATLAS));
    auto tempText = GTexture::LoadTextureFromFile(wideStr, cmdList);
    tempText->SetName(L"UI_NAVIGATION_ATLAS");
    assets->AddTexture(tempText);



    uiMapDesc = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    loader.get()->LoadTextureFromFile(UI_MAP, device->GetDXDevice().Get(), uiMapDesc.GetCPUHandle(), &uiMapRes, &UI_MAP_SIZE, &UI_MAP_SIZE);

    wideStr = std::wstring(UI_MAP, UI_MAP + strlen(UI_MAP));
    tempText = GTexture::LoadTextureFromFile(wideStr, cmdList);
    tempText->SetName(L"UI_MAP");
    assets->AddTexture(tempText);

//    hbBarSRV.~GDescriptor();
 //   hbBarUAV.~GDescriptor();
//    hbBarTex.~GTexture();
    InitializeHPBar();

    queue->WaitForFenceValue(queue->ExecuteCommandList(cmdList));
    queue->Flush();
    device->Flush();
}

void UILayer::ChangeDevice(const std::shared_ptr<GDevice>& device)
{
    uiDescriptorCounter = 0;
    currentColorIndex = 0;

    if (this->device == device)
    {
        return;
    }
    this->device = device;

    Invalidate();
    isShared = !isShared;
    SetupRenderBackends();
    CreateDeviceObject();
   // SetStyle();
    SetTexture();

    //blurDescriptors.push_back(device->AllocateDescriptors(
  //      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
  //      globalCountFrameResources));

    // Создаем новый блюр-фильтр (возможно, BlurFilter сам создаёт mBlurMap0, mBlurMap1 и пр.)
  //  auto blurFilter = std::make_shared<BlurFilter>(
   //     device->GetDXDevice().Get(),
   //     blurDescriptors[0].GetDescriptorHeap()->GetDirectxHeap(),
   //    wnd->GetClientWidth(),
   //     wnd->GetClientHeight(),
    //    DXGI_FORMAT_R8G8B8A8_UNORM,
    //    globalCountFrameResources,
   //     blurDescriptors[0].GetGPUHandle(),
   //     blurDescriptors[0].GetCPUHandle(),
   //     device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
  //  );

    // Обновляем размеры фильтра (если нужно)
  //  blurFilter->OnResize(ImVec2(0.0f, 0.0f), ImVec2(wnd->GetClientWidth(), wnd->GetClientHeight()));

    // Добавляем его в массив
   // blurFilters.push_back(blurFilter);
}

void UILayer::ApplyGradientAndStarsEffect(
    const ImVec2& size,
    const ImVec4& gradientColor1,
    const ImVec4& gradientColor2,
    float glowSpeed,
    int starCount,
    float starSize,
    const ImVec4& starColor
)
{
    /*     ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Получение координат элемента
    ImVec2 item_min = ImGui::GetItemRectMin();
    ImVec2 item_max = ImGui::GetItemRectMax();

    // Обновление фазы свечения
    glow_phase += ImGui::GetIO().DeltaTime * glowSpeed;
    if (glow_phase > 2.0f * IM_PI) glow_phase -= 2.0f * IM_PI;

    // Вычисление градиентного цвета
    float t = 0.5f + 0.5f * sinf(glow_phase); // Анимация градиента

    // Выполняем интерполяцию компонентов цвета вручную
    ImVec4 color1 = gradientColor1;
    ImVec4 color2 = gradientColor2;
    ImVec4 interpolatedColor(
        color1.x * (1.0f - t) + color2.x * t,
        color1.y * (1.0f - t) + color2.y * t,
        color1.z * (1.0f - t) + color2.z * t,
       color1.w * (1.0f - t) + color2.w * t
    );

   ImU32 gradient_color = ImGui::ColorConvertFloat4ToU32(interpolatedColor);

    // Наложение градиента
    draw_list->AddRectFilledMultiColor(
        item_min, item_max,
        gradient_color, gradient_color, // Левый верхний и правый верхний
        gradient_color, gradient_color  // Левый нижний и правый нижний
    );

    // Добавление звездочек
   for (int i = 0; i < starCount; ++i)
    {
        float x = item_min.x + (item_max.x - item_min.x) * (0.2f + i * 0.3f / starCount) + sinf(glow_phase + i) * 5.0f;
        float y = item_min.y + (item_max.y - item_min.y) * 0.5f + cosf(glow_phase + i) * 5.0f;
        ImVec2 center(x, y);
        ImVec2 p1(center.x, center.y - starSize);
        ImVec2 p2(center.x - starSize * 0.5f, center.y + starSize * 0.5f);
        ImVec2 p3(center.x + starSize * 0.5f, center.y + starSize * 0.5f);
        draw_list->AddTriangleFilled(p1, p2, p3, ImGui::ColorConvertFloat4ToU32(starColor));
    }*/
}

void UILayer::Render(const std::shared_ptr<GCommandList>& cmdList)
{
    skillCounter = 0;

    cmdList->SetDescriptorsHeap(&srvMemory);

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    displaySize = ImGui::GetIO().DisplaySize;

    for (uint16_t i = 0; i < 1; i++)
    {
        currentColorIndex = 0;
       // DrawLeftBar();
        DrawLeftBottomPanel();
        DrawRightBar();
        DrawRightTopBar();
        DrawBottomPanel();
        DrawRightMiddlePanel();
    }

    ImGui::Begin("detail level", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    ImGui::Text(std::to_string(currentDetailsLevel).c_str());
    ImGui::End();

    ImGui::ShowDemoWindow();
    ImGui::Render();

    // Рендерим ImGui
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList->GetGraphicsCommandList().Get());

   // lastTime = std::chrono::high_resolution_clock::now();
    //std::chrono::duration<double> duration = lastTime - currentTime;
  //  timee = duration.count();
   // totalTime += timee * mod;
   // deltatime = timee;
}
