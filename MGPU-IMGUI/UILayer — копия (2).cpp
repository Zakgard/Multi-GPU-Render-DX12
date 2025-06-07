#include "UILayer.h"

#include "ComputePSO.h"
#include <utility>
#include "Window.h"
#include "GResourceStateTracker.h"
#include "dxgiformat.h"
#include <random>
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
#include <chrono>
#include <fstream>
#include "Data.h"
#include "BlurFilter.h"

static double timee;
static std::vector<ImVec4> baseColors;
static std::vector<ImVec4> endColors;
static int currentColorIndex = 0;
static int maxColors = 100;
static float glow_phase = 0.0f;
static bool initialized = false;
static float glow_speed = 0.05f;
static std::wstring currentDeviceName;
static int postDeviceChangeFpsCounter = 0;
static int fpsToInitBlur = 10;

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

    srvPircutre = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, globalCountFrameResources);

    loader = std::make_shared<UIPictruresLoader>(UIPictruresLoader());

    displaySize = ImGui::GetIO().DisplaySize;

    SetTexture();
    srand(static_cast<unsigned int>(time(nullptr))); // Инициализация случайных чисел
    initialized = true;
    //DrawBlurShit();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(this->hwnd);

    ImGui_ImplDX12_Init(device->GetDXDevice().Get(), 1,
        DXGI_FORMAT_R8G8B8A8_UNORM, srvMemory.GetDescriptorHeap()->GetDirectxHeap(),
        srvMemory.GetCPUHandle(), srvMemory.GetGPUHandle());
    InitializeGradientResources();
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
    assets = std::make_shared<AssetsLoader>(device);
    SetupRenderBackends();
  //  InitializeGradientResources();
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
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 4.0f;

    ImGui::StyleColorsDark();
    style.WindowRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.6f; // Полупрозрачный фон
    style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // Синяя граница

    for (uint16_t i = 0; i < maxColors - 1; i++)
    {
        float r = static_cast<float>(color(rng)) / 255.0f;
        float g = static_cast<float>(color(rng)) / 255.0f;
        float b = static_cast<float>(color(rng)) / 255.0f;
        baseColors.push_back(ImVec4(r, g, b, 0.3f));

        r = static_cast<float>(color(rng)) / 255.0f;
        g = static_cast<float>(color(rng)) / 255.0f;
        b = static_cast<float>(color(rng)) / 255.0f;
        endColors.push_back(ImVec4(r, g, b, 0.3f));
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(7680, 4320); // 8K разрешение
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
    fps = FPS;
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

    ImGui::ImageButton((void*)(intptr_t)skillPicturesDescriptors[36].GetGPUHandle().ptr, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на реальную текстуру
    ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
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
        ImGui::ImageButton((void*)(intptr_t)skillPicturesDescriptors[36].GetGPUHandle().ptr, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", L"Goida");
        ImGui::SameLine();
    }
    ImGui::End();
}

void UILayer::DrawLeftBottomPanel()
{
    ImGui::SetNextWindowPos(ImVec2(paddingX, displaySize.y - paddingY - buttonSize + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(buttonSize * 10 + 100, buttonSize + 20));
    ImGui::Begin("HotbarLeft", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 10; ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::ImageButton((void*)(intptr_t)uiNavigationPicturesDescriptors[1].GetGPUHandle().ptr, ImVec2(buttonSize, buttonSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Слот %zu", i + 1);
        if (i < 9) ImGui::SameLine();
        ImGui::PopID();
    }
    ImGui::End();
}

void UILayer::DrawRightBar()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - paddingX - iconSize * 10 - 100, displaySize.y - paddingY - iconSize + bottomPosOffset));
    ImGui::SetNextWindowSize(ImVec2(iconSize * 10 + 100, iconSize + 20));
    ImGui::Begin("HotbarRight", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 10; ++i) {
        ImGui::PushID(static_cast<int>(i + 22));
        ImGui::ImageButton((void*)(intptr_t)uiNavigationPicturesDescriptors[1].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
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
    ImGui::ImageButton((void*)(intptr_t)skillPicturesDescriptors[36].GetGPUHandle().ptr, ImVec2(180.0f, 60.0f), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на текстуру миникарты
    ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
    currentColorIndex++;
    // 5.2 Текущее время
    ImGui::Text("Время: %s", L"22:34");
    ImGui::End();
}

void UILayer::DrawRightMiddlePanel()
{
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - iconSize - 100, displaySize.y - iconSize * 15));
    ImGui::SetNextWindowSize(ImVec2((iconSize + 10) * 2, iconSize * 10 + 100));

    ImGui::Begin("HotbarRightMiddle", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    for (size_t i = 0; i < 20; ++i) {
        ImGui::PushID(static_cast<int>(i + 22));
        ImGui::ImageButton((void*)(intptr_t)uiNavigationPicturesDescriptors[1].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
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
        ImGui::ImageButton((void*)(intptr_t)effectTextureSRV.GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0); // Замени на иконки
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
        currentColorIndex++;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Способность %zu", i + 1);
        if (i < 14) ImGui::SameLine();
        ImGui::PopID();

        skillCounter++;
    }

    // 3.1 Ряд иконок-способностей (нижний)
    for (size_t i = 0; i < 15; ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::ImageButton((void*)(intptr_t)uiNavigationPicturesDescriptors[1].GetGPUHandle().ptr, ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0);
        ApplyGradientAndStarsEffect(ImVec2(iconSize, iconSize), baseColors[currentColorIndex], endColors[currentColorIndex]);
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
    //  blurFilters[currentBlurIndex]->OnResize(ImVec2(iconPos.x, iconPos.y), ImVec2(size.x, size.y));
    // currentBlurIndex++;
}

void UILayer::Update(const std::shared_ptr<GCommandList>& cmdList)
{
    cmdList->SetPipelineState(gradientComputePSO);
    cmdList->SetRootSignature(gradientRootSignature);

    // Устанавливаем дескрипторный хип и UAV
    cmdList->SetDescriptorsHeap(&effectTextureUAV);
    cmdList->SetRootDescriptorTable(0, &effectTextureUAV);

    // Убедимся, что текстура в состоянии UAV
    cmdList->TransitionBarrier(*effectTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->FlushResourceBarriers();

    // Диспетчеризация (256x256 текстура, 16x16 нитей на группу)
    const UINT threadGroupX = (256 + 15) / 16; // 16 групп
    const UINT threadGroupY = (256 + 15) / 16; // 16 групп
    cmdList->Dispatch(threadGroupX, threadGroupY, 1);

    // UAV барьер для синхронизации
    cmdList->UAVBarrier(*effectTexture);
    cmdList->FlushResourceBarriers();
}

void UILayer::InitializeGradientResources()
{
    const UINT width = 256;
    const UINT height = 256;
    D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        width,
        height,
        1, // Array size
        1, // Mip levels
        1, // Sample count
        0, // Sample quality
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    effectTexture = std::make_shared<GResource>(
        device,
        texDesc,
        L"GradientTexture",
        nullptr,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        heapProps,
        D3D12_HEAP_FLAG_NONE
    );

    // SRV
    effectTextureSRV = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    effectTexture->CreateShaderResourceView(&srvDesc, &effectTextureSRV, 0);

    // UAV
    effectTextureUAV = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = texDesc.Format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    effectTexture->CreateUnorderedAccessView(&uavDesc, &effectTextureUAV, 0, 0);

    // Копируем SRV в srvMemory для ImGui
    device->GetDXDevice()->CopyDescriptorsSimple(
        1,
        srvMemory.GetCPUHandle(1), // Смещение 0
        effectTextureSRV.GetCPUHandle(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );

    std::wstring ws = L"Shaders\\GradientCompute.hlsl";
    // Создаем шейдер
    auto computeShader = std::make_shared<GShader>(
        ws,
        ShaderType::ComputeShader
    );

    // Корневая сигнатура
    computeRootSignature = GRootSignature();
    CD3DX12_ROOT_PARAMETER rootParam;
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParam.Descriptor.ShaderRegister = 0;
    rootParam.Descriptor.RegisterSpace = 0;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    computeRootSignature.AddParameter(rootParam);
    computeRootSignature.Initialize(device);

    // PSO
    gradientComputePSO = ComputePSO(computeRootSignature);
    gradientComputePSO.SetShader(computeShader.get());
    gradientComputePSO.Initialize(device);
}


UILayer::UILayer(const std::shared_ptr<GDevice>& device, const HWND hwnd, std::shared_ptr<Window> wnd) : hwnd(hwnd), device((device))
{
    Initialize();
    CreateDeviceObject();

    for (uint16_t i = 0; i < blursToPush; i++)
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
    }

    currentDeviceName = device->GetName();
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
        skillPicturesDescriptors[i].~GDescriptor();
    }

    for (uint16_t i = 0; i < sizeof(uiNavigationPicturesDescriptors) / sizeof(uiNavigationPicturesDescriptors[0]); i++)
    {
        uiNavigationPicturesDescriptors[i].~GDescriptor();
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
    blurDescriptors.clear();
    blurFilters.clear();
    baseColors.clear();
    endColors.clear();

    currentColorIndex = 0;
    currentBlurIndex = 0;

    SetStyle();

    if (this->device == device)
    {
        return;
    }
    this->device = device;

    Invalidate();
    SetupRenderBackends();
    CreateDeviceObject();

    blurDescriptors.push_back(device->AllocateDescriptors(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        globalCountFrameResources));

    // Создаем новый блюр-фильтр (возможно, BlurFilter сам создаёт mBlurMap0, mBlurMap1 и пр.)
    auto blurFilter = std::make_shared<BlurFilter>(
        device->GetDXDevice().Get(),
        blurDescriptors[0].GetDescriptorHeap()->GetDirectxHeap(),
        wnd->GetClientWidth(),
        wnd->GetClientHeight(),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        globalCountFrameResources,
        blurDescriptors[0].GetGPUHandle(),
        blurDescriptors[0].GetCPUHandle(),
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    );

    // Обновляем размеры фильтра (если нужно)
    blurFilter->OnResize(ImVec2(0.0f, 0.0f), ImVec2(wnd->GetClientWidth(), wnd->GetClientHeight()));

    // Добавляем его в массив
    blurFilters.push_back(blurFilter);
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
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

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
    }
}

void UILayer::Render(const std::shared_ptr<GCommandList>& cmdList)
{
    auto start = std::chrono::high_resolution_clock::now();

    cmdList->TransitionBarrier(*effectTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->FlushResourceBarriers();

    // Устанавливаем хип с SRV
    cmdList->SetDescriptorsHeap(&effectTextureSRV);
    cmdList->SetDescriptorsHeap(&srvMemory);
    
    auto queue = device->GetCommandQueue(GQueueType::Compute);

    const auto cmdList2 = queue->GetCommandList();
    Update(cmdList2);
 // GResource res = GResource();

//    blurFilters[0]->OnResize(ImVec2(0.0f, 0.0f), ImVec2(353, 349));

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    displaySize = ImGui::GetIO().DisplaySize;

    for (uint16_t i = 0; i < 1; i++)
    {
        currentColorIndex = 0;
        DrawLeftBar();
        DrawLeftBottomPanel();
        DrawRightBar();
        DrawRightTopBar();
        DrawRightMiddlePanel();

        DrawBottomPanel();
    }
    if (timee) {
        ImGui::Begin("sec", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        ImGui::Text(std::to_string(timee).c_str());
        ImGui::End();
    }

    // При рендеринге

    ImGui::ShowDemoWindow();
    ImGui::Render();

    if (ImGui::GetDrawData()) 
    {
       // ImGui::GetDrawData()->ScaleClipRects(ImGui::GetIO().DisplayFramebufferScale);
    }

    cmdList->SetDescriptorsHeap(&srvMemory);
    cmdList->SetDescriptorsHeap(&effectTextureSRV);

    // Рендерим ImGui
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList->GetGraphicsCommandList().Get());

    // Возвращаем текстуру в состояние UAV для следующего вычисления
    cmdList->TransitionBarrier(*effectTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->FlushResourceBarriers();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    timee = duration.count();
    
 //   blurFilters[0]->GetBackBufferResource(cmdList->GetDevice()->GetCommandQueue()->get)
//    blurFilters[0]->Execute(cmdList->GetGraphicsCommandList().Get(), my_textures[0], 40);
}
