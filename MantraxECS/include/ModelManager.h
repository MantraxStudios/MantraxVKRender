#pragma once
#include "../MantraxRender/include/MantraxGFX_API.h"
#include "../MantraxECS/include/AssimpLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <string>

struct RenderableObject
{
    Mantrax::RenderObject renderObj;
    std::shared_ptr<Mantrax::Material> material;
    Mantrax::UniformBufferObject ubo;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    std::string name;
};

class ModelManager
{
public:
    ModelManager(Mantrax::GFX *gfx);
    ~ModelManager();

    // Crear modelo desde archivo
    RenderableObject *CreateModelFromFile(
        const std::string &modelPath,
        const std::string &name,
        std::shared_ptr<Mantrax::Shader> shader);

    // Crear modelo con texturas PBR
    RenderableObject *CreateModelWithPBR(
        const std::string &modelPath,
        const std::string &name,
        const std::string &albedoPath,
        const std::string &normalPath,
        const std::string &metallicPath,
        const std::string &roughnessPath,
        const std::string &aoPath,
        std::shared_ptr<Mantrax::Shader> shader);

    // Eliminar modelo
    void DestroyModel(const std::string &name);
    void DestroyModel(RenderableObject *obj);

    RenderableObject *CreateModelOnly(
        const std::string &modelPath,
        const std::string &name);

    // Obtener modelo
    RenderableObject *GetModel(const std::string &name);

    // Obtener todos los modelos
    std::vector<RenderableObject *> GetAllModels();

    // Limpiar todos los modelos
    void Clear();

private:
    Mantrax::GFX *m_gfx;
    std::vector<std::unique_ptr<RenderableObject>> m_models;
    Mantrax::AssimpLoader m_modelLoader;

    std::shared_ptr<Mantrax::Texture> LoadTexture(
        const std::string &path,
        const std::string &name);
};