#include "../include/MantraxGFX_API.h"

namespace Mantrax
{

    //=====================================
    //
    //
    // PUBLIC SECTION
    //
    //
    //=====================================

    GFX::GFX(HINSTANCE hInstance, HWND hWnd, const Config &config)
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

    GFX::~GFX()
    {
        Cleanup();
    }

    std::shared_ptr<Texture> GFX::CreateTexture(unsigned char *data, int width, int height, VkFilter TextureFilter)
    {
        auto texture = std::make_shared<Texture>();
        texture->width = width;
        texture->height = height;

        VkDeviceSize imageSize = width * height * 4; // RGBA

        // Crear staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        CreateBuffer(imageSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingMemory);

        void *mappedData;
        vkMapMemory(m_Device, stagingMemory, 0, imageSize, 0, &mappedData);
        memcpy(mappedData, data, imageSize);
        vkUnmapMemory(m_Device, stagingMemory);

        CreateImage(width, height, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    texture->image, texture->memory);

        TransitionImageLayout(texture->image, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        CopyBufferToImage(stagingBuffer, texture->image, width, height);

        TransitionImageLayout(texture->image, VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
        vkFreeMemory(m_Device, stagingMemory, nullptr);

        texture->imageView = CreateImageView(texture->image, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_ASPECT_COLOR_BIT);

        // ✅ SAMPLER MEJORADO - Configuración óptima para texturas pequeñas
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        // ✅ Para texturas pequeñas/pixel art: NEAREST
        // Para texturas normales: LINEAR
        // Puedes detectar automáticamente:
        if (width <= 16 || height <= 16)
        {
            // Texturas muy pequeñas o pixel art
            samplerInfo.magFilter = TextureFilter;
            samplerInfo.minFilter = TextureFilter;
            samplerInfo.mipmapMode = TextureFilter == VK_FILTER_NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
        else
        {
            // Texturas normales
            samplerInfo.magFilter = TextureFilter;
            samplerInfo.minFilter = TextureFilter;
            samplerInfo.mipmapMode = TextureFilter == VK_FILTER_NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }

        // ✅ CLAMP_TO_EDGE evita que se repita la textura
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        // ✅ Anisotropía desactivada para texturas pequeñas (mejora rendimiento)
        if (width <= 16 || height <= 16)
        {
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
        }
        else
        {
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
        }

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE; // ✅ Coordenadas normalizadas (0-1)
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        // ✅ LOD correcto para evitar problemas con texturas pequeñas
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f; // Sin mipmaps por ahora

        if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS)
            throw std::runtime_error("Error creando sampler de textura");

        std::cout << "✅ Textura creada: " << width << "x" << height
                  << " (Filtro: " << (width <= 16 ? "NEAREST" : "LINEAR") << ")" << std::endl;

        return texture;
    }

    void GFX::SetMaterialTexture(std::shared_ptr<Material> material, std::shared_ptr<Texture> texture)
    {
        // Ahora asigna a albedo para compatibilidad
        material->SetAlbedoTexture(texture);

        // UpdatePBRDescriptorSet(material);
    }

    void GFX::UpdateRenderObjectUBO(RenderObject *obj, const UniformBufferObject &ubo)
    {
        if (!obj || !obj->mesh || obj->mesh->uniformBuffer == VK_NULL_HANDLE)
        {
            throw std::runtime_error("RenderObject o mesh no válido para actualizar UBO");
        }

        UpdateMeshUBO(obj->mesh.get(), ubo);
    }

    void GFX::AddRenderObjectSafe(const RenderObject &obj)
    {
        vkDeviceWaitIdle(m_Device);

        if (!obj.mesh || !obj.material)
        {
            throw std::runtime_error("RenderObject debe tener mesh y material válidos");
        }

        // Si el mesh no tiene descriptor set, crearlo
        if (obj.mesh->descriptorSet == VK_NULL_HANDLE)
        {
            CreateMeshDescriptorSet(obj.mesh, obj.material);
        }

        m_RenderObjects.push_back(obj);
        m_NeedCommandBufferRebuild = true;
    }

    std::shared_ptr<OffscreenFramebuffer> GFX::CreateOffscreenFramebuffer(uint32_t width, uint32_t height)
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

    void GFX::ResizeOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen,
                                         uint32_t width, uint32_t height)
    {
        // ✅ Validación de tamaño mínimo
        if (width == 0 || height == 0)
        {
            std::cerr << "❌ Invalid framebuffer size: " << width << "x" << height << std::endl;
            return;
        }

        // ✅ Si el tamaño no cambió, no hacer nada
        if (offscreen->extent.width == width && offscreen->extent.height == height)
        {
            std::cout << "⏭️ Framebuffer size unchanged, skipping resize" << std::endl;
            return;
        }

        vkDeviceWaitIdle(m_Device);

        // Limpiar recursos anteriores (orden importante)
        if (offscreen->framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(m_Device, offscreen->framebuffer, nullptr);
            offscreen->framebuffer = VK_NULL_HANDLE;
        }

        if (offscreen->colorImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, offscreen->colorImageView, nullptr);
            offscreen->colorImageView = VK_NULL_HANDLE;
        }

        if (offscreen->depthImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, offscreen->depthImageView, nullptr);
            offscreen->depthImageView = VK_NULL_HANDLE;
        }

        if (offscreen->colorImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_Device, offscreen->colorImage, nullptr);
            offscreen->colorImage = VK_NULL_HANDLE;
        }

        if (offscreen->depthImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_Device, offscreen->depthImage, nullptr);
            offscreen->depthImage = VK_NULL_HANDLE;
        }

        if (offscreen->colorMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, offscreen->colorMemory, nullptr);
            offscreen->colorMemory = VK_NULL_HANDLE;
        }

        if (offscreen->depthMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, offscreen->depthMemory, nullptr);
            offscreen->depthMemory = VK_NULL_HANDLE;
        }

        // Actualizar dimensiones
        offscreen->extent = {width, height};

        // Recrear color image
        CreateImage(width, height, offscreen->colorFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    offscreen->colorImage, offscreen->colorMemory);

        offscreen->colorImageView = CreateImageView(offscreen->colorImage,
                                                    offscreen->colorFormat,
                                                    VK_IMAGE_ASPECT_COLOR_BIT);

        // Recrear depth image
        CreateImage(width, height, offscreen->depthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    offscreen->depthImage, offscreen->depthMemory);

        offscreen->depthImageView = CreateImageView(offscreen->depthImage,
                                                    offscreen->depthFormat,
                                                    VK_IMAGE_ASPECT_DEPTH_BIT);

        // Recrear framebuffer
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

        // ✅ CRÍTICO: Transición correcta con command buffer
        VkCommandBuffer cmd = BeginSingleTimeCommands();

        // Transición de color image: UNDEFINED → SHADER_READ_ONLY
        VkImageMemoryBarrier colorBarrier{};
        colorBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        colorBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        colorBarrier.image = offscreen->colorImage;
        colorBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorBarrier.subresourceRange.baseMipLevel = 0;
        colorBarrier.subresourceRange.levelCount = 1;
        colorBarrier.subresourceRange.baseArrayLayer = 0;
        colorBarrier.subresourceRange.layerCount = 1;
        colorBarrier.srcAccessMask = 0;
        colorBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &colorBarrier);

        // ✅ Transición de depth image: UNDEFINED → DEPTH_STENCIL_ATTACHMENT
        VkImageMemoryBarrier depthBarrier{};
        depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.image = offscreen->depthImage;
        depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthBarrier.subresourceRange.baseMipLevel = 0;
        depthBarrier.subresourceRange.levelCount = 1;
        depthBarrier.subresourceRange.baseArrayLayer = 0;
        depthBarrier.subresourceRange.layerCount = 1;
        depthBarrier.srcAccessMask = 0;
        depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &depthBarrier);

        EndSingleTimeCommands(cmd);
    }

    std::shared_ptr<Texture> GFX::CreateDefaultWhiteTexture()
    {
        unsigned char whitePixel[4] = {255, 255, 255, 255};
        return CreateTexture(whitePixel, 1, 1);
    }

    // Crear textura normal por defecto (normal apuntando hacia arriba)
    std::shared_ptr<Texture> GFX::CreateDefaultNormalTexture()
    {
        unsigned char normalPixel[4] = {128, 128, 255, 255}; // Normal (0, 0, 1) en tangent space
        return CreateTexture(normalPixel, 1, 1);
    }

    // Función pública para asignar texturas PBR a un material
    void GFX::SetMaterialPBRTextures(std::shared_ptr<Material> material,
                                     std::shared_ptr<Texture> albedo,
                                     std::shared_ptr<Texture> normal,
                                     std::shared_ptr<Texture> metallic,
                                     std::shared_ptr<Texture> roughness,
                                     std::shared_ptr<Texture> ao)
    {
        if (albedo)
            material->SetAlbedoTexture(albedo);
        if (normal)
            material->SetNormalTexture(normal);
        if (metallic)
            material->SetMetallicTexture(metallic);
        if (roughness)
            material->SetRoughnessTexture(roughness);
        if (ao)
            material->SetAOTexture(ao);

        UpdatePBRDescriptorSet(material);
    }

    void GFX::DestroyOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen)
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

    std::shared_ptr<Shader> GFX::CreateShader(const ShaderConfig &config)
    {
        auto shader = std::make_shared<Shader>(config);
        CreateShaderPipeline(shader);
        return shader;
    }

    std::shared_ptr<Mesh> GFX::CreateMesh(const std::vector<Vertex> &vertices,
                                          const std::vector<uint32_t> &indices)
    {
        auto mesh = std::make_shared<Mesh>(vertices, indices);
        CreateVertexBuffer(mesh);
        CreateIndexBuffer(mesh);
        CreateUniformBuffer(mesh);
        return mesh;
    }

    std::shared_ptr<Material> GFX::CreateMaterial(std::shared_ptr<Shader> shader)
    {
        auto material = std::make_shared<Material>(shader);
        // Ya no crea uniform buffer ni descriptor set
        return material;
    }

    std::shared_ptr<RenderPassObject> GFX::CreateRenderPass(const RenderPassConfig &config)
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

    void GFX::CreateFramebuffersForRenderPass(std::shared_ptr<RenderPassObject> renderPassObj,
                                              const std::vector<VkImageView> &additionalAttachments)
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

    std::shared_ptr<Shader> GFX::CreateShaderWithRenderPass(const ShaderConfig &config,
                                                            std::shared_ptr<RenderPassObject> renderPassObj)
    {
        auto shader = std::make_shared<Shader>(config);
        CreateShaderPipeline(shader, renderPassObj->renderPass);
        return shader;
    }

    void GFX::BeginRenderPassInCommandBuffer(VkCommandBuffer cmd,
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
    void GFX::DrawFrameWithRenderPass(std::shared_ptr<RenderPassObject> renderPassObj,
                                      const std::vector<RenderObject> &objects,
                                      const std::vector<VkClearValue> &clearValues,
                                      std::function<void(VkCommandBuffer)> additionalCommands)
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

            // ✅ CAMBIAR ESTO:
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.material->shader->pipelineLayout,
                                    0, 1, &obj.mesh->descriptorSet, 0, nullptr);

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
    void GFX::CleanupCustomRenderPasses()
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

    void GFX::AddRenderObject(const RenderObject &obj)
    {
        if (!obj.mesh || !obj.material)
        {
            throw std::runtime_error("RenderObject debe tener mesh y material válidos (no safe)");
        }

        // Si el mesh no tiene descriptor set, crearlo
        if (obj.mesh->descriptorSet == VK_NULL_HANDLE)
        {
            CreateMeshDescriptorSet(obj.mesh, obj.material);
        }

        m_RenderObjects.push_back(obj);
        m_NeedCommandBufferRebuild = true;
    }

    void GFX::UpdateMeshUBO(Mesh *mesh, const UniformBufferObject &ubo)
    {
        if (!mesh || mesh->uniformBuffer == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Mesh no tiene uniform buffer válido");
        }

        mesh->ubo = ubo;
        void *data;
        vkMapMemory(m_Device, mesh->uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);
        memcpy(data, &ubo, sizeof(UniformBufferObject));
        vkUnmapMemory(m_Device, mesh->uniformBufferMemory);
    }

    void GFX::ClearRenderObjects()
    {
        m_RenderObjects.clear();
        m_NeedCommandBufferRebuild = true;
    }

    void GFX::NotifyFramebufferResized()
    {
        m_FramebufferResized = true;
    }

    bool GFX::DrawFrame(std::function<void(VkCommandBuffer)> imguiRenderCallback)
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

    void GFX::WaitIdle()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_Device);
        }
    }

    void GFX::ClearRenderObjectsSafe()
    {
        vkDeviceWaitIdle(m_Device);
        m_RenderObjects.clear();
        m_NeedCommandBufferRebuild = true;
    }

    std::vector<RenderObject> GFX::GetRenderObjects() const
    {
        return m_RenderObjects;
    }

    //=====================================
    //
    //
    // PRIVATE SECTION
    //
    //
    //=====================================

    void GFX::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer cmd = BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(cmd, buffer, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);

        EndSingleTimeCommands(cmd);
    }

    // void GFX::UpdateDescriptorSetWithTexture(std::shared_ptr<Material> material)
    // {
    //     UpdatePBRDescriptorSet(material);
    // }

    std::vector<char> GFX::ReadFile(const std::string &filename)
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

    void GFX::CreateImage(uint32_t width, uint32_t height, VkFormat format,
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

    VkImageView GFX::CreateImageView(VkImage image, VkFormat format,
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

    VkRenderPass GFX::CreateOffscreenRenderPass(VkFormat colorFormat, VkFormat depthFormat)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

    VkCommandBuffer GFX::BeginSingleTimeCommands()
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

    void GFX::EndSingleTimeCommands(VkCommandBuffer cmd)
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

    void GFX::TransitionImageLayout(VkImage image, VkFormat format,
                                    VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer cmd = BeginSingleTimeCommands();

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
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
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

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

        EndSingleTimeCommands(cmd);
    }

    void GFX::TransitionImageLayoutCmd(VkCommandBuffer cmd, VkImage image, VkFormat format,
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

    VkShaderModule GFX::CreateShaderModule(const std::vector<char> &code)
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

    uint32_t GFX::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props)
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

    void GFX::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
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

    void GFX::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
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

    void GFX::InitVulkan()
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

    void GFX::CreateInstance()
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

    void GFX::CreateSurface()
    {
        VkWin32SurfaceCreateInfoKHR s{};
        s.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        s.hinstance = m_hInstance;
        s.hwnd = m_hWnd;

        if (vkCreateWin32SurfaceKHR(m_Instance, &s, nullptr, &m_Surface) != VK_SUCCESS)
            throw std::runtime_error("Error creando surface");
    }

    void GFX::PickPhysicalDevice()
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

    void GFX::CreateLogicalDevice()
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

    void GFX::CreateSwapchain(bool enableVSync)
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

    void GFX::CreateImageViews()
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

    VkFormat GFX::FindDepthFormat()
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

    void GFX::CreateDepthResources()
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

    void GFX::CreateRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

    void GFX::CreateDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};

        // UBOs
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 1000;

        // Image Samplers (ahora necesitamos 5 por material: albedo, normal, metallic, roughness, AO)
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 5000; // 1000 materiales * 5 texturas

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 2000;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Para poder actualizar

        if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Error creando descriptor pool");
    }

    void GFX::CreateFramebuffers()
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

    void GFX::CreateCommandPool()
    {
        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = m_GraphicsQueueFamily;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(m_Device, &ci, nullptr, &m_CommandPool) != VK_SUCCESS)
            throw std::runtime_error("Error creando command pool");
    }

    void GFX::CreateCommandBuffers()
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

    void GFX::CreateSyncObjects()
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

    void GFX::CreateShaderPipeline(std::shared_ptr<Shader> shader, VkRenderPass renderPass)
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

        // ✅ VIEWPORT Y SCISSOR DINÁMICOS
        VkViewport vp{};
        vp.x = 0.0f;
        vp.y = 0.0f;
        vp.width = 1.0f;  // Dummy - se establecerá dinámicamente
        vp.height = 1.0f; // Dummy - se establecerá dinámicamente
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;

        VkRect2D sc{{0, 0}, {1, 1}}; // Dummy - se establecerá dinámicamente

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

        if (shader->config.blendEnable)
        {
            cba.blendEnable = VK_TRUE;
            // Alpha blending correcto
            cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            cba.colorBlendOp = VK_BLEND_OP_ADD;

            cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            cba.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        else
        {
            cba.blendEnable = VK_FALSE;
        }

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;

        // Descriptor Set Layout con todas las texturas PBR
        std::array<VkDescriptorSetLayoutBinding, 6> bindings{};

        // Binding 0: UBO (matrices)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // Binding 1: Albedo Map
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 2: Normal Map
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 3: Metallic Map
        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 4: Roughness Map
        bindings[4].binding = 4;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 5: AO Map
        bindings[5].binding = 5;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[5].descriptorCount = 1;
        bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &shader->descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Error creando descriptor set layout");

        // Pipeline Layout con Push Constants
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(MaterialPushConstants);

        VkPipelineLayoutCreateInfo pl{};
        pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pl.setLayoutCount = 1;
        pl.pSetLayouts = &shader->descriptorSetLayout;
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(m_Device, &pl, nullptr, &shader->pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Error creando pipeline layout");

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        // ✅ CRÍTICO: Para objetos transparentes
        if (shader->config.blendEnable)
        {
            ds.depthTestEnable = VK_TRUE;   // Leer depth buffer
            ds.depthWriteEnable = VK_FALSE; // NO escribir en depth buffer
            ds.depthCompareOp = VK_COMPARE_OP_LESS;
        }
        else
        {
            // Para objetos opacos
            ds.depthTestEnable = shader->config.depthTestEnable ? VK_TRUE : VK_FALSE;
            ds.depthWriteEnable = shader->config.depthWriteEnable ? VK_TRUE : VK_FALSE;
            ds.depthCompareOp = shader->config.depthCompareOp;
        }

        ds.depthBoundsTestEnable = VK_FALSE;
        ds.stencilTestEnable = VK_FALSE;

        // ✅ DYNAMIC STATE PARA VIEWPORT Y SCISSOR
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Crear pipeline
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
        pi.pDynamicState = &dynamicState; // ✅ AÑADIR DYNAMIC STATE
        pi.layout = shader->pipelineLayout;
        pi.renderPass = (renderPass != VK_NULL_HANDLE) ? renderPass : m_RenderPass;
        pi.subpass = 0;

        if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pi, nullptr, &shader->pipeline) != VK_SUCCESS)
            throw std::runtime_error("Error creando pipeline");

        vkDestroyShaderModule(m_Device, vert, nullptr);
        vkDestroyShaderModule(m_Device, frag, nullptr);

        // Agregar a la lista de shaders
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

    void GFX::CreateVertexBuffer(std::shared_ptr<Mesh> mesh)
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

    void GFX::CreateIndexBuffer(std::shared_ptr<Mesh> mesh)
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

    void GFX::CreateUniformBuffer(std::shared_ptr<Mesh> mesh)
    {
        VkDeviceSize size = sizeof(UniformBufferObject);

        CreateBuffer(size,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     mesh->uniformBuffer, mesh->uniformBufferMemory);
    }

    void GFX::CreateMeshDescriptorSet(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
    {
        if (!material || !material->shader)
        {
            throw std::runtime_error("Material y shader deben ser válidos");
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &material->shader->descriptorSetLayout;

        if (vkAllocateDescriptorSets(m_Device, &allocInfo, &mesh->descriptorSet) != VK_SUCCESS)
            throw std::runtime_error("Error creando descriptor set para mesh");

        // Actualizar descriptor set con UBO del mesh
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mesh->uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // Texturas por defecto
        static std::shared_ptr<Texture> defaultWhite = nullptr;
        static std::shared_ptr<Texture> defaultNormal = nullptr;

        if (!defaultWhite)
            defaultWhite = CreateDefaultWhiteTexture();
        if (!defaultNormal)
            defaultNormal = CreateDefaultNormalTexture();

        // Usar texturas del material o defaults
        auto albedoTex = material->pbrTextures.albedo ? material->pbrTextures.albedo : defaultWhite;
        auto normalTex = material->pbrTextures.normal ? material->pbrTextures.normal : defaultNormal;
        auto metallicTex = material->pbrTextures.metallic ? material->pbrTextures.metallic : defaultWhite;
        auto roughnessTex = material->pbrTextures.roughness ? material->pbrTextures.roughness : defaultWhite;
        auto aoTex = material->pbrTextures.ao ? material->pbrTextures.ao : defaultWhite;

        // Image infos
        std::array<VkDescriptorImageInfo, 5> imageInfos{};

        imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[0].imageView = albedoTex->imageView;
        imageInfos[0].sampler = albedoTex->sampler;

        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[1].imageView = normalTex->imageView;
        imageInfos[1].sampler = normalTex->sampler;

        imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[2].imageView = metallicTex->imageView;
        imageInfos[2].sampler = metallicTex->sampler;

        imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[3].imageView = roughnessTex->imageView;
        imageInfos[3].sampler = roughnessTex->sampler;

        imageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[4].imageView = aoTex->imageView;
        imageInfos[4].sampler = aoTex->sampler;

        // Writes
        std::array<VkWriteDescriptorSet, 6> writes{};

        // Write 0: UBO del mesh
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = mesh->descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &bufferInfo;

        // Writes 1-5: Texturas del material
        for (int i = 0; i < 5; i++)
        {
            writes[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i + 1].dstSet = mesh->descriptorSet;
            writes[i + 1].dstBinding = i + 1;
            writes[i + 1].dstArrayElement = 0;
            writes[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i + 1].descriptorCount = 1;
            writes[i + 1].pImageInfo = &imageInfos[i];
        }

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(writes.size()),
                               writes.data(), 0, nullptr);
    }

    void GFX::CleanupSwapchain()
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

    void GFX::RecreateSwapchainWithCustomRenderPasses()
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

    void GFX::RecreateSwapchain()
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

        // Limpiar render passes personalizados
        CleanupCustomRenderPasses();

        // Invalidar descriptor sets de meshes (se recrearán automáticamente)
        for (auto &obj : m_RenderObjects)
        {
            if (obj.mesh && obj.mesh->descriptorSet != VK_NULL_HANDLE)
            {
                // El descriptor set se liberará con el pool, solo invalidamos la referencia
                obj.mesh->descriptorSet = VK_NULL_HANDLE;
            }
        }

        CleanupSwapchain();

        CreateSwapchain(false);
        CreateImageViews();
        CreateDepthResources();
        CreateRenderPass();
        CreateFramebuffers();
        CreateCommandBuffers();

        // Recrear shaders
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

        // Recrear descriptor sets para meshes
        for (auto &obj : m_RenderObjects)
        {
            if (obj.mesh && obj.material && obj.material->shader)
            {
                CreateMeshDescriptorSet(obj.mesh, obj.material);
            }
        }

        m_NeedCommandBufferRebuild = true;

        std::cout << "✅ Swapchain recreada: " << width << "x" << height << "\n";
    }

    // ============================================
    // FUNCIÓN COMPLETA: RecordCommandBuffer
    // ============================================

    void GFX::RecordCommandBuffer(VkCommandBuffer cmd, uint32_t index,
                                  std::function<void(VkCommandBuffer)> imguiRenderCallback)
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

        // ✅ ESTABLECER VIEWPORT Y SCISSOR DINÁMICOS
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_SwapchainExtent.width);
        viewport.height = static_cast<float>(m_SwapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_SwapchainExtent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        VkPipeline lastPipeline = VK_NULL_HANDLE;

        // ✅ PASO 1: Renderizar objetos OPACOS primero
        for (const auto &obj : m_RenderObjects)
        {
            if (!obj.mesh || !obj.material || !obj.material->shader)
                continue;

            if (!obj.mesh->vertexBuffer || !obj.mesh->indexBuffer)
                continue;

            if (!obj.material->shader->pipeline)
                continue;

            // Saltar objetos transparentes en este pase
            if (obj.material->shader->config.blendEnable)
                continue;

            if (obj.material->shader->pipeline != lastPipeline)
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  obj.material->shader->pipeline);
                lastPipeline = obj.material->shader->pipeline;
            }

            // Push constants del material
            vkCmdPushConstants(
                cmd,
                obj.material->shader->pipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(MaterialPushConstants),
                &obj.material->pushConstants);

            VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // ✅ CAMBIO CRÍTICO: Usar descriptor set del MESH
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.material->shader->pipelineLayout,
                                    0, 1, &obj.mesh->descriptorSet, 0, nullptr);

            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()),
                             1, 0, 0, 0);
        }

        // ✅ PASO 2: Renderizar objetos TRANSPARENTES
        for (const auto &obj : m_RenderObjects)
        {
            if (!obj.mesh || !obj.material || !obj.material->shader)
                continue;

            if (!obj.mesh->vertexBuffer || !obj.mesh->indexBuffer)
                continue;

            if (!obj.material->shader->pipeline)
                continue;

            // Solo objetos transparentes
            if (!obj.material->shader->config.blendEnable)
                continue;

            if (obj.material->shader->pipeline != lastPipeline)
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  obj.material->shader->pipeline);
                lastPipeline = obj.material->shader->pipeline;
            }

            // Push constants del material
            vkCmdPushConstants(
                cmd,
                obj.material->shader->pipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(MaterialPushConstants),
                &obj.material->pushConstants);

            VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // ✅ CAMBIO CRÍTICO: Usar descriptor set del MESH
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.material->shader->pipelineLayout,
                                    0, 1, &obj.mesh->descriptorSet, 0, nullptr);

            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()),
                             1, 0, 0, 0);
        }

        // Renderizar ImGui si hay callback
        if (imguiRenderCallback)
        {
            imguiRenderCallback(cmd);
        }

        vkCmdEndRenderPass(cmd);

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
            throw std::runtime_error("Error finalizando command buffer");
    }

    // ============================================
    // FUNCIÓN COMPLETA: RenderToOffscreenFramebuffer
    // ============================================

    void GFX::RenderToOffscreenFramebuffer(std::shared_ptr<OffscreenFramebuffer> offscreen,
                                           const std::vector<RenderObject> &objects)
    {
        VkCommandBuffer cmd = BeginSingleTimeCommands();

        // Transición: SHADER_READ_ONLY → COLOR_ATTACHMENT
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
        clearValues[0].color = m_Config.clearColor;
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

        // ✅ ESTABLECER VIEWPORT Y SCISSOR
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

        VkPipeline lastPipeline = VK_NULL_HANDLE;

        // Renderizar objetos OPACOS primero
        for (const auto &obj : objects)
        {
            if (!obj.mesh || !obj.material || !obj.material->shader)
                continue;

            if (!obj.mesh->vertexBuffer || !obj.mesh->indexBuffer)
                continue;

            if (!obj.material->shader->pipeline)
                continue;

            // Saltar transparentes
            if (obj.material->shader->config.blendEnable)
                continue;

            if (obj.material->shader->pipeline != lastPipeline)
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  obj.material->shader->pipeline);
                lastPipeline = obj.material->shader->pipeline;
            }

            vkCmdPushConstants(
                cmd,
                obj.material->shader->pipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(MaterialPushConstants),
                &obj.material->pushConstants);

            VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // ✅ CAMBIO: Usar descriptor set del MESH
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.material->shader->pipelineLayout,
                                    0, 1, &obj.mesh->descriptorSet, 0, nullptr);

            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()),
                             1, 0, 0, 0);
        }

        // Renderizar objetos TRANSPARENTES
        for (const auto &obj : objects)
        {
            if (!obj.mesh || !obj.material || !obj.material->shader)
                continue;

            if (!obj.mesh->vertexBuffer || !obj.mesh->indexBuffer)
                continue;

            if (!obj.material->shader->pipeline)
                continue;

            // Solo transparentes
            if (!obj.material->shader->config.blendEnable)
                continue;

            if (obj.material->shader->pipeline != lastPipeline)
            {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  obj.material->shader->pipeline);
                lastPipeline = obj.material->shader->pipeline;
            }

            vkCmdPushConstants(
                cmd,
                obj.material->shader->pipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(MaterialPushConstants),
                &obj.material->pushConstants);

            VkBuffer vertexBuffers[] = {obj.mesh->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmd, obj.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            // ✅ CAMBIO: Usar descriptor set del MESH
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.material->shader->pipelineLayout,
                                    0, 1, &obj.mesh->descriptorSet, 0, nullptr);

            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(obj.mesh->indices.size()),
                             1, 0, 0, 0);
        }

        vkCmdEndRenderPass(cmd);

        // Transición: COLOR_ATTACHMENT → SHADER_READ_ONLY
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

    // ============================================
    // FUNCIÓN COMPLETA: UpdatePBRDescriptorSet
    // ============================================

    void GFX::UpdatePBRDescriptorSet(std::shared_ptr<Material> material)
    {
        // NOTA: Esta función ahora debe recibir también el mesh
        // Se mantiene para compatibilidad pero el flujo correcto es:
        // 1. Crear mesh
        // 2. Crear material
        // 3. Llamar a CreateMeshDescriptorSet(mesh, material)

        std::cerr << "⚠️ WARNING: UpdatePBRDescriptorSet() está deprecada.\n"
                  << "Usa CreateMeshDescriptorSet(mesh, material) en su lugar.\n";
    }

    // ============================================
    // FUNCIÓN NUEVA: UpdateMeshMaterialTextures
    // ============================================

    void GFX::UpdateMeshMaterialTextures(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
    {
        if (!mesh || !material)
        {
            throw std::runtime_error("Mesh y material deben ser válidos");
        }

        // Si el mesh no tiene descriptor set, crearlo
        if (mesh->descriptorSet == VK_NULL_HANDLE)
        {
            CreateMeshDescriptorSet(mesh, material);
        }
        else
        {
            // Recrear descriptor set con las nuevas texturas
            // Primero liberarlo
            vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &mesh->descriptorSet);
            mesh->descriptorSet = VK_NULL_HANDLE;

            // Crear nuevo
            CreateMeshDescriptorSet(mesh, material);
        }

        std::cout << "✅ Texturas del mesh actualizadas correctamente\n";
    }

    // ============================================
    // FUNCIÓN COMPLETA: Cleanup
    // ============================================

    void GFX::Cleanup()
    {
        if (m_Device == VK_NULL_HANDLE && m_Instance == VK_NULL_HANDLE)
            return;

        if (m_Device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(m_Device);

        // Limpiar render objects
        for (auto &obj : m_RenderObjects)
        {
            if (obj.mesh)
            {
                // Limpiar buffers del mesh
                if (obj.mesh->vertexBuffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(m_Device, obj.mesh->vertexBuffer, nullptr);
                    obj.mesh->vertexBuffer = VK_NULL_HANDLE;
                }
                if (obj.mesh->vertexBufferMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(m_Device, obj.mesh->vertexBufferMemory, nullptr);
                    obj.mesh->vertexBufferMemory = VK_NULL_HANDLE;
                }
                if (obj.mesh->indexBuffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(m_Device, obj.mesh->indexBuffer, nullptr);
                    obj.mesh->indexBuffer = VK_NULL_HANDLE;
                }
                if (obj.mesh->indexBufferMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(m_Device, obj.mesh->indexBufferMemory, nullptr);
                    obj.mesh->indexBufferMemory = VK_NULL_HANDLE;
                }

                // ✅ AÑADIR: Limpiar uniform buffer del mesh
                if (obj.mesh->uniformBuffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(m_Device, obj.mesh->uniformBuffer, nullptr);
                    obj.mesh->uniformBuffer = VK_NULL_HANDLE;
                }
                if (obj.mesh->uniformBufferMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(m_Device, obj.mesh->uniformBufferMemory, nullptr);
                    obj.mesh->uniformBufferMemory = VK_NULL_HANDLE;
                }

                // Descriptor set se libera automáticamente con el pool
                obj.mesh->descriptorSet = VK_NULL_HANDLE;
            }
        }

        // Limpiar shaders
        for (auto &shader : m_AllShaders)
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
        }

        // Limpiar custom render passes
        CleanupCustomRenderPasses();

        // Sincronización
        if (m_InFlightFence != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_Device, m_InFlightFence, nullptr);
            m_InFlightFence = VK_NULL_HANDLE;
        }
        if (m_RenderFinishedSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
            m_RenderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (m_ImageAvailableSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
            m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        }

        // Swapchain
        CleanupSwapchain();

        // Descriptor pool
        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
            m_DescriptorPool = VK_NULL_HANDLE;
        }

        // Command pool
        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        // Device y instance
        if (m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }
    }
}