#pragma once

#include "../render/include/MantraxGFX_API.h"
#include "../add-ons/include/imgui/imgui.h"
#include "../add-ons/include/imgui/imgui_impl_win32.h"
#include "../add-ons/include/imgui/imgui_impl_vulkan.h"
#include <glm/glm.hpp>
#include <stdexcept>

namespace Mantrax
{
    class ImGuiManager
    {
    public:
        ImGuiManager(HWND hwnd, GFX *gfx) : m_GFX(gfx)
        {
            InitializeImGui(hwnd, gfx);
        }

        ~ImGuiManager()
        {
            CleanupImGui();
        }

        void BeginFrame()
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }

        void EndFrame(VkCommandBuffer commandBuffer)
        {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }

        void OnResize()
        {
            // ImGui maneja el resize automáticamente a través de Win32
        }

        void ShowDebugWindow(int fps, const glm::vec3 &cameraPos, float fov, bool sprint, bool mouseCaptured)
        {
            ImGui::Begin("Mantrax Debug Info");

            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Text("FPS: %d (%.2f ms)", fps, fps > 0 ? 1000.0f / fps : 0.0f);

            ImGui::Spacing();
            ImGui::Text("Camera");
            ImGui::Separator();
            ImGui::Text("Position:");
            ImGui::Text("  X: %.1f", cameraPos.x);
            ImGui::Text("  Y: %.1f", cameraPos.y);
            ImGui::Text("  Z: %.1f", cameraPos.z);
            ImGui::Text("FOV: %.1f", fov);

            ImGui::Spacing();
            ImGui::Text("Status");
            ImGui::Separator();
            ImGui::Text("Mode: %s", sprint ? "SPRINT" : "Walk");
            ImGui::Text("Mouse: %s", mouseCaptured ? "CAPTURED" : "Free");

            ImGui::Spacing();
            ImGui::Text("Controls");
            ImGui::Separator();
            ImGui::BulletText("Click: Capture mouse");
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Space/Ctrl: Up/Down");
            ImGui::BulletText("Shift: Sprint");
            ImGui::BulletText("R: Regenerate terrain");
            ImGui::BulletText("ESC: Release mouse/Exit");

            ImGui::End();
        }

    private:
        void InitializeImGui(HWND hwnd, GFX *gfx)
        {
            // Crear descriptor pool - IGUAL QUE EL EJEMPLO OFICIAL
            VkDescriptorPoolSize pool_sizes[] = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes = pool_sizes;

            if (vkCreateDescriptorPool(gfx->GetDevice(), &pool_info, nullptr, &m_DescriptorPool) != VK_SUCCESS)
                throw std::runtime_error("Failed to create ImGui descriptor pool");

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            // Setup Platform/Renderer backends
            ImGui_ImplWin32_Init(hwnd);

            // EXACTAMENTE COMO EL EJEMPLO OFICIAL
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = gfx->GetInstance();
            init_info.PhysicalDevice = gfx->GetPhysicalDevice();
            init_info.Device = gfx->GetDevice();
            init_info.QueueFamily = gfx->GetGraphicsQueueFamily();
            init_info.Queue = gfx->GetGraphicsQueue();
            init_info.PipelineCache = VK_NULL_HANDLE;
            init_info.DescriptorPool = m_DescriptorPool;
            init_info.MinImageCount = 2;
            init_info.ImageCount = 2; // Ajusta esto según tu swapchain
            init_info.Allocator = nullptr;
            init_info.PipelineInfoMain.RenderPass = gfx->GetRenderPass();
            init_info.PipelineInfoMain.Subpass = 0;
            init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.CheckVkResultFn = nullptr; // O pon tu función check_vk_result

            // Inicializar ImGui Vulkan - LAS FUENTES SE CARGAN AUTOMÁTICAMENTE
            ImGui_ImplVulkan_Init(&init_info);

            m_Initialized = true;
        }

        void CleanupImGui()
        {
            if (!m_Initialized)
                return;

            vkDeviceWaitIdle(m_GFX->GetDevice());

            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            if (m_DescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(m_GFX->GetDevice(), m_DescriptorPool, nullptr);
                m_DescriptorPool = VK_NULL_HANDLE;
            }

            m_Initialized = false;
        }

        GFX *m_GFX = nullptr;
        bool m_Initialized = false;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    };
}