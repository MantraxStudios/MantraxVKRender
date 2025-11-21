#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <array>

using namespace DirectX;

namespace MantraxGFX
{
// Macro para verificar VkResult
#define CHECK_VK(result, msg)          \
    if (result != VK_SUCCESS)          \
    {                                  \
        OutputDebugStringA("ERROR: "); \
        OutputDebugStringA(msg);       \
        OutputDebugStringA("\n");      \
        throw std::runtime_error(msg); \
    }

    // ============================================================================
    // ESTRUCTURAS DE DATOS PARA PBR
    // ============================================================================

    struct PBRMaterial
    {
        XMFLOAT4 albedo = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 0.5f;
        float ao = 1.0f;
        float _padding = 0.0f;
    };

    struct Light
    {
        XMFLOAT3 position;
        float intensity;
        XMFLOAT3 color;
        float radius;
    };

    struct CameraData
    {
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT3 position;
        float _padding;
    };

    struct ObjectData
    {
        XMMATRIX model;
        XMMATRIX normalMatrix;
    };

    // ============================================================================
    // BUFFER DE CONSTANTES (Uniform Buffer)
    // ============================================================================

    template <typename T>
    class ConstantBuffer
    {
    public:
        void Create(VkDevice device, VkPhysicalDevice physicalDevice, UINT count = 1)
        {
            this->device = device;
            elementSize = (sizeof(T) + 255) & ~255;
            bufferSize = elementSize * count;

            CreateBuffer(device, physicalDevice, bufferSize,
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &mappedData);
        }

        void Update(const T &data, UINT index = 0)
        {
            if (mappedData)
            {
                memcpy(static_cast<char *>(mappedData) + (index * elementSize), &data, sizeof(T));
            }
        }

        void Destroy()
        {
            if (device != VK_NULL_HANDLE)
            {
                if (mappedData)
                {
                    vkUnmapMemory(device, bufferMemory);
                    mappedData = nullptr;
                }
                if (buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(device, buffer, nullptr);
                }
                if (bufferMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device, bufferMemory, nullptr);
                }
            }
        }

        VkBuffer GetBuffer() const { return buffer; }
        VkDeviceSize GetSize() const { return bufferSize; }

