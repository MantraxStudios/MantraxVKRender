#pragma once

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#elif defined(__linux__)
#include <vulkan/vulkan.h>
#include <X11/Xlib.h>
#else
#error "Plataforma no soportada"
#endif

#include <vector>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>
#include <iostream>
#include <functional>
#include <array>

#include <glm/glm.hpp>
#include <algorithm>

#include "../../MantraxECS/include/EngineLoaderDLL.h"

namespace Mantrax
{
    struct MANTRAX_API Vertex
    {
        float position[3];
        float color[3];
        float texCoord[2];
        float normal[3];
        float barycentric[3];

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(Vertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return binding;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
        {
            std::vector<VkVertexInputAttributeDescription> attrs(5);

            attrs[0].binding = 0;
            attrs[0].location = 0;
            attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[0].offset = offsetof(Vertex, position);

            attrs[1].binding = 0;
            attrs[1].location = 1;
            attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[1].offset = offsetof(Vertex, color);

            attrs[2].binding = 0;
            attrs[2].location = 2;
            attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
            attrs[2].offset = offsetof(Vertex, texCoord);

            attrs[3].binding = 0;
            attrs[3].location = 3;
            attrs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[3].offset = offsetof(Vertex, normal);

            attrs[4].binding = 0;
            attrs[4].location = 4;
            attrs[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[4].offset = offsetof(Vertex, barycentric);

            return attrs;
        }
    };

    struct MANTRAX_API UniformBufferObject
    {
        float model[16];
        float view[16];
        float projection[16];
        float cameraPosition[4];
    };

    class MANTRAX_API Mesh
    {
    public:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

        Mesh() = default;
        Mesh(const std::vector<Vertex> &verts, const std::vector<uint32_t> &inds)
            : vertices(verts), indices(inds) {}
    };

    struct MANTRAX_API ShaderConfig
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        VkVertexInputBindingDescription vertexBinding;
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
        bool depthWriteEnable = true;
        bool depthTestEnable = true;
        bool blendEnable = true;
    };

    class MANTRAX_API Shader
    {
    public:
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        ShaderConfig config;

        Shader() = default;
        Shader(const ShaderConfig &cfg) : config(cfg) {}
    };

    class MANTRAX_API Texture
    {
    public:
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;

        Texture() = default;
    };

    struct MANTRAX_API MaterialPushConstants
    {
        float baseColorFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // RGB + unused
        float metallicFactor = 1.0f;
        float roughnessFactor = 0.3f;
        float normalScale = 1.0f;
        int32_t useAlbedoMap = 0;
        int32_t useNormalMap = 0;
        int32_t useMetallicMap = 0;
        int32_t useRoughnessMap = 0;
        int32_t useAOMap = 0;
    };

    // Conjunto de texturas PBR
    struct MANTRAX_API PBRTextures
    {
        std::shared_ptr<Texture> albedo;
        std::shared_ptr<Texture> normal;
        std::shared_ptr<Texture> metallic;
        std::shared_ptr<Texture> roughness;
        std::shared_ptr<Texture> ao;
    };

    // Actualizar la clase Material
    class MANTRAX_API Material
    {
    public:
        std::shared_ptr<Shader> shader;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        UniformBufferObject ubo{};
        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;

        // Texturas PBR
        PBRTextures pbrTextures;

        // Push constants para propiedades del material
        MaterialPushConstants pushConstants;

        Material() = default;
        Material(std::shared_ptr<Shader> shdr) : shader(shdr) {}

        // Helper methods
        void SetAlbedoTexture(std::shared_ptr<Texture> tex)
        {
            pbrTextures.albedo = tex;
            pushConstants.useAlbedoMap = (tex != nullptr) ? 1 : 0;
        }

        void SetNormalTexture(std::shared_ptr<Texture> tex)
        {
            pbrTextures.normal = tex;
            pushConstants.useNormalMap = (tex != nullptr) ? 1 : 0;
        }

        void SetMetallicTexture(std::shared_ptr<Texture> tex)
        {
            pbrTextures.metallic = tex;
            pushConstants.useMetallicMap = (tex != nullptr) ? 1 : 0;
        }

        void SetRoughnessTexture(std::shared_ptr<Texture> tex)
        {
            pbrTextures.roughness = tex;
            pushConstants.useRoughnessMap = (tex != nullptr) ? 1 : 0;
        }

        void SetAOTexture(std::shared_ptr<Texture> tex)
        {
            pbrTextures.ao = tex;
            pushConstants.useAOMap = (tex != nullptr) ? 1 : 0;
        }

        void SetBaseColor(float r, float g, float b)
        {
            pushConstants.baseColorFactor[0] = r;
            pushConstants.baseColorFactor[1] = g;
            pushConstants.baseColorFactor[2] = b;
        }

