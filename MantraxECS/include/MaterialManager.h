#pragma once

#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "TextureLoader.h"
#include "IService.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace Mantrax
{
    struct MaterialTextures
    {
        std::shared_ptr<Texture> albedo = nullptr;
        std::shared_ptr<Texture> normal = nullptr;
        std::shared_ptr<Texture> metallic = nullptr;
        std::shared_ptr<Texture> roughness = nullptr;
        std::shared_ptr<Texture> ao = nullptr;
    };

    struct MaterialProperties
    {
        glm::vec3 baseColor = glm::vec3(1.0f);
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;
        float normalScale = 1.0f;
    };

    class MaterialManager : public IService
    {
    public:
        explicit MaterialManager(GFX *gfxAPI) : m_gfxAPI(gfxAPI)
        {
            if (!m_gfxAPI)
            {
                throw std::runtime_error("MaterialManager: GFX API cannot be null");
            }
        }

        ~MaterialManager() override
        {
            Clear();
        }

        // Implementación de IService
        std::string getName() override
        {
            return "MaterialManager";
        }

        // Prevenir copia
        MaterialManager(const MaterialManager &) = delete;
        MaterialManager &operator=(const MaterialManager &) = delete;

        // ====================================================================
        // CREACIÓN DE MATERIALES
        // ====================================================================

        // Crear material básico con shader
        std::shared_ptr<Material> CreateMaterial(const std::string &name, std::shared_ptr<Shader> shader)
        {
            if (m_materials.find(name) != m_materials.end())
            {
                std::cerr << "⚠️  Material '" << name << "' already exists. Returning existing.\n";
                return m_materials[name];
            }

            auto material = m_gfxAPI->CreateMaterial(shader);
            if (!material)
            {
                throw std::runtime_error("Failed to create material: " + name);
            }

            m_materials[name] = material;
            m_materialShaders[name] = shader;

            std::cout << "✅ Material created: " << name << "\n";
            return material;
        }

        // Crear material PBR completo con texturas ya cargadas
        std::shared_ptr<Material> CreatePBRMaterial(
            const std::string &name,
            std::shared_ptr<Shader> shader,
            const MaterialTextures &textures,
            const MaterialProperties &properties = MaterialProperties())
        {
            auto material = CreateMaterial(name, shader);

            // Aplicar propiedades
            material->SetBaseColor(properties.baseColor.r, properties.baseColor.g, properties.baseColor.b);
            material->SetMetallicFactor(properties.metallicFactor);
            material->SetRoughnessFactor(properties.roughnessFactor);
            material->SetNormalScale(properties.normalScale);

            // Asignar texturas PBR usando la función de GFX
            m_gfxAPI->SetMaterialPBRTextures(
                material,
                textures.albedo,
                textures.normal,
                textures.metallic,
                textures.roughness,
                textures.ao);

            // Guardar texturas y propiedades asociadas
            m_materialTextures[name] = textures;
            m_materialProperties[name] = properties;

            std::cout << "✅ PBR Material created: " << name << "\n";
            return material;
        }

        // Crear material PBR desde archivos de textura
        std::shared_ptr<Material> CreatePBRMaterialFromFiles(
            const std::string &name,
            std::shared_ptr<Shader> shader,
            const std::string &albedoPath = "",
            const std::string &normalPath = "",
            const std::string &metallicPath = "",
            const std::string &roughnessPath = "",
            const std::string &aoPath = "",
            const MaterialProperties &properties = MaterialProperties())
        {
            MaterialTextures textures;

            // Cargar texturas si existen
            if (!albedoPath.empty())
            {
                textures.albedo = LoadTexture(albedoPath);
                std::cout << "  📦 Loaded albedo: " << albedoPath << "\n";
            }

            if (!normalPath.empty())
            {
                textures.normal = LoadTexture(normalPath);
                std::cout << "  📦 Loaded normal: " << normalPath << "\n";
            }

            if (!metallicPath.empty())
            {
                textures.metallic = LoadTexture(metallicPath);
                std::cout << "  📦 Loaded metallic: " << metallicPath << "\n";
            }

            if (!roughnessPath.empty())
            {
                textures.roughness = LoadTexture(roughnessPath);
                std::cout << "  📦 Loaded roughness: " << roughnessPath << "\n";
            }

            if (!aoPath.empty())
            {
                textures.ao = LoadTexture(aoPath);
                std::cout << "  📦 Loaded AO: " << aoPath << "\n";
            }

            return CreatePBRMaterial(name, shader, textures, properties);
        }

        // ====================================================================
        // ACCESO A MATERIALES
        // ====================================================================

        std::shared_ptr<Material> GetMaterial(const std::string &name) const
        {
            auto it = m_materials.find(name);
            if (it != m_materials.end())
            {
                return it->second;
            }

            std::cerr << "⚠️  Material '" << name << "' not found\n";
            return nullptr;
        }

        bool HasMaterial(const std::string &name) const
        {
            return m_materials.find(name) != m_materials.end();
        }

        // ====================================================================
        // CLONACIÓN Y MODIFICACIÓN
        // ====================================================================

        std::shared_ptr<Material> CloneMaterial(const std::string &sourceName, const std::string &newName)
        {
            auto sourceMat = GetMaterial(sourceName);
            if (!sourceMat)
            {
                throw std::runtime_error("Cannot clone non-existent material: " + sourceName);
            }

            auto shaderIt = m_materialShaders.find(sourceName);
            if (shaderIt == m_materialShaders.end())
            {
                throw std::runtime_error("Shader not found for material: " + sourceName);
            }

            auto newMat = CreateMaterial(newName, shaderIt->second);

            // Copiar propiedades si existen
            auto propsIt = m_materialProperties.find(sourceName);
            if (propsIt != m_materialProperties.end())
            {
                const auto &props = propsIt->second;
                newMat->SetBaseColor(props.baseColor.r, props.baseColor.g, props.baseColor.b);
                newMat->SetMetallicFactor(props.metallicFactor);
                newMat->SetRoughnessFactor(props.roughnessFactor);
                newMat->SetNormalScale(props.normalScale);
                m_materialProperties[newName] = props;
            }

            // Copiar texturas si existen
            auto texIt = m_materialTextures.find(sourceName);
            if (texIt != m_materialTextures.end())
            {
                const auto &textures = texIt->second;
                m_gfxAPI->SetMaterialPBRTextures(
                    newMat,
                    textures.albedo,
                    textures.normal,
                    textures.metallic,
                    textures.roughness,
                    textures.ao);
                m_materialTextures[newName] = textures;
            }

            std::cout << "✅ Material cloned: " << sourceName << " -> " << newName << "\n";
            return newMat;
        }

        // Actualizar propiedades de material
        void UpdateMaterialProperties(const std::string &name, const MaterialProperties &properties)
        {
            auto material = GetMaterial(name);
            if (!material)
                return;

            material->SetBaseColor(properties.baseColor.r, properties.baseColor.g, properties.baseColor.b);
            material->SetMetallicFactor(properties.metallicFactor);
            material->SetRoughnessFactor(properties.roughnessFactor);
            material->SetNormalScale(properties.normalScale);

            m_materialProperties[name] = properties;
        }

        // Actualizar solo una textura específica
        void UpdateMaterialTexture(const std::string &name, const std::string &texturePath, const std::string &textureType)
        {
            auto material = GetMaterial(name);
            if (!material)
                return;

            auto texture = LoadTexture(texturePath);
            auto &textures = m_materialTextures[name];

            if (textureType == "albedo")
            {
                textures.albedo = texture;
                material->SetAlbedoTexture(texture);
            }
            else if (textureType == "normal")
            {
                textures.normal = texture;
                material->SetNormalTexture(texture);
            }
            else if (textureType == "metallic")
            {
                textures.metallic = texture;
                material->SetMetallicTexture(texture);
            }
            else if (textureType == "roughness")
            {
                textures.roughness = texture;
                material->SetRoughnessTexture(texture);
            }
            else if (textureType == "ao")
            {
                textures.ao = texture;
                material->SetAOTexture(texture);
            }

            m_gfxAPI->UpdatePBRDescriptorSet(material);
            std::cout << "✅ Updated " << textureType << " texture for material: " << name << "\n";
        }

        // ====================================================================
        // OBTENCIÓN DE INFORMACIÓN
        // ====================================================================

        MaterialProperties GetMaterialProperties(const std::string &name) const
        {
            auto it = m_materialProperties.find(name);
            if (it != m_materialProperties.end())
            {
                return it->second;
            }
            return MaterialProperties();
        }

        MaterialTextures GetMaterialTextures(const std::string &name) const
        {
            auto it = m_materialTextures.find(name);
            if (it != m_materialTextures.end())
            {
                return it->second;
            }
            return MaterialTextures();
        }

        std::shared_ptr<Shader> GetMaterialShader(const std::string &name) const
        {
            auto it = m_materialShaders.find(name);
            if (it != m_materialShaders.end())
            {
                return it->second;
            }
            return nullptr;
        }

        // Listar todos los materiales
        std::vector<std::string> GetAllMaterialNames() const
        {
            std::vector<std::string> names;
            names.reserve(m_materials.size());
            for (const auto &pair : m_materials)
            {
                names.push_back(pair.first);
            }
            return names;
        }

        // ====================================================================
        // GESTIÓN Y LIMPIEZA
        // ====================================================================

        void RemoveMaterial(const std::string &name)
        {
            auto it = m_materials.find(name);
            if (it != m_materials.end())
            {
                m_materials.erase(it);
                m_materialShaders.erase(name);
                m_materialTextures.erase(name);
                m_materialProperties.erase(name);
                std::cout << "🗑️  Material removed: " << name << "\n";
            }
        }

        void Clear()
        {
            m_materials.clear();
            m_materialShaders.clear();
            m_materialTextures.clear();
            m_materialProperties.clear();
            m_loadedTextures.clear();
            std::cout << "🗑️  All materials cleared\n";
        }

        size_t GetMaterialCount() const
        {
            return m_materials.size();
        }

        // ====================================================================
        // PRESETS COMUNES
        // ====================================================================

        // Crear material metálico
        std::shared_ptr<Material> CreateMetallicMaterial(
            const std::string &name,
            std::shared_ptr<Shader> shader,
            const std::string &albedoPath,
            const glm::vec3 &baseColor = glm::vec3(1.0f))
        {
            MaterialProperties props;
            props.baseColor = baseColor;
            props.metallicFactor = 0.9f;
            props.roughnessFactor = 0.2f;

            return CreatePBRMaterialFromFiles(name, shader, albedoPath, "", "", "", "", props);
        }

        // Crear material no metálico (dieléctrico)
        std::shared_ptr<Material> CreateDielectricMaterial(
            const std::string &name,
            std::shared_ptr<Shader> shader,
            const std::string &albedoPath,
            float roughness = 0.5f,
            const glm::vec3 &baseColor = glm::vec3(1.0f))
        {
            MaterialProperties props;
            props.baseColor = baseColor;
            props.metallicFactor = 0.0f;
            props.roughnessFactor = roughness;

            return CreatePBRMaterialFromFiles(name, shader, albedoPath, "", "", "", "", props);
        }

        // Crear material con color sólido (sin texturas)
        std::shared_ptr<Material> CreateSolidColorMaterial(
            const std::string &name,
            std::shared_ptr<Shader> shader,
            const glm::vec3 &color,
            float metallic = 0.0f,
            float roughness = 0.5f)
        {
            MaterialProperties props;
            props.baseColor = color;
            props.metallicFactor = metallic;
            props.roughnessFactor = roughness;

            return CreatePBRMaterial(name, shader, MaterialTextures(), props);
        }

    private:
        // ====================================================================
        // HELPERS PRIVADOS
        // ====================================================================

        std::shared_ptr<Texture> LoadTexture(const std::string &path)
        {
            // Verificar si ya está cargada
            auto it = m_loadedTextures.find(path);
            if (it != m_loadedTextures.end())
            {
                std::cout << "  ♻️  Reusing cached texture: " << path << "\n";
                return it->second;
            }

            // Cargar nueva textura
            auto textureData = TextureLoader::LoadFromFile(path);
            if (!textureData)
            {
                throw std::runtime_error("Failed to load texture: " + path);
            }

            auto texture = m_gfxAPI->CreateTexture(
                textureData->pixels,
                textureData->width,
                textureData->height);

            // Cachear para reutilización
            m_loadedTextures[path] = texture;
            return texture;
        }

        // ====================================================================
        // MIEMBROS PRIVADOS
        // ====================================================================

        GFX *m_gfxAPI;

        // Materiales registrados
        std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_materialShaders;
        std::unordered_map<std::string, MaterialTextures> m_materialTextures;
        std::unordered_map<std::string, MaterialProperties> m_materialProperties;

        // Cache de texturas para evitar cargar duplicados
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_loadedTextures;
    };
}