    private:
        void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                          VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            CHECK_VK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer),
                     "Failed to create buffer");

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

            CHECK_VK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory),
                     "Failed to allocate buffer memory");

            vkBindBufferMemory(device, buffer, bufferMemory, 0);
        }

        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }

            throw std::runtime_error("Failed to find suitable memory type");
        }

        VkDevice device = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
        void *mappedData = nullptr;
        VkDeviceSize elementSize = 0;
        VkDeviceSize bufferSize = 0;
    };

    // ============================================================================
    // SHADER
    // ============================================================================

    class Shader
    {
    public:
        enum class Type
        {
            Vertex,
            Fragment,
            Compute
        };

        bool LoadFromFile(VkDevice device, const std::string &filename, Type type)
        {
            this->device = device;
            this->shaderType = type;

            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open())
            {
                OutputDebugStringA("Failed to open shader file: ");
                OutputDebugStringA(filename.c_str());
                OutputDebugStringA("\n");
                return false;
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = buffer.size();
            createInfo.pCode = reinterpret_cast<const uint32_t *>(buffer.data());

            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            {
                OutputDebugStringA("Failed to create shader module\n");
                return false;
            }

            return true;
        }

        void Destroy()
        {
            if (device != VK_NULL_HANDLE && shaderModule != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(device, shaderModule, nullptr);
                shaderModule = VK_NULL_HANDLE;
            }
        }

        VkShaderModule GetModule() const { return shaderModule; }
        Type GetType() const { return shaderType; }

        VkPipelineShaderStageCreateInfo GetStageInfo() const
        {
            VkPipelineShaderStageCreateInfo stageInfo{};
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.module = shaderModule;
            stageInfo.pName = "main";

            switch (shaderType)
            {
            case Type::Vertex:
                stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case Type::Fragment:
                stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case Type::Compute:
                stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            }

            return stageInfo;
        }

    private:
        VkDevice device = VK_NULL_HANDLE;
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        Type shaderType;
    };

    // ============================================================================
    // PIPELINE PBR
    // ============================================================================

    class PBRPipeline
    {
    public:
        void Create(VkDevice device, VkRenderPass renderPass, VkExtent2D extent)
        {
            this->device = device;
            CreateDescriptorSetLayout();
            CreatePipelineLayout();
            CreateDefaultShaders();
            CreateGraphicsPipeline(renderPass, extent);
        }

        void SetCustomShaders(const Shader &vs, const Shader &fs)
        {
            vertexShader = vs;
            fragmentShader = fs;
        }

        void Bind(VkCommandBuffer commandBuffer)
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        }

        void BindDescriptorSet(VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet)
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        }

        void Destroy()
        {
            if (device != VK_NULL_HANDLE)
            {
                if (pipeline != VK_NULL_HANDLE)
                    vkDestroyPipeline(device, pipeline, nullptr);
                if (pipelineLayout != VK_NULL_HANDLE)
                    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
                if (descriptorSetLayout != VK_NULL_HANDLE)
                    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

                vertexShader.Destroy();
                fragmentShader.Destroy();
            }
        }

        VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
        VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        void CreateDescriptorSetLayout()
        {
            std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

            // Camera data
            bindings[0].binding = 0;
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[0].descriptorCount = 1;
            bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            // Object data
            bindings[1].binding = 1;
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[1].descriptorCount = 1;
            bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            // Material data
            bindings[2].binding = 2;
            bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindings[2].descriptorCount = 1;
            bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            // Lights buffer
            bindings[3].binding = 3;
            bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[3].descriptorCount = 1;
            bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            CHECK_VK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout),
                     "Failed to create descriptor set layout");
        }

        void CreatePipelineLayout()
        {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

            CHECK_VK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout),
                     "Failed to create pipeline layout");
        }

        void CreateDefaultShaders()
        {
            // En Vulkan necesitas shaders SPIR-V precompilados
            // Estos deben compilarse con glslangValidator o similar
            // Por ahora dejamos que el usuario proporcione los archivos .spv
            OutputDebugStringA("Default shaders need to be compiled to SPIR-V format\n");
            OutputDebugStringA("Use: glslangValidator -V shader.vert -o vert.spv\n");
        }

        void CreateGraphicsPipeline(VkRenderPass renderPass, VkExtent2D extent)
        {
            VkPipelineShaderStageCreateInfo vertShaderStageInfo = vertexShader.GetStageInfo();
            VkPipelineShaderStageCreateInfo fragShaderStageInfo = fragmentShader.GetStageInfo();

            VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

            // Vertex input
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(float) * 8; // 3 pos + 3 normal + 2 texcoord
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = 0;

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = sizeof(float) * 3;

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = sizeof(float) * 6;

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

            // Input assembly
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            // Viewport
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)extent.width;
            viewport.height = (float)extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = extent;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            // Rasterizer
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            // Multisampling
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // Depth stencil
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;

            // Color blending
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;

            // Pipeline
            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0;

            CHECK_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
                     "Failed to create graphics pipeline");
        }

        VkDevice device = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        Shader vertexShader;
        Shader fragmentShader;
    };

    // ============================================================================
    // CONTEXTO PRINCIPAL
    // ============================================================================

    class Context
    {
    public:
        bool Initialize(HWND hwnd, UINT width, UINT height)
        {
            this->width = width;
            this->height = height;
            this->hwnd = hwnd;

            try
            {
                CreateInstance();
                CreateSurface();
                PickPhysicalDevice();
                CreateLogicalDevice();
                CreateSwapChain();
                CreateCommandPool();
                CreateCommandBuffers();
                CreateSyncObjects();

                OutputDebugStringA("Vulkan initialized successfully\n");
                return true;
            }
            catch (const std::exception &e)
            {
                OutputDebugStringA("Initialization failed: ");
                OutputDebugStringA(e.what());
                OutputDebugStringA("\n");
                return false;
            }
        }

        void WaitForGPU()
        {
            vkDeviceWaitIdle(device);
        }

        void BeginFrame()
        {
            vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

            VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                                    imageAvailableSemaphores[currentFrame],
                                                    VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                // Recreate swap chain
                return;
            }

            vkResetFences(device, 1, &inFlightFences[currentFrame]);
            vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        }

        void EndFrame()
        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

            VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            CHECK_VK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]),
                     "Failed to submit draw command buffer");

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;

            vkQueuePresentKHR(presentQueue, &presentInfo);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        void Cleanup()
        {
            vkDeviceWaitIdle(device);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device, inFlightFences[i], nullptr);
            }

            vkDestroyCommandPool(device, commandPool, nullptr);

            for (auto imageView : swapChainImageViews)
            {
                vkDestroyImageView(device, imageView, nullptr);
            }

            vkDestroySwapchainKHR(device, swapChain, nullptr);
            vkDestroyDevice(device, nullptr);
            vkDestroySurfaceKHR(instance, surface, nullptr);
            vkDestroyInstance(instance, nullptr);
        }

        VkDevice GetDevice() const { return device; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        VkCommandBuffer GetCommandBuffer() const { return commandBuffers[currentFrame]; }
        uint32_t GetImageIndex() const { return imageIndex; }

    private:
        void CreateInstance()
        {
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "MantraxGFX";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "MantraxGFX";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_2;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            std::vector<const char *> extensions = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.enabledLayerCount = 0;

            CHECK_VK(vkCreateInstance(&createInfo, nullptr, &instance),
                     "Failed to create Vulkan instance");
        }

        void CreateSurface()
        {
            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hwnd = hwnd;
            createInfo.hinstance = GetModuleHandle(nullptr);

            CHECK_VK(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface),
                     "Failed to create window surface");
        }

        void PickPhysicalDevice()
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

            if (deviceCount == 0)
            {
                throw std::runtime_error("Failed to find GPUs with Vulkan support");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            physicalDevice = devices[0]; // Usar primera GPU disponible
        }

        void CreateLogicalDevice()
        {
            QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            float queuePriority = 1.0f;

            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);

            VkPhysicalDeviceFeatures deviceFeatures{};

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = &deviceFeatures;

            std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME};

            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();

            CHECK_VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device),
                     "Failed to create logical device");

            vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
            vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
        }

        void CreateSwapChain()
        {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

            VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
            uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

            if (indices.graphicsFamily != indices.presentFamily)
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            CHECK_VK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain),
                     "Failed to create swap chain");

            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
            swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

            swapChainImageFormat = surfaceFormat.format;
            swapChainExtent = extent;

            CreateImageViews();
        }

        void CreateImageViews()
        {
            swapChainImageViews.resize(swapChainImages.size());

            for (size_t i = 0; i < swapChainImages.size(); i++)
            {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = swapChainImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = swapChainImageFormat;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                CHECK_VK(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]),
                         "Failed to create image views");
            }
        }

        void CreateCommandPool()
        {
            QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

            CHECK_VK(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool),
                     "Failed to create command pool");
        }

        void CreateCommandBuffers()
        {
            commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

            CHECK_VK(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()),
                     "Failed to allocate command buffers");
        }

        void CreateSyncObjects()
        {
            imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                CHECK_VK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]),
                         "Failed to create semaphores");
                CHECK_VK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]),
                         "Failed to create semaphores");
                CHECK_VK(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]),
                         "Failed to create fence");
            }
        }

        struct QueueFamilyIndices
        {
            uint32_t graphicsFamily = 0;
            uint32_t presentFamily = 0;
        };

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
        {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto &queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.graphicsFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport)
                {
                    indices.presentFamily = i;
                }

                i++;
            }

            return indices;
        }

        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
        {
            SwapChainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

            if (formatCount != 0)
            {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0)
            {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
        {
            for (const auto &availableFormat : availableFormats)
            {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }

        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
        {
            for (const auto &availablePresentMode : availablePresentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
        {
            if (capabilities.currentExtent.width != UINT32_MAX)
            {
                return capabilities.currentExtent;
            }
            else
            {
                VkExtent2D actualExtent = {width, height};

                actualExtent.width = std::max(capabilities.minImageExtent.width,
                                              std::min(capabilities.maxImageExtent.width, actualExtent.width));
                actualExtent.height = std::max(capabilities.minImageExtent.height,
                                               std::min(capabilities.maxImageExtent.height, actualExtent.height));

                return actualExtent;
            }
        }

        static const int MAX_FRAMES_IN_FLIGHT = 2;

        HWND hwnd = nullptr;
        UINT width = 0;
        UINT height = 0;

        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;

        uint32_t currentFrame = 0;
        uint32_t imageIndex = 0;
    };

    // ============================================================================
    // CÁMARA
    // ============================================================================

    class Camera
    {
    public:
        Camera()
            : position(0.0f, 0.0f, -5.0f),
              target(0.0f, 0.0f, 0.0f),
              up(0.0f, 1.0f, 0.0f),
              fov(XM_PIDIV4),
              aspectRatio(16.0f / 9.0f),
              nearPlane(0.1f),
              farPlane(100.0f)
        {
            UpdateMatrices();
        }

        void SetPosition(float x, float y, float z)
        {
            position = XMFLOAT3(x, y, z);
            UpdateMatrices();
        }

        void SetPosition(const XMFLOAT3 &pos)
        {
            position = pos;
            UpdateMatrices();
        }

        void SetTarget(float x, float y, float z)
        {
            target = XMFLOAT3(x, y, z);
            UpdateMatrices();
        }

        void SetTarget(const XMFLOAT3 &tgt)
        {
            target = tgt;
            UpdateMatrices();
        }

        void SetUp(float x, float y, float z)
        {
            up = XMFLOAT3(x, y, z);
            UpdateMatrices();
        }

        void SetPerspective(float fovRadians, float aspect, float nearZ, float farZ)
        {
            fov = fovRadians;
            aspectRatio = aspect;
            nearPlane = nearZ;
            farPlane = farZ;
            UpdateMatrices();
        }

        void SetOrthographic(float width, float height, float nearZ, float farZ)
        {
            projectionMatrix = XMMatrixOrthographicLH(width, height, nearZ, farZ);
        }

        XMMATRIX GetViewMatrix() const { return viewMatrix; }
        XMMATRIX GetProjectionMatrix() const { return projectionMatrix; }
        XMFLOAT3 GetPosition() const { return position; }
        XMFLOAT3 GetTarget() const { return target; }

        CameraData GetCameraData() const
        {
            CameraData data;
            data.view = viewMatrix;
            data.projection = projectionMatrix;
            data.position = position;
            data._padding = 0.0f;
            return data;
        }

        void MoveForward(float distance)
        {
            XMVECTOR pos = XMLoadFloat3(&position);
            XMVECTOR tgt = XMLoadFloat3(&target);
            XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(tgt, pos));
            pos = XMVectorAdd(pos, XMVectorScale(forward, distance));
            XMStoreFloat3(&position, pos);
            UpdateMatrices();
        }

        void MoveRight(float distance)
        {
            XMVECTOR pos = XMLoadFloat3(&position);
            XMVECTOR tgt = XMLoadFloat3(&target);
            XMVECTOR upVec = XMLoadFloat3(&up);
            XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(tgt, pos));
            XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, upVec));
            pos = XMVectorAdd(pos, XMVectorScale(right, distance));
            XMStoreFloat3(&position, pos);
            UpdateMatrices();
        }

        void MoveUp(float distance)
        {
            XMVECTOR pos = XMLoadFloat3(&position);
            XMVECTOR upVec = XMLoadFloat3(&up);
            pos = XMVectorAdd(pos, XMVectorScale(upVec, distance));
            XMStoreFloat3(&position, pos);
            UpdateMatrices();
        }

        void Rotate(float pitch, float yaw)
        {
            XMVECTOR pos = XMLoadFloat3(&position);
            XMVECTOR tgt = XMLoadFloat3(&target);
            XMVECTOR upVec = XMLoadFloat3(&up);

            XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(tgt, pos));
            XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, upVec));

            XMMATRIX yawRot = XMMatrixRotationAxis(upVec, yaw);
            forward = XMVector3TransformNormal(forward, yawRot);

            XMMATRIX pitchRot = XMMatrixRotationAxis(right, pitch);
            forward = XMVector3TransformNormal(forward, pitchRot);

            tgt = XMVectorAdd(pos, forward);
            XMStoreFloat3(&target, tgt);
            UpdateMatrices();
        }

        void Orbit(float deltaX, float deltaY, float radius)
        {
            XMVECTOR tgt = XMLoadFloat3(&target);
            XMVECTOR pos = XMLoadFloat3(&position);
            XMVECTOR upVec = XMLoadFloat3(&up);

            XMVECTOR toCamera = XMVectorSubtract(pos, tgt);
            float currentRadius = XMVectorGetX(XMVector3Length(toCamera));

            if (radius > 0.0f)
                currentRadius = radius;

            float currentYaw = atan2f(XMVectorGetZ(toCamera), XMVectorGetX(toCamera));
            float currentPitch = asinf(XMVectorGetY(toCamera) / currentRadius);

            currentYaw += deltaX;
            currentPitch += deltaY;

            currentPitch = max(-XM_PIDIV2 + 0.1f, min(XM_PIDIV2 - 0.1f, currentPitch));

            float x = currentRadius * cosf(currentPitch) * cosf(currentYaw);
            float y = currentRadius * sinf(currentPitch);
            float z = currentRadius * cosf(currentPitch) * sinf(currentYaw);

            XMVECTOR newPos = XMVectorAdd(tgt, XMVectorSet(x, y, z, 0.0f));
            XMStoreFloat3(&position, newPos);
            UpdateMatrices();
        }

    private:
        void UpdateMatrices()
        {
            XMVECTOR pos = XMLoadFloat3(&position);
            XMVECTOR tgt = XMLoadFloat3(&target);
            XMVECTOR upVec = XMLoadFloat3(&up);

            viewMatrix = XMMatrixLookAtLH(pos, tgt, upVec);
            projectionMatrix = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
        }

        XMFLOAT3 position;
        XMFLOAT3 target;
        XMFLOAT3 up;
        float fov;
        float aspectRatio;
        float nearPlane;
        float farPlane;

        XMMATRIX viewMatrix;
        XMMATRIX projectionMatrix;
    };

} // namespace MantraxGFX