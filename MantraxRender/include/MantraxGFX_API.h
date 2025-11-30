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

namespace Mantrax
{
    struct Vertex
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

    struct UniformBufferObject
    {
        float model[16];
        float view[16];
        float projection[16];
    };

    class Mesh
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

    struct ShaderConfig
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        VkVertexInputBindingDescription vertexBinding;
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        bool depthTestEnable = true;
        bool blendEnable = false;
    };

    class Shader
    {
    public:
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        ShaderConfig config;

        Shader() = default;
        Shader(const ShaderConfig &cfg) : config(cfg) {}
    };

    class Material
    {
    public:
        std::shared_ptr<Shader> shader;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        UniformBufferObject ubo{};
        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;

        Material() = default;
        Material(std::shared_ptr<Shader> shdr) : shader(shdr) {}
    };

    struct RenderObject
    {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;

        RenderObject() = default;
        RenderObject(std::shared_ptr<Mesh> m, std::shared_ptr<Material> mat)
            : mesh(m), material(mat) {}
    };

    struct GFXConfig
    {
        bool enableValidation = false;
        VkClearColorValue clearColor = {0.3f, 0.3f, 0.3f, 1.0f};
    };

    class OffscreenFramebuffer
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

        VkDescriptorSet imguiTextureID = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;

        VkExtent2D extent = {0, 0};
        VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

        OffscreenFramebuffer() = default;
    };

    struct RenderPassConfig
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
    class RenderPassObject
    {
    public:
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> framebuffers;
        RenderPassConfig config;
        VkExtent2D extent;

        RenderPassObject() = default;
        RenderPassObject(const RenderPassConfig &cfg) : config(cfg) {}
    };

    class GFX
    {
    public:
        using Config = GFXConfig;

        GFX(HINSTANCE hInstance, HWND hWnd, const Config &config = Config{})
            : m_hInstance(hInstance),
              m_hWnd(hWnd),
              m_Config(config),
              m_FramebufferResized(false),
              m_Instance(VK_NULL_HANDLE),
              m_Surface(VK_NULL_HANDLE),
              m_PhysicalDevice(VK_NULL_HANDLE),
              m_Device(VK_NULL_HANDLE),
              m_GraphicsQueueFamily(UINT32_MAX),
              m_PresentQueueFamily(UINT32_MAX),
              m_GraphicsQueue(VK_NULL_HANDLE),
              m_PresentQueue(VK_NULL_HANDLE),
              m_Swapchain(VK_NULL_HANDLE),
              m_SwapchainImageFormat(VK_FORMAT_B8G8R8A8_UNORM),
              m_RenderPass(VK_NULL_HANDLE),
              m_CommandPool(VK_NULL_HANDLE),
              m_DescriptorPool(VK_NULL_HANDLE),
              m_ImageAvailableSemaphore(VK_NULL_HANDLE),
              m_RenderFinishedSemaphore(VK_NULL_HANDLE),
              m_InFlightFence(VK_NULL_HANDLE),
              m_DepthImage(VK_NULL_HANDLE),
              m_DepthImageMemory(VK_NULL_HANDLE),
              m_DepthImageView(VK_NULL_HANDLE),
              m_DepthFormat(VK_FORMAT_D32_SFLOAT)
        {
            RECT rect;
            GetClientRect(m_hWnd, &rect);
            m_SwapchainExtent.width = rect.right - rect.left;
            m_SwapchainExtent.height = rect.bottom - rect.top;

            InitVulkan();
        }

        ~GFX()
        {
            Cleanup();
        }

        std::shared_ptr<OffscreenFramebuffer> CreateOffscreenFramebuffer(uint32_t width, uint32_t height)
        {
            auto offscreen = std::make_shared<OffscreenFramebuffer>();
            offscreen->extent = {width, height};
            offscreen->colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
            offscreen->depthFormat = m_DepthFormat;

            // Crear color image con los usage flags correctos
            CreateImage(width, height, offscreen->colorFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        offscreen->colorImage, offscreen->colorMemory);

            offscreen->colorImageView = CreateImageView(offscreen->colorImage,
                                                        offscreen->colorFormat,
                                                        VK_IMAGE_ASPECT_COLOR_BIT);

            // Crear depth image
            CreateImage(width, height, offscreen->depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        offscreen->depthImage, offscreen->depthMemory);

            offscreen->depthImageView = CreateImageView(offscreen->depthImage,
                                                        offscreen->depthFormat,
                                                        VK_IMAGE_ASPECT_DEPTH_BIT);

            // Crear render pass
            offscreen->renderPass = CreateOffscreenRenderPass(offscreen->colorFormat,
                                                              offscreen->depthFormat);

            // Crear framebuffer
            std::array<VkImageView, 2> attachments = {
                offscreen->colorImageView,
                offscreen->depthImageView};

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = offscreen->renderPass;
            fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = width;
            fbInfo.height = height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &offscreen->framebuffer) != VK_SUCCESS)
                throw std::runtime_error("Error creando offscreen framebuffer");

            // Crear sampler
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &offscreen->sampler) != VK_SUCCESS)
                throw std::runtime_error("Error creando sampler");

            // CORRECCIÓN: Transicionar directamente a SHADER_READ_ONLY sin pasar por render
            // La primera vez que se use para render, se hará la transición correcta
            VkCommandBuffer cmd = BeginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = offscreen->colorImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            EndSingleTimeCommands(cmd);

            std::cout << "Offscreen framebuffer creado: " << width << "x" << height << "\n";

            return offscreen;
        }

        void ResizeOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen,
                                        uint32_t width, uint32_t height)
        {
            if (offscreen->extent.width == width && offscreen->extent.height == height)
                return;

            vkDeviceWaitIdle(m_Device);

            // Limpiar recursos anteriores
            if (offscreen->framebuffer != VK_NULL_HANDLE)
                vkDestroyFramebuffer(m_Device, offscreen->framebuffer, nullptr);
            if (offscreen->colorImageView != VK_NULL_HANDLE)
                vkDestroyImageView(m_Device, offscreen->colorImageView, nullptr);
            if (offscreen->colorImage != VK_NULL_HANDLE)
                vkDestroyImage(m_Device, offscreen->colorImage, nullptr);
            if (offscreen->colorMemory != VK_NULL_HANDLE)
                vkFreeMemory(m_Device, offscreen->colorMemory, nullptr);
            if (offscreen->depthImageView != VK_NULL_HANDLE)
                vkDestroyImageView(m_Device, offscreen->depthImageView, nullptr);
            if (offscreen->depthImage != VK_NULL_HANDLE)
                vkDestroyImage(m_Device, offscreen->depthImage, nullptr);
            if (offscreen->depthMemory != VK_NULL_HANDLE)
                vkFreeMemory(m_Device, offscreen->depthMemory, nullptr);

            // Reinicializar handles
            offscreen->framebuffer = VK_NULL_HANDLE;
            offscreen->colorImageView = VK_NULL_HANDLE;
            offscreen->colorImage = VK_NULL_HANDLE;
            offscreen->colorMemory = VK_NULL_HANDLE;
            offscreen->depthImageView = VK_NULL_HANDLE;
            offscreen->depthImage = VK_NULL_HANDLE;
            offscreen->depthMemory = VK_NULL_HANDLE;

            // Recrear con nuevo tamaño
            offscreen->extent = {width, height};

            CreateImage(width, height, offscreen->colorFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        offscreen->colorImage, offscreen->colorMemory);

            offscreen->colorImageView = CreateImageView(offscreen->colorImage,
                                                        offscreen->colorFormat,
                                                        VK_IMAGE_ASPECT_COLOR_BIT);

            CreateImage(width, height, offscreen->depthFormat,
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        offscreen->depthImage, offscreen->depthMemory);

            offscreen->depthImageView = CreateImageView(offscreen->depthImage,
                                                        offscreen->depthFormat,
                                                        VK_IMAGE_ASPECT_DEPTH_BIT);

            std::array<VkImageView, 2> attachments = {
                offscreen->colorImageView,
                offscreen->depthImageView};

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = offscreen->renderPass;
            fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = width;
            fbInfo.height = height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(m_Device, &fbInfo, nullptr, &offscreen->framebuffer) != VK_SUCCESS)
                throw std::runtime_error("Error recreando offscreen framebuffer");

            // Transicionar la nueva imagen
            VkCommandBuffer cmd = BeginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = offscreen->colorImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            EndSingleTimeCommands(cmd);

            std::cout << "Offscreen framebuffer redimensionado: " << width << "x" << height << "\n";
        }

        void RenderToOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen,
                                          const std::vector<RenderObject> &objects,
                                          const VkClearColorValue &clearColor = {0.6f, 0.6f, 0.8f, 1.0f})
        {
            VkCommandBuffer cmd = BeginSingleTimeCommands();

            // Transición: SHADER_READ_ONLY -> COLOR_ATTACHMENT
            VkImageMemoryBarrier barrier1{};
            barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier1.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier1.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier1.image = offscreen->colorImage;
            barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier1.subresourceRange.baseMipLevel = 0;
            barrier1.subresourceRange.levelCount = 1;
            barrier1.subresourceRange.baseArrayLayer = 0;
            barrier1.subresourceRange.layerCount = 1;
            barrier1.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier1);

            // Comenzar render pass
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = clearColor;
            clearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = offscreen->renderPass;
            renderPassInfo.framebuffer = offscreen->framebuffer;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = offscreen->extent;
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Viewport y scissor
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(offscreen->extent.width);
            viewport.height = static_cast<float>(offscreen->extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(cmd, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = offscreen->extent;
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // Renderizar objetos
            for (const auto &obj : objects)
            {
                if (!obj.mesh || !obj.material || !obj.material->shader)
                    continue;

                if (!obj.mesh->vertexBuffer || !obj.mesh->indexBuffer)
                    continue;

                if (!obj.material->shader->pipeline)
                    continue;

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  obj.material->shader->pipeline);

                VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        obj.material->shader->pipelineLayout,
                                        0, 1, &obj.material->descriptorSet, 0, nullptr);

                vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()),
                                 1, 0, 0, 0);
            }

            vkCmdEndRenderPass(cmd);

            // Transición: COLOR_ATTACHMENT -> SHADER_READ_ONLY
            VkImageMemoryBarrier barrier2{};
            barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier2.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier2.image = offscreen->colorImage;
            barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier2.subresourceRange.baseMipLevel = 0;
            barrier2.subresourceRange.levelCount = 1;
            barrier2.subresourceRange.baseArrayLayer = 0;
            barrier2.subresourceRange.layerCount = 1;
            barrier2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier2);

            EndSingleTimeCommands(cmd);
        }

        void DestroyOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen)
        {
            if (!offscreen)
                return;

            vkDeviceWaitIdle(m_Device);

            if (offscreen->sampler)
                vkDestroySampler(m_Device, offscreen->sampler, nullptr);
            if (offscreen->framebuffer)
                vkDestroyFramebuffer(m_Device, offscreen->framebuffer, nullptr);
            if (offscreen->colorImageView)
                vkDestroyImageView(m_Device, offscreen->colorImageView, nullptr);
            if (offscreen->colorImage)
                vkDestroyImage(m_Device, offscreen->colorImage, nullptr);
            if (offscreen->colorMemory)
                vkFreeMemory(m_Device, offscreen->colorMemory, nullptr);
            if (offscreen->depthImageView)
                vkDestroyImageView(m_Device, offscreen->depthImageView, nullptr);
            if (offscreen->depthImage)
                vkDestroyImage(m_Device, offscreen->depthImage, nullptr);
            if (offscreen->depthMemory)
                vkFreeMemory(m_Device, offscreen->depthMemory, nullptr);
            if (offscreen->renderPass)
                vkDestroyRenderPass(m_Device, offscreen->renderPass, nullptr);
        }

        std::shared_ptr<Shader> CreateShader(const ShaderConfig &config)
        {
            auto shader = std::make_shared<Shader>(config);
            CreateShaderPipeline(shader);
            return shader;
        }

        std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex> &vertices,
                                         const std::vector<uint32_t> &indices)
        {
            auto mesh = std::make_shared<Mesh>(vertices, indices);
            CreateVertexBuffer(mesh);
            CreateIndexBuffer(mesh);
            return mesh;
        }

        std::shared_ptr<Material> CreateMaterial(std::shared_ptr<Shader> shader)
        {
            auto material = std::make_shared<Material>(shader);
            CreateUniformBuffer(material);
            CreateDescriptorSet(material);
            return material;
        }

        std::shared_ptr<RenderPassObject> CreateRenderPass(const RenderPassConfig &config)
        {
            auto renderPassObj = std::make_shared<RenderPassObject>(config);
            renderPassObj->extent = m_SwapchainExtent;

            // Crear attachments descriptions
            std::vector<VkAttachmentDescription> attachments;
            for (const auto &attCfg : config.attachments)
            {
                VkAttachmentDescription att{};
                att.format = attCfg.format;
                att.samples = attCfg.samples;
                att.loadOp = attCfg.loadOp;
                att.storeOp = attCfg.storeOp;
                att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                att.initialLayout = attCfg.initialLayout;
                att.finalLayout = attCfg.finalLayout;
                attachments.push_back(att);
            }

            // Referencias a attachments para el subpass
            std::vector<VkAttachmentReference> colorRefs;
            for (uint32_t idx : config.colorAttachmentIndices)
            {
                VkAttachmentReference ref{};
                ref.attachment = idx;
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorRefs.push_back(ref);
            }

            VkAttachmentReference depthRef{};
            bool hasDepth = config.depthAttachmentIndex >= 0;
            if (hasDepth)
            {
                depthRef.attachment = config.depthAttachmentIndex;
                depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            // Configurar subpass
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.data();
            subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

            // Dependencia del subpass
            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            if (hasDepth)
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }

            // Crear render pass
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPassObj->renderPass) != VK_SUCCESS)
                throw std::runtime_error("Error creando render pass personalizado: " + config.name);

            // Guardar en la lista
            m_CustomRenderPasses.push_back(renderPassObj);

            std::cout << "Render Pass '" << config.name << "' creado exitosamente\n";

            return renderPassObj;
        }

        void CreateFramebuffersForRenderPass(std::shared_ptr<RenderPassObject> renderPassObj,
                                             const std::vector<VkImageView> &additionalAttachments = {})
        {
            renderPassObj->framebuffers.resize(m_SwapchainImageViews.size());

            for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
            {
                std::vector<VkImageView> attachments;

                // Agregar swapchain image view (normalmente el color attachment)
                attachments.push_back(m_SwapchainImageViews[i]);

                // Agregar attachments adicionales (ej: depth)
                for (const auto &att : additionalAttachments)
                {
                    attachments.push_back(att);
                }

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPassObj->renderPass;
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = renderPassObj->extent.width;
                framebufferInfo.height = renderPassObj->extent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr,
                                        &renderPassObj->framebuffers[i]) != VK_SUCCESS)
                    throw std::runtime_error("Error creando framebuffer para: " + renderPassObj->config.name);
            }

            std::cout << "Framebuffers creados para '" << renderPassObj->config.name << "'\n";
        }

        std::shared_ptr<Shader> CreateShaderWithRenderPass(const ShaderConfig &config,
                                                           std::shared_ptr<RenderPassObject> renderPassObj)
        {
            auto shader = std::make_shared<Shader>(config);
            CreateShaderPipeline(shader, renderPassObj->renderPass);
            return shader;
        }

        void BeginRenderPassInCommandBuffer(VkCommandBuffer cmd,
                                            std::shared_ptr<RenderPassObject> renderPassObj,
                                            uint32_t framebufferIndex,
                                            const std::vector<VkClearValue> &clearValues)
        {
            if (framebufferIndex >= renderPassObj->framebuffers.size())
                throw std::runtime_error("Índice de framebuffer inválido");

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPassObj->renderPass;
            renderPassInfo.framebuffer = renderPassObj->framebuffers[framebufferIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = renderPassObj->extent;
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        }

        // Dibujar con un render pass específico
        void DrawFrameWithRenderPass(std::shared_ptr<RenderPassObject> renderPassObj,
                                     const std::vector<RenderObject> &objects,
                                     const std::vector<VkClearValue> &clearValues,
                                     std::function<void(VkCommandBuffer)> additionalCommands = nullptr)
        {
            if (m_Device == VK_NULL_HANDLE || m_Swapchain == VK_NULL_HANDLE)
                return;

            vkWaitForFences(m_Device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult res = vkAcquireNextImageKHR(
                m_Device, m_Swapchain, UINT64_MAX,
                m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

            if (res == VK_ERROR_OUT_OF_DATE_KHR)
            {
                RecreateSwapchain();
                return;
            }

            vkResetFences(m_Device, 1, &m_InFlightFence);

            // Usar un command buffer temporal
            VkCommandBuffer cmd = m_CommandBuffers[imageIndex];
            vkResetCommandBuffer(cmd, 0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(cmd, &beginInfo);

            // Comenzar render pass personalizado
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPassObj->renderPass;
            renderPassInfo.framebuffer = renderPassObj->framebuffers[imageIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = renderPassObj->extent;
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Dibujar objetos
            for (const auto &obj : objects)
            {
                if (!obj.mesh || !obj.material || !obj.material->shader)
                    continue;

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.material->shader->pipeline);

                VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        obj.material->shader->pipelineLayout,
                                        0, 1, &obj.material->descriptorSet, 0, nullptr);

                vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()), 1, 0, 0, 0);
            }

            // Comandos adicionales (ej: ImGui)
            if (additionalCommands)
            {
                additionalCommands(cmd);
            }

            vkCmdEndRenderPass(cmd);
            vkEndCommandBuffer(cmd);

            // Submit y present
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo si{};
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.waitSemaphoreCount = 1;
            si.pWaitSemaphores = &m_ImageAvailableSemaphore;
            si.pWaitDstStageMask = &waitStage;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &cmd;
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores = &m_RenderFinishedSemaphore;

            vkQueueSubmit(m_GraphicsQueue, 1, &si, m_InFlightFence);

            VkPresentInfoKHR pi{};
            pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.waitSemaphoreCount = 1;
            pi.pWaitSemaphores = &m_RenderFinishedSemaphore;
            pi.swapchainCount = 1;
            pi.pSwapchains = &m_Swapchain;
            pi.pImageIndices = &imageIndex;

            vkQueuePresentKHR(m_PresentQueue, &pi);
        }

        // Limpiar render passes personalizados
        void CleanupCustomRenderPasses()
        {
            for (auto &rp : m_CustomRenderPasses)
            {
                for (auto fb : rp->framebuffers)
                {
                    if (fb != VK_NULL_HANDLE)
                        vkDestroyFramebuffer(m_Device, fb, nullptr);
                }
                rp->framebuffers.clear();

                if (rp->renderPass != VK_NULL_HANDLE)
                {
                    vkDestroyRenderPass(m_Device, rp->renderPass, nullptr);
                    rp->renderPass = VK_NULL_HANDLE;
                }
            }
            m_CustomRenderPasses.clear();
        }

        void AddRenderObject(const RenderObject &obj)
        {
            m_RenderObjects.push_back(obj);
            m_NeedCommandBufferRebuild = true;
        }

        void UpdateMaterialUBO(Material *material, const UniformBufferObject &ubo)
        {
            material->ubo = ubo;
            void *data;
            vkMapMemory(m_Device, material->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
            memcpy(data, &ubo, sizeof(UniformBufferObject));
            vkUnmapMemory(m_Device, material->uniformBufferMemory);
        }

        void ClearRenderObjects()
        {
            m_RenderObjects.clear();
            m_NeedCommandBufferRebuild = true;
        }

        void NotifyFramebufferResized()
        {
            m_FramebufferResized = true;
        }

        bool DrawFrame(std::function<void(VkCommandBuffer)> imguiRenderCallback = nullptr)
        {
            if (m_Device == VK_NULL_HANDLE || m_Swapchain == VK_NULL_HANDLE)
                return false;

            if (m_FramebufferResized)
            {
                m_FramebufferResized = false;
                RecreateSwapchain();
                return true;
            }

            if (m_NeedCommandBufferRebuild)
            {
                vkDeviceWaitIdle(m_Device);
                m_NeedCommandBufferRebuild = false;
            }

            vkWaitForFences(m_Device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult res = vkAcquireNextImageKHR(
                m_Device, m_Swapchain, UINT64_MAX,
                m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

            if (res == VK_ERROR_OUT_OF_DATE_KHR)
            {
                RecreateSwapchain();
                return true;
            }
            else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error("Error adquiriendo imagen");
            }

            if (imageIndex >= m_CommandBuffers.size())
            {
                RecreateSwapchain();
                return true;
            }

            vkResetFences(m_Device, 1, &m_InFlightFence);

            vkResetCommandBuffer(m_CommandBuffers[imageIndex], 0);
            RecordCommandBuffer(m_CommandBuffers[imageIndex], imageIndex, imguiRenderCallback);

            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            VkSubmitInfo si{};
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.waitSemaphoreCount = 1;
            si.pWaitSemaphores = &m_ImageAvailableSemaphore;
            si.pWaitDstStageMask = &waitStage;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &m_CommandBuffers[imageIndex];
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores = &m_RenderFinishedSemaphore;

            if (vkQueueSubmit(m_GraphicsQueue, 1, &si, m_InFlightFence) != VK_SUCCESS)
                throw std::runtime_error("Error en vkQueueSubmit");

            VkPresentInfoKHR pi{};
            pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.waitSemaphoreCount = 1;
            pi.pWaitSemaphores = &m_RenderFinishedSemaphore;
            pi.swapchainCount = 1;
            pi.pSwapchains = &m_Swapchain;
            pi.pImageIndices = &imageIndex;

            res = vkQueuePresentKHR(m_PresentQueue, &pi);
            if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
            {
                RecreateSwapchain();
                return true;
            }
            else if (res != VK_SUCCESS)
            {
                throw std::runtime_error("Error en vkQueuePresentKHR");
            }

            return false;
        }

        void WaitIdle()
        {
            if (m_Device != VK_NULL_HANDLE)
            {
                vkDeviceWaitIdle(m_Device);
            }
        }

        void ClearRenderObjectsSafe()
        {
            vkDeviceWaitIdle(m_Device);
            m_RenderObjects.clear();
            m_NeedCommandBufferRebuild = true;
        }

        void AddRenderObjectSafe(const RenderObject &obj)
        {
            vkDeviceWaitIdle(m_Device);
            m_RenderObjects.push_back(obj);
            m_NeedCommandBufferRebuild = true;
        }

        std::vector<RenderObject> GetRenderObjects() const
        {
            return m_RenderObjects;
        }

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

        GFX(const GFX &) = delete;
        GFX &operator=(const GFX &) = delete;

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

        static std::vector<char> ReadFile(const std::string &filename)
        {
            std::ifstream file(filename, std::ios::binary | std::ios::ate);
            if (!file)
                throw std::runtime_error("No se pudo abrir: " + filename);

            size_t size = (size_t)file.tellg();
            std::vector<char> buffer(size);
            file.seekg(0);
            file.read(buffer.data(), size);
            return buffer;
        }

        void CreateImage(uint32_t width, uint32_t height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         VkImage &image, VkDeviceMemory &memory)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS)
                throw std::runtime_error("Error creando imagen");

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(m_Device, image, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, properties);

            if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
                throw std::runtime_error("Error allocando memoria de imagen");

            vkBindImageMemory(m_Device, image, memory, 0);
        }

        VkImageView CreateImageView(VkImage image, VkFormat format,
                                    VkImageAspectFlags aspectFlags)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
                throw std::runtime_error("Error creando image view");

            return imageView;
        }

        VkRenderPass CreateOffscreenRenderPass(VkFormat colorFormat, VkFormat depthFormat)
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = colorFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = depthFormat;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorRef{};
            colorRef.attachment = 0;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthRef{};
            depthRef.attachment = 1;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorRef;
            subpass.pDepthStencilAttachment = &depthRef;

            std::array<VkSubpassDependency, 2> dependencies;

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            VkRenderPass renderPass;
            if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
                throw std::runtime_error("Error creando offscreen render pass");

            return renderPass;
        }

        VkCommandBuffer BeginSingleTimeCommands()
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer cmd;
            vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(cmd, &beginInfo);

            return cmd;
        }

        void EndSingleTimeCommands(VkCommandBuffer cmd)
        {
            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;

            vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(m_GraphicsQueue);

            vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
        }

        void TransitionImageLayout(VkImage image, VkFormat format,
                                   VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            VkCommandBuffer cmd = BeginSingleTimeCommands();
            TransitionImageLayoutCmd(cmd, image, format, oldLayout, newLayout);
            EndSingleTimeCommands(cmd);
        }

        void TransitionImageLayoutCmd(VkCommandBuffer cmd, VkImage image, VkFormat format,
                                      VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags srcStage;
            VkPipelineStageFlags dstStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                     newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
                     newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else
            {
                throw std::invalid_argument("Transición de layout no soportada");
            }

            vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr,
                                 1, &barrier);
        }

        VkShaderModule CreateShaderModule(const std::vector<char> &code)
        {
            VkShaderModuleCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ci.codeSize = code.size();
            ci.pCode = reinterpret_cast<const uint32_t *>(code.data());

            VkShaderModule module;
            if (vkCreateShaderModule(m_Device, &ci, nullptr, &module) != VK_SUCCESS)
                throw std::runtime_error("Error creando shader module");

            return module;
        }

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props)
        {
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);

            for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) &&
                    (memProps.memoryTypes[i].propertyFlags & props) == props)
                {
                    return i;
                }
            }

            throw std::runtime_error("No se encontró tipo de memoria adecuado");
        }

        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags props, VkBuffer &buffer,
                          VkDeviceMemory &memory)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
                throw std::runtime_error("Error creando buffer");

            VkMemoryRequirements memReqs;
            vkGetBufferMemoryRequirements(m_Device, buffer, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, props);

            if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
                throw std::runtime_error("Error allocando memoria de buffer");

            vkBindBufferMemory(m_Device, buffer, memory, 0);
        }

        void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer cmd;
            vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(cmd, &beginInfo);

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;

            vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(m_GraphicsQueue);

            vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
        }

        void InitVulkan()
        {
            CreateInstance();
            CreateSurface();
            PickPhysicalDevice();
            CreateLogicalDevice();
            CreateSwapchain(false);
            CreateImageViews();
            CreateDepthResources();
            CreateRenderPass();
            CreateDescriptorPool();
            CreateFramebuffers();
            CreateCommandPool();
            CreateCommandBuffers();
            CreateSyncObjects();
        }

        void CreateInstance()
        {
            VkApplicationInfo app{};
            app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app.pApplicationName = "Mantrax Vulkan";
            app.apiVersion = VK_API_VERSION_1_3;

            const char *exts[] = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

            VkInstanceCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            ci.pApplicationInfo = &app;
            ci.enabledExtensionCount = 2;
            ci.ppEnabledExtensionNames = exts;

            if (vkCreateInstance(&ci, nullptr, &m_Instance) != VK_SUCCESS)
                throw std::runtime_error("Error creando instancia Vulkan");
        }

        void CreateSurface()
        {
            VkWin32SurfaceCreateInfoKHR s{};
            s.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            s.hinstance = m_hInstance;
            s.hwnd = m_hWnd;

            if (vkCreateWin32SurfaceKHR(m_Instance, &s, nullptr, &m_Surface) != VK_SUCCESS)
                throw std::runtime_error("Error creando surface");
        }

        void PickPhysicalDevice()
        {
            uint32_t count = 0;
            vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
            if (count == 0)
                throw std::runtime_error("No hay dispositivos con Vulkan");

            std::vector<VkPhysicalDevice> devices(count);
            vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

            for (VkPhysicalDevice dev : devices)
            {
                uint32_t qCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
                std::vector<VkQueueFamilyProperties> props(qCount);
                vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, props.data());

                uint32_t gfx = UINT32_MAX;
                uint32_t present = UINT32_MAX;

                for (uint32_t i = 0; i < qCount; ++i)
                {
                    if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        gfx = i;

                    VkBool32 support = VK_FALSE;
                    vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_Surface, &support);
                    if (support)
                        present = i;
                }

                if (gfx != UINT32_MAX && present != UINT32_MAX)
                {
                    m_PhysicalDevice = dev;
                    m_GraphicsQueueFamily = gfx;
                    m_PresentQueueFamily = present;
                    return;
                }
            }

            throw std::runtime_error("No GPU válida encontrada");
        }

        void CreateLogicalDevice()
        {
            float priority = 1.0f;

            std::vector<VkDeviceQueueCreateInfo> queues;
            std::set<uint32_t> uniqueFamilies = {m_GraphicsQueueFamily, m_PresentQueueFamily};

            for (uint32_t family : uniqueFamilies)
            {
                VkDeviceQueueCreateInfo q{};
                q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                q.queueFamilyIndex = family;
                q.queueCount = 1;
                q.pQueuePriorities = &priority;
                queues.push_back(q);
            }

            const char *exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

            VkDeviceCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            ci.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
            ci.pQueueCreateInfos = queues.data();
            ci.enabledExtensionCount = 1;
            ci.ppEnabledExtensionNames = exts;

            if (vkCreateDevice(m_PhysicalDevice, &ci, nullptr, &m_Device) != VK_SUCCESS)
                throw std::runtime_error("Error creando dispositivo lógico");

            vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
            vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);
        }

        void CreateSwapchain(bool enableVSync)
        {
            VkSurfaceCapabilitiesKHR caps{};
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &caps);

            uint32_t fCount = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &fCount, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(fCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &fCount, formats.data());

            VkSurfaceFormatKHR chosen = formats[0];
            for (auto &f : formats)
            {
                if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    chosen = f;
                    break;
                }
            }

            VkExtent2D extent;
            if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                extent = caps.currentExtent;
            }
            else
            {
                extent = m_SwapchainExtent;
                extent.width = std::max(caps.minImageExtent.width,
                                        std::min(caps.maxImageExtent.width, extent.width));
                extent.height = std::max(caps.minImageExtent.height,
                                         std::min(caps.maxImageExtent.height, extent.height));
            }

            uint32_t imgCount = caps.minImageCount + 1;
            if (caps.maxImageCount > 0 && imgCount > caps.maxImageCount)
                imgCount = caps.maxImageCount;

            VkSwapchainCreateInfoKHR ci{};
            ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            ci.surface = m_Surface;
            ci.minImageCount = imgCount;
            ci.imageFormat = chosen.format;
            ci.imageColorSpace = chosen.colorSpace;
            ci.imageExtent = extent;
            ci.imageArrayLayers = 1;
            ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t fams[] = {m_GraphicsQueueFamily, m_PresentQueueFamily};
            if (m_GraphicsQueueFamily != m_PresentQueueFamily)
            {
                ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                ci.queueFamilyIndexCount = 2;
                ci.pQueueFamilyIndices = fams;
            }
            else
            {
                ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            ci.preTransform = caps.currentTransform;
            ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            if (enableVSync)
            {
                ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            }
            else
            {
                ci.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }

            ci.clipped = VK_TRUE;
            ci.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(m_Device, &ci, nullptr, &m_Swapchain) != VK_SUCCESS)
                throw std::runtime_error("Error creando swapchain");

            vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imgCount, nullptr);
            m_SwapchainImages.resize(imgCount);
            vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imgCount, m_SwapchainImages.data());

            m_SwapchainImageFormat = chosen.format;
            m_SwapchainExtent = extent;
        }

        void CreateImageViews()
        {
            m_SwapchainImageViews.resize(m_SwapchainImages.size());

            for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
            {
                VkImageViewCreateInfo ci{};
                ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                ci.image = m_SwapchainImages[i];
                ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
                ci.format = m_SwapchainImageFormat;
                ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                ci.subresourceRange.baseMipLevel = 0;
                ci.subresourceRange.levelCount = 1;
                ci.subresourceRange.baseArrayLayer = 0;
                ci.subresourceRange.layerCount = 1;
                if (vkCreateImageView(m_Device, &ci, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
                    throw std::runtime_error("Error creando image view");
            }
        }

        VkFormat FindDepthFormat()
        {
            std::vector<VkFormat> candidates = {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT};

            for (VkFormat format : candidates)
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

                if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                {
                    return format;
                }
            }

            throw std::runtime_error("No se encontró formato de depth adecuado");
        }

        void CreateDepthResources()
        {
            m_DepthFormat = FindDepthFormat();

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = m_SwapchainExtent.width;
            imageInfo.extent.height = m_SwapchainExtent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = m_DepthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(m_Device, &imageInfo, nullptr, &m_DepthImage) != VK_SUCCESS)
                throw std::runtime_error("Error creando depth image");

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(m_Device, m_DepthImage, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_DepthImageMemory) != VK_SUCCESS)
                throw std::runtime_error("Error allocando memoria depth");

            vkBindImageMemory(m_Device, m_DepthImage, m_DepthImageMemory, 0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_DepthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_DepthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_DepthImageView) != VK_SUCCESS)
                throw std::runtime_error("Error creando depth image view");
        }

        void CreateRenderPass()
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = m_SwapchainImageFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = m_DepthFormat;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorRef{};
            colorRef.attachment = 0;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthRef{};
            depthRef.attachment = 1;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorRef;
            subpass.pDepthStencilAttachment = &depthRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
                throw std::runtime_error("Error creando render pass");
        }

        void CreateDescriptorPool()
        {
            std::array<VkDescriptorPoolSize, 2> poolSizes{};

            poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = 1000;

            poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[1].descriptorCount = 1000;

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = 2000;

            if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
                throw std::runtime_error("Error creando descriptor pool");
        }

        void CreateFramebuffers()
        {
            m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

            for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
            {
                std::array<VkImageView, 2> attachments = {
                    m_SwapchainImageViews[i],
                    m_DepthImageView};

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = m_RenderPass;
                framebufferInfo.attachmentCount = 2;
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = m_SwapchainExtent.width;
                framebufferInfo.height = m_SwapchainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr,
                                        &m_SwapchainFramebuffers[i]) != VK_SUCCESS)
                    throw std::runtime_error("Error creando framebuffer");
            }
        }

        void CreateCommandPool()
        {
            VkCommandPoolCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            ci.queueFamilyIndex = m_GraphicsQueueFamily;
            ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if (vkCreateCommandPool(m_Device, &ci, nullptr, &m_CommandPool) != VK_SUCCESS)
                throw std::runtime_error("Error creando command pool");
        }

        void CreateCommandBuffers()
        {
            m_CommandBuffers.resize(m_SwapchainFramebuffers.size());

            VkCommandBufferAllocateInfo ai{};
            ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            ai.commandPool = m_CommandPool;
            ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            ai.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

            if (vkAllocateCommandBuffers(m_Device, &ai, m_CommandBuffers.data()) != VK_SUCCESS)
                throw std::runtime_error("Error creando command buffers");
        }

        void CreateSyncObjects()
        {
            VkSemaphoreCreateInfo si{};
            si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fi{};
            fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateSemaphore(m_Device, &si, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &si, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fi, nullptr, &m_InFlightFence) != VK_SUCCESS)
                throw std::runtime_error("Error creando objetos de sincronización");
        }

        void CreateShaderPipeline(std::shared_ptr<Shader> shader, VkRenderPass renderPass = VK_NULL_HANDLE)
        {
            auto vertCode = ReadFile(shader->config.vertexShaderPath);
            auto fragCode = ReadFile(shader->config.fragmentShaderPath);

            VkShaderModule vert = CreateShaderModule(vertCode);
            VkShaderModule frag = CreateShaderModule(fragCode);

            VkPipelineShaderStageCreateInfo vs{};
            vs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vs.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vs.module = vert;
            vs.pName = "main";

            VkPipelineShaderStageCreateInfo fs{};
            fs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fs.module = frag;
            fs.pName = "main";

            VkPipelineShaderStageCreateInfo stages[] = {vs, fs};

            VkPipelineVertexInputStateCreateInfo vin{};
            vin.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vin.vertexBindingDescriptionCount = 1;
            vin.pVertexBindingDescriptions = &shader->config.vertexBinding;
            vin.vertexAttributeDescriptionCount = static_cast<uint32_t>(shader->config.vertexAttributes.size());
            vin.pVertexAttributeDescriptions = shader->config.vertexAttributes.data();

            VkPipelineInputAssemblyStateCreateInfo ia{};
            ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            ia.topology = shader->config.topology;

            VkViewport vp{};
            vp.x = 0.0f;
            vp.y = 0.0f;
            vp.width = (float)m_SwapchainExtent.width;
            vp.height = (float)m_SwapchainExtent.height;
            vp.minDepth = 0.0f;
            vp.maxDepth = 1.0f;

            VkRect2D sc{{0, 0}, m_SwapchainExtent};

            VkPipelineViewportStateCreateInfo vpst{};
            vpst.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vpst.viewportCount = 1;
            vpst.pViewports = &vp;
            vpst.scissorCount = 1;
            vpst.pScissors = &sc;

            VkPipelineRasterizationStateCreateInfo rs{};
            rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rs.polygonMode = shader->config.polygonMode;
            rs.cullMode = shader->config.cullMode;
            rs.frontFace = shader->config.frontFace;
            rs.lineWidth = 1.0f;

            VkPipelineMultisampleStateCreateInfo ms{};
            ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState cba{};
            cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            cba.blendEnable = shader->config.blendEnable ? VK_TRUE : VK_FALSE;

            if (shader->config.blendEnable)
            {
                cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                cba.colorBlendOp = VK_BLEND_OP_ADD;
                cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                cba.alphaBlendOp = VK_BLEND_OP_ADD;
            }

            VkPipelineColorBlendStateCreateInfo cb{};
            cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cb.attachmentCount = 1;
            cb.pAttachments = &cba;

            VkDescriptorSetLayoutBinding uboBinding{};
            uboBinding.binding = 0;
            uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboBinding.descriptorCount = 1;
            uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &uboBinding;

            if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &shader->descriptorSetLayout) != VK_SUCCESS)
                throw std::runtime_error("Error creando descriptor set layout");

            VkPipelineLayoutCreateInfo pl{};
            pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pl.setLayoutCount = 1;
            pl.pSetLayouts = &shader->descriptorSetLayout;

            if (vkCreatePipelineLayout(m_Device, &pl, nullptr, &shader->pipelineLayout) != VK_SUCCESS)
                throw std::runtime_error("Error creando pipeline layout");

            VkPipelineDepthStencilStateCreateInfo ds{};
            ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            ds.depthTestEnable = shader->config.depthTestEnable ? VK_TRUE : VK_FALSE;
            ds.depthWriteEnable = VK_TRUE;
            ds.depthCompareOp = VK_COMPARE_OP_LESS;

            VkGraphicsPipelineCreateInfo pi{};
            pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pi.stageCount = 2;
            pi.pStages = stages;
            pi.pVertexInputState = &vin;
            pi.pInputAssemblyState = &ia;
            pi.pViewportState = &vpst;
            pi.pRasterizationState = &rs;
            pi.pMultisampleState = &ms;
            pi.pColorBlendState = &cb;
            pi.pDepthStencilState = &ds;
            pi.layout = shader->pipelineLayout;
            pi.renderPass = (renderPass != VK_NULL_HANDLE) ? renderPass : m_RenderPass;
            pi.subpass = 0;

            if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pi, nullptr, &shader->pipeline) != VK_SUCCESS)
                throw std::runtime_error("Error creando pipeline");

            vkDestroyShaderModule(m_Device, vert, nullptr);
            vkDestroyShaderModule(m_Device, frag, nullptr);

            bool exists = false;
            for (const auto &s : m_AllShaders)
            {
                if (s == shader)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
            {
                m_AllShaders.push_back(shader);
            }
        }

        void CreateVertexBuffer(std::shared_ptr<Mesh> mesh)
        {
            VkDeviceSize size = sizeof(Vertex) * mesh->vertices.size();

            VkBuffer staging;
            VkDeviceMemory stagingMem;

            CreateBuffer(size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         staging, stagingMem);

            void *data;
            vkMapMemory(m_Device, stagingMem, 0, size, 0, &data);
            memcpy(data, mesh->vertices.data(), size);
            vkUnmapMemory(m_Device, stagingMem);

            CreateBuffer(size,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         mesh->vertexBuffer, mesh->vertexBufferMemory);

            CopyBuffer(staging, mesh->vertexBuffer, size);

            vkDestroyBuffer(m_Device, staging, nullptr);
            vkFreeMemory(m_Device, stagingMem, nullptr);
        }

        void CreateIndexBuffer(std::shared_ptr<Mesh> mesh)
        {
            VkDeviceSize size = sizeof(uint32_t) * mesh->indices.size();

            VkBuffer staging;
            VkDeviceMemory stagingMem;

            CreateBuffer(size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         staging, stagingMem);

            void *data;
            vkMapMemory(m_Device, stagingMem, 0, size, 0, &data);
            memcpy(data, mesh->indices.data(), size);
            vkUnmapMemory(m_Device, stagingMem);

            CreateBuffer(size,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         mesh->indexBuffer, mesh->indexBufferMemory);

            CopyBuffer(staging, mesh->indexBuffer, size);

            vkDestroyBuffer(m_Device, staging, nullptr);
            vkFreeMemory(m_Device, stagingMem, nullptr);
        }

        void CreateUniformBuffer(std::shared_ptr<Material> material)
        {
            VkDeviceSize size = sizeof(UniformBufferObject);

            CreateBuffer(size,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         material->uniformBuffer, material->uniformBufferMemory);
        }

        void CreateDescriptorSet(std::shared_ptr<Material> material)
        {
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &material->shader->descriptorSetLayout;

            if (vkAllocateDescriptorSets(m_Device, &allocInfo, &material->descriptorSet) != VK_SUCCESS)
                throw std::runtime_error("Error creando descriptor set");

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = material->uniformBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = material->descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
        }

        void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t index,
                                 std::function<void(VkCommandBuffer)> imguiRenderCallback = nullptr)
        {
            if (cmd == VK_NULL_HANDLE || index >= m_SwapchainFramebuffers.size())
            {
                throw std::runtime_error("Command buffer o índice inválido");
            }

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("Error comenzando command buffer");

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = m_Config.clearColor;
            clearValues[1].depthStencil = {1.0f, 0};

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_RenderPass;
            renderPassInfo.framebuffer = m_SwapchainFramebuffers[index];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = m_SwapchainExtent;
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            for (const auto &obj : m_RenderObjects)
            {
                if (!obj.mesh || !obj.material || !obj.material->shader)
                    continue;

                if (!obj.mesh->vertexBuffer || !obj.mesh->indexBuffer)
                    continue;

                if (!obj.material->shader->pipeline)
                    continue;

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.material->shader->pipeline);

                VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        obj.material->shader->pipelineLayout,
                                        0, 1, &obj.material->descriptorSet, 0, nullptr);

                vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()), 1, 0, 0, 0);
            }

            if (imguiRenderCallback)
            {
                imguiRenderCallback(cmd);
            }

            vkCmdEndRenderPass(cmd);

            if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
                throw std::runtime_error("Error finalizando command buffer");
        }

        void CleanupSwapchain()
        {
            for (auto fb : m_SwapchainFramebuffers)
                vkDestroyFramebuffer(m_Device, fb, nullptr);
            m_SwapchainFramebuffers.clear();

            if (!m_CommandBuffers.empty())
            {
                vkFreeCommandBuffers(m_Device, m_CommandPool,
                                     static_cast<uint32_t>(m_CommandBuffers.size()),
                                     m_CommandBuffers.data());
                m_CommandBuffers.clear();
            }
            for (auto iv : m_SwapchainImageViews)
                vkDestroyImageView(m_Device, iv, nullptr);
            m_SwapchainImageViews.clear();

            if (m_DepthImageView)
            {
                vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
                m_DepthImageView = VK_NULL_HANDLE;
            }
            if (m_DepthImage)
            {
                vkDestroyImage(m_Device, m_DepthImage, nullptr);
                m_DepthImage = VK_NULL_HANDLE;
            }
            if (m_DepthImageMemory)
            {
                vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);
                m_DepthImageMemory = VK_NULL_HANDLE;
            }

            if (m_RenderPass)
            {
                vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
                m_RenderPass = VK_NULL_HANDLE;
            }

            if (m_Swapchain)
            {
                vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
                m_Swapchain = VK_NULL_HANDLE;
            }
        }

        void RecreateSwapchainWithCustomRenderPasses()
        {
            if (m_Device == VK_NULL_HANDLE)
                return;

            vkDeviceWaitIdle(m_Device);

            // Guardar configuraciones de render passes
            std::vector<std::shared_ptr<RenderPassObject>> oldRenderPasses = m_CustomRenderPasses;

            // Limpiar
            CleanupSwapchain();
            CleanupCustomRenderPasses();

            // Recrear swapchain base
            CreateSwapchain(false);
            CreateImageViews();
            CreateDepthResources();
            CreateRenderPass();
            CreateFramebuffers();
            CreateCommandBuffers();

            // Recrear render passes personalizados
            for (auto &oldRP : oldRenderPasses)
            {
                auto newRP = CreateRenderPass(oldRP->config);
                newRP->extent = m_SwapchainExtent;

                // Recrear framebuffers (necesitarás adaptar esto según tus attachments)
                CreateFramebuffersForRenderPass(newRP, {m_DepthImageView});
            }

            // Recrear pipelines
            auto shadersToRecreate = m_AllShaders;
            for (auto &shader : shadersToRecreate)
            {
                if (shader->pipeline != VK_NULL_HANDLE)
                    vkDestroyPipeline(m_Device, shader->pipeline, nullptr);
                if (shader->pipelineLayout != VK_NULL_HANDLE)
                    vkDestroyPipelineLayout(m_Device, shader->pipelineLayout, nullptr);
                if (shader->descriptorSetLayout != VK_NULL_HANDLE)
                    vkDestroyDescriptorSetLayout(m_Device, shader->descriptorSetLayout, nullptr);

                CreateShaderPipeline(shader);
            }

            m_NeedCommandBufferRebuild = true;
        }

        void RecreateSwapchain()
        {
            if (m_Device == VK_NULL_HANDLE)
                return;

            RECT rect;
            GetClientRect(m_hWnd, &rect);
            uint32_t width = rect.right - rect.left;
            uint32_t height = rect.bottom - rect.top;

            while (width == 0 || height == 0)
            {
                GetClientRect(m_hWnd, &rect);
                width = rect.right - rect.left;
                height = rect.bottom - rect.top;
                Sleep(10);
            }

            vkDeviceWaitIdle(m_Device);

            m_SwapchainExtent.width = width;
            m_SwapchainExtent.height = height;

            // NUEVO: Limpiar render passes personalizados primero
            CleanupCustomRenderPasses();

            for (auto &obj : m_RenderObjects)
            {
                if (obj.material && obj.material->descriptorSet != VK_NULL_HANDLE)
                {
                    obj.material->descriptorSet = VK_NULL_HANDLE;
                }
            }

            CleanupSwapchain();

            CreateSwapchain(false);
            CreateImageViews();
            CreateDepthResources();
            CreateRenderPass();
            CreateFramebuffers();
            CreateCommandBuffers();

            std::vector<RenderPassConfig> savedConfigs;

            auto shadersToRecreate = m_AllShaders;

            for (auto &shader : shadersToRecreate)
            {
                if (shader->pipeline != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(m_Device, shader->pipeline, nullptr);
                    shader->pipeline = VK_NULL_HANDLE;
                }
                if (shader->pipelineLayout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(m_Device, shader->pipelineLayout, nullptr);
                    shader->pipelineLayout = VK_NULL_HANDLE;
                }
                if (shader->descriptorSetLayout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(m_Device, shader->descriptorSetLayout, nullptr);
                    shader->descriptorSetLayout = VK_NULL_HANDLE;
                }

                CreateShaderPipeline(shader);
            }

            for (auto &obj : m_RenderObjects)
            {
                if (obj.material && obj.material->shader)
                {
                    CreateDescriptorSet(obj.material);
                }
            }

            m_NeedCommandBufferRebuild = true;
        }

        void Cleanup()
        {
            if (m_Device == VK_NULL_HANDLE && m_Instance == VK_NULL_HANDLE)
                return;

            if (m_Device != VK_NULL_HANDLE)
                vkDeviceWaitIdle(m_Device);

            for (auto &obj : m_RenderObjects)
            {
                if (obj.mesh)
                {
                    if (obj.mesh->vertexBuffer)
                        vkDestroyBuffer(m_Device, obj.mesh->vertexBuffer, nullptr);
                    if (obj.mesh->vertexBufferMemory)
                        vkFreeMemory(m_Device, obj.mesh->vertexBufferMemory, nullptr);
                    if (obj.mesh->indexBuffer)
                        vkDestroyBuffer(m_Device, obj.mesh->indexBuffer, nullptr);
                    if (obj.mesh->indexBufferMemory)
                        vkFreeMemory(m_Device, obj.mesh->indexBufferMemory, nullptr);
                }

                if (obj.material)
                {
                    if (obj.material->uniformBuffer)
                        vkDestroyBuffer(m_Device, obj.material->uniformBuffer, nullptr);
                    if (obj.material->uniformBufferMemory)
                        vkFreeMemory(m_Device, obj.material->uniformBufferMemory, nullptr);
                }
            }

            for (auto &shader : m_AllShaders)
            {
                if (shader->pipeline)
                    vkDestroyPipeline(m_Device, shader->pipeline, nullptr);
                if (shader->pipelineLayout)
                    vkDestroyPipelineLayout(m_Device, shader->pipelineLayout, nullptr);
                if (shader->descriptorSetLayout)
                    vkDestroyDescriptorSetLayout(m_Device, shader->descriptorSetLayout, nullptr);
            }

            if (m_Device == VK_NULL_HANDLE && m_Instance == VK_NULL_HANDLE)
                return;

            if (m_Device != VK_NULL_HANDLE)
                vkDeviceWaitIdle(m_Device);

            CleanupCustomRenderPasses();

            if (m_InFlightFence)
                vkDestroyFence(m_Device, m_InFlightFence, nullptr);
            if (m_RenderFinishedSemaphore)
                vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
            if (m_ImageAvailableSemaphore)
                vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);

            CleanupSwapchain();

            if (m_DescriptorPool)
                vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

            if (m_CommandPool)
                vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

            if (m_Device)
                vkDestroyDevice(m_Device, nullptr);

            if (m_Surface)
                vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

            if (m_Instance)
                vkDestroyInstance(m_Instance, nullptr);

            m_Device = VK_NULL_HANDLE;
            m_Instance = VK_NULL_HANDLE;
        }
    };
}