        void SetMetallicFactor(float m)
        {
            pushConstants.metallicFactor = glm::clamp(m, 0.0f, 1.0f);
        }

        void SetRoughnessFactor(float r)
        {
            pushConstants.roughnessFactor = glm::clamp(r, 0.0f, 1.0f);
        }

        void SetNormalScale(float s)
        {
            pushConstants.normalScale = s;
        }
    };

    struct MANTRAX_API RenderObject
    {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;

        RenderObject() = default;
        RenderObject(std::shared_ptr<Mesh> m, std::shared_ptr<Material> mat)
            : mesh(m), material(mat) {}
    };

    struct MANTRAX_API GFXConfig
    {
        bool enableValidation = false;
        VkClearColorValue clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
    };

    class MANTRAX_API OffscreenFramebuffer
    {
    public:
        VkImage colorImage = VK_NULL_HANDLE;
        VkDeviceMemory colorMemory = VK_NULL_HANDLE;
        VkImageView colorImageView = VK_NULL_HANDLE;

        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;

        VkDescriptorSet renderID = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;

        VkExtent2D extent = {0, 0};
        VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        OffscreenFramebuffer() = default;
    };

    struct MANTRAX_API RenderPassConfig
    {
        struct AttachmentConfig
        {
            VkFormat format;
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            bool isDepth = false;
        };

        std::vector<AttachmentConfig> attachments;
        std::string name; // Para debug/identificación

        // Configuración de subpass
        std::vector<uint32_t> colorAttachmentIndices;
        int32_t depthAttachmentIndex = -1;
    };

    // NUEVA: Clase para manejar un render pass y sus framebuffers asociados
    class MANTRAX_API RenderPassObject
    {
    public:
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> framebuffers;
        RenderPassConfig config;
        VkExtent2D extent;

        RenderPassObject() = default;
        RenderPassObject(const RenderPassConfig &cfg) : config(cfg) {}
    };

    class MANTRAX_API GFX
    {
    public:
        using Config = GFXConfig;

        GFX(HINSTANCE hInstance, HWND hWnd, const Config &config = Config{});
        ~GFX();
        std::shared_ptr<Texture> CreateTexture(unsigned char *data, int width, int height);
        void SetMaterialTexture(std::shared_ptr<Material> material, std::shared_ptr<Texture> texture);

        std::shared_ptr<OffscreenFramebuffer> CreateOffscreenFramebuffer(uint32_t width, uint32_t height);
        void ResizeOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen,
                                        uint32_t width, uint32_t height);

        void RenderToOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen,
                                          const std::vector<RenderObject> &objects);

        std::shared_ptr<Texture> CreateDefaultWhiteTexture();

        // Crear textura normal por defecto (normal apuntando hacia arriba)
        std::shared_ptr<Texture> CreateDefaultNormalTexture();

        // Actualizar descriptor set con todas las texturas PBR
        void UpdatePBRDescriptorSet(std::shared_ptr<Material> material);

        // Función pública para asignar texturas PBR a un material
        void SetMaterialPBRTextures(std::shared_ptr<Material> material,
                                    std::shared_ptr<Texture> albedo = nullptr,
                                    std::shared_ptr<Texture> normal = nullptr,
                                    std::shared_ptr<Texture> metallic = nullptr,
                                    std::shared_ptr<Texture> roughness = nullptr,
                                    std::shared_ptr<Texture> ao = nullptr);

        void DestroyOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen);

        std::shared_ptr<Shader> CreateShader(const ShaderConfig &config);

        std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex> &vertices,
                                         const std::vector<uint32_t> &indices);

        std::shared_ptr<Material> CreateMaterial(std::shared_ptr<Shader> shader);

        std::shared_ptr<RenderPassObject> CreateRenderPass(const RenderPassConfig &config);

        void CreateFramebuffersForRenderPass(std::shared_ptr<RenderPassObject> renderPassObj,
                                             const std::vector<VkImageView> &additionalAttachments = {});

        std::shared_ptr<Shader> CreateShaderWithRenderPass(const ShaderConfig &config,
                                                           std::shared_ptr<RenderPassObject> renderPassObj);
        void BeginRenderPassInCommandBuffer(VkCommandBuffer cmd,
                                            std::shared_ptr<RenderPassObject> renderPassObj,
                                            uint32_t framebufferIndex,
                                            const std::vector<VkClearValue> &clearValues);

        // Dibujar con un render pass específico
        void DrawFrameWithRenderPass(std::shared_ptr<RenderPassObject> renderPassObj,
                                     const std::vector<RenderObject> &objects,
                                     const std::vector<VkClearValue> &clearValues,
                                     std::function<void(VkCommandBuffer)> additionalCommands = nullptr);

        // Limpiar render passes personalizados
        void CleanupCustomRenderPasses();

        void AddRenderObject(const RenderObject &obj);

        void UpdateMaterialUBO(Material *material, const UniformBufferObject &ubo);

        void ClearRenderObjects();

        void NotifyFramebufferResized();

        bool DrawFrame(std::function<void(VkCommandBuffer)> imguiRenderCallback = nullptr);

        void WaitIdle();

        void ClearRenderObjectsSafe();
        void AddRenderObjectSafe(const RenderObject &obj);
        std::vector<RenderObject> GetRenderObjects() const;

        VkInstance GetInstance() const { return m_Instance; }
        VkDevice GetDevice() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkCommandPool GetCommandPool() const { return m_CommandPool; }
        VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }
        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
        VkFormat GetSwapchainImageFormat() const { return m_SwapchainImageFormat; }
        VkFormat GetDepthFormat() const { return m_DepthFormat; }
        VkImageView GetDepthImageView() const { return m_DepthImageView; }

        // Getters adicionales para rendering manual
        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        const VkSemaphore &GetImageAvailableSemaphoreRef() const { return m_ImageAvailableSemaphore; }
        const VkSemaphore &GetRenderFinishedSemaphoreRef() const { return m_RenderFinishedSemaphore; }
        const VkFence &GetInFlightFenceRef() const { return m_InFlightFence; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        VkCommandBuffer GetCommandBuffer(uint32_t index) const { return m_CommandBuffers[index]; }
        VkFramebuffer GetFramebuffer(uint32_t index) const { return m_SwapchainFramebuffers[index]; }

    private:
        Config m_Config;
        HINSTANCE m_hInstance;
        HWND m_hWnd;
        bool m_FramebufferResized;

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        uint32_t m_GraphicsQueueFamily;
        uint32_t m_PresentQueueFamily;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        VkImage m_DepthImage;
        VkDeviceMemory m_DepthImageMemory;
        VkImageView m_DepthImageView;
        VkFormat m_DepthFormat;

        VkSwapchainKHR m_Swapchain;
        VkFormat m_SwapchainImageFormat;
        VkExtent2D m_SwapchainExtent;
        std::vector<VkImage> m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        std::vector<std::shared_ptr<RenderPassObject>> m_CustomRenderPasses;

        VkRenderPass m_RenderPass;

        std::vector<VkFramebuffer> m_SwapchainFramebuffers;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        VkDescriptorPool m_DescriptorPool;

        VkSemaphore m_ImageAvailableSemaphore;
        VkSemaphore m_RenderFinishedSemaphore;
        VkFence m_InFlightFence;

        std::vector<RenderObject> m_RenderObjects;
        bool m_NeedCommandBufferRebuild = false;

        std::vector<std::shared_ptr<Shader>> m_AllShaders;

    private:
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void UpdateDescriptorSetWithTexture(std::shared_ptr<Material> material);
        static std::vector<char> ReadFile(const std::string &filename);
        void CreateImage(uint32_t width, uint32_t height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         VkImage &image, VkDeviceMemory &memory);
        VkImageView CreateImageView(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspectFlags);
        VkRenderPass CreateOffscreenRenderPass(VkFormat colorFormat, VkFormat depthFormat);
        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer cmd);
        void TransitionImageLayout(VkImage image, VkFormat format,
                                   VkImageLayout oldLayout, VkImageLayout newLayout);
        void TransitionImageLayoutCmd(VkCommandBuffer cmd, VkImage image, VkFormat format,
                                      VkImageLayout oldLayout, VkImageLayout newLayout);
        VkShaderModule CreateShaderModule(const std::vector<char> &code);
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags props, VkBuffer &buffer,
                          VkDeviceMemory &memory);
        void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        void InitVulkan();
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapchain(bool enableVSync);
        void CreateImageViews();
        VkFormat FindDepthFormat();
        void CreateDepthResources();
        void CreateRenderPass();
        void CreateDescriptorPool();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateShaderPipeline(std::shared_ptr<Shader> shader, VkRenderPass renderPass = VK_NULL_HANDLE);
        void CreateVertexBuffer(std::shared_ptr<Mesh> mesh);
        void CreateIndexBuffer(std::shared_ptr<Mesh> mesh);
        void CreateUniformBuffer(std::shared_ptr<Material> material);
        void CreateDescriptorSet(std::shared_ptr<Material> material);
        void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t index,
                                 std::function<void(VkCommandBuffer)> imguiRenderCallback = nullptr);
        void CleanupSwapchain();
        void RecreateSwapchainWithCustomRenderPasses();
        void RecreateSwapchain();
        void Cleanup();
    };
}