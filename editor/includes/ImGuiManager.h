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
        }

        void DestroyImgui()
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
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
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

            ImGui::End();
        }

    private:
        void InitializeImGui(HWND hwnd, GFX *gfx)
        {
            // Descriptor pool
            VkDescriptorPoolSize pool_sizes[] = {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

            VkDescriptorPoolCreateInfo pool_info{};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes = pool_sizes;

            if (vkCreateDescriptorPool(gfx->GetDevice(), &pool_info, nullptr, &m_DescriptorPool) != VK_SUCCESS)
                throw std::runtime_error("Failed to create ImGui descriptor pool");

            // ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();

            // Enable docking only
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            // 🔥 Important: Disable viewports (this prevents the crash)
            io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

            // Style
            ImGui::StyleColorsDark();

            // Win32 backend
            ImGui_ImplWin32_Init(hwnd);

            // Vulkan backend
            ImGui_ImplVulkan_InitInfo init_info{};
            init_info.Instance = gfx->GetInstance();
            init_info.PhysicalDevice = gfx->GetPhysicalDevice();
            init_info.Device = gfx->GetDevice();
            init_info.QueueFamily = gfx->GetGraphicsQueueFamily();
            init_info.Queue = gfx->GetGraphicsQueue();
            init_info.DescriptorPool = m_DescriptorPool;
            init_info.MinImageCount = 2;
            init_info.ImageCount = 2;
            init_info.PipelineCache = VK_NULL_HANDLE;
            init_info.Allocator = nullptr;
            init_info.CheckVkResultFn = nullptr;
            init_info.PipelineInfoMain.RenderPass = gfx->GetRenderPass();
            init_info.PipelineInfoMain.Subpass = 0;

            ImGui_ImplVulkan_Init(&init_info);

            m_Initialized = true;
        }

        void CleanupImGui()
        {
            if (!m_Initialized)
                return;

            vkDeviceWaitIdle(m_GFX->GetDevice());

            // Shutdown backends & context
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            // Destroy pool
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
