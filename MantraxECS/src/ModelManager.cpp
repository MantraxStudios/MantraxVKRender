#include "../include/ModelManager.h"
#include "../include/TextureLoader.h"
#include <iostream>

ModelManager::ModelManager(Mantrax::GFX *gfx)
    : m_gfx(gfx)
{
}

ModelManager::~ModelManager()
{
    Clear();
}

RenderableObject *ModelManager::CreateModelFromFile(
    const std::string &modelPath,
    const std::string &name,
    std::shared_ptr<Mantrax::Shader> shader)
{
    std::vector<Mantrax::Vertex> vertices;
    std::vector<uint32_t> indices;

    Mantrax::AssimpLoader::LoadSettings settings;
    settings.triangulate = true;
    settings.genNormals = true;
    settings.flipUVs = true;
    settings.calcTangents = true;

    std::cout << "Loading model: " << modelPath << std::endl;

    if (!m_modelLoader.LoadModel(modelPath, vertices, indices, settings, true))
    {
        std::cerr << "ERROR: " << m_modelLoader.GetLastError() << std::endl;
        return nullptr;
    }

    auto mesh = m_gfx->CreateMesh(vertices, indices);
    auto material = m_gfx->CreateMaterial(shader);

    auto obj = std::make_unique<RenderableObject>();
    obj->position = glm::vec3(0.0f);
    obj->rotation = glm::vec3(0.0f);
    obj->scale = glm::vec3(1.0f);
    obj->material = material;
    obj->renderObj = Mantrax::RenderObject(mesh, material);
    obj->name = name;

    auto ptr = obj.get();
    m_models.push_back(std::move(obj));

    std::cout << "Model '" << name << "' loaded successfully!" << std::endl;
    return ptr;
}

RenderableObject *ModelManager::CreateModelWithPBR(
    const std::string &modelPath,
    const std::string &name,
    const std::string &albedoPath,
    const std::string &normalPath,
    const std::string &metallicPath,
    const std::string &roughnessPath,
    const std::string &aoPath,
    std::shared_ptr<Mantrax::Shader> shader)
{
    auto obj = CreateModelFromFile(modelPath, name, shader);
    if (!obj)
        return nullptr;

    std::cout << "\n=== Loading PBR Textures for '" << name << "' ===" << std::endl;

    auto albedoTex = LoadTexture(albedoPath, "Albedo");
    auto normalTex = LoadTexture(normalPath, "Normal");
    auto metalTex = LoadTexture(metallicPath, "Metallic");
    auto roughnessTex = LoadTexture(roughnessPath, "Roughness");
    auto aoTex = LoadTexture(aoPath, "AO");

    obj->material->SetBaseColor(1.0f, 1.0f, 1.0f);
    obj->material->SetMetallicFactor(1.0f);
    obj->material->SetRoughnessFactor(0.2f);
    obj->material->SetNormalScale(1.0f);

    m_gfx->SetMaterialPBRTextures(
        obj->material,
        albedoTex,
        normalTex,
        metalTex,
        roughnessTex,
        aoTex);

    std::cout << "PBR textures loaded successfully!" << std::endl;
    return obj;
}

void ModelManager::DestroyModel(const std::string &name)
{
    auto it = std::find_if(m_models.begin(), m_models.end(),
                           [&name](const std::unique_ptr<RenderableObject> &obj)
                           {
                               return obj->name == name;
                           });

    if (it != m_models.end())
    {
        std::cout << "Destroying model: " << name << std::endl;
        m_models.erase(it);
    }
}

void ModelManager::DestroyModel(RenderableObject *obj)
{
    auto it = std::find_if(m_models.begin(), m_models.end(),
                           [obj](const std::unique_ptr<RenderableObject> &o)
                           {
                               return o.get() == obj;
                           });

    if (it != m_models.end())
    {
        std::cout << "Destroying model: " << (*it)->name << std::endl;
        m_models.erase(it);
    }
}

RenderableObject *ModelManager::GetModel(const std::string &name)
{
    auto it = std::find_if(m_models.begin(), m_models.end(),
                           [&name](const std::unique_ptr<RenderableObject> &obj)
                           {
                               return obj->name == name;
                           });

    return (it != m_models.end()) ? it->get() : nullptr;
}

std::vector<RenderableObject *> ModelManager::GetAllModels()
{
    std::vector<RenderableObject *> result;
    for (auto &obj : m_models)
    {
        result.push_back(obj.get());
    }
    return result;
}

void ModelManager::Clear()
{
    std::cout << "Clearing all models..." << std::endl;
    m_models.clear();
}

std::shared_ptr<Mantrax::Texture> ModelManager::LoadTexture(
    const std::string &path,
    const std::string &name)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
    {
        std::cerr << "Failed to load " << name << " texture: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return nullptr;
    }

    auto texture = m_gfx->CreateTexture(data, width, height);
    stbi_image_free(data);

    std::cout << "✓ Loaded " << name << ": " << path << " (" << width << "x" << height << ")" << std::endl;
    return texture;
}