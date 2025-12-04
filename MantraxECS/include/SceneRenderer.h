#pragma once
#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "../../MantraxEditor/includes/FPSCamera.h"
#include "ModelManager.h"
#include <vector>
#include <memory>

class SceneRenderer
{
public:
    SceneRenderer(Mantrax::GFX *gfx);
    ~SceneRenderer();

    // Agregar objetos a la escena
    void AddObject(RenderableObject *obj);
    void RemoveObject(RenderableObject *obj);
    void ClearScene();

    // Actualizar transformaciones de un objeto
    void UpdateObjectTransform(
        RenderableObject *obj,
        const glm::vec3 &position,
        const glm::vec3 &rotation,
        const glm::vec3 &scale);

    // Actualizar UBOs con la cámara actual
    void UpdateUBOs(Mantrax::FPSCamera *camera);

    // Renderizar la escena
    void RenderScene(
        std::shared_ptr<Mantrax::OffscreenFramebuffer> framebuffer);

    // Obtener objetos
    std::vector<Mantrax::RenderObject> GetRenderObjects();

private:
    Mantrax::GFX *m_gfx;
    std::vector<RenderableObject *> m_sceneObjects;

    void CopyMat4(float *dest, const glm::mat4 &src);
    glm::mat4 CreateRotationMatrix(const glm::vec3 &rotation);
};