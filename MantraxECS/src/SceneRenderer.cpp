#include "../include/SceneRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

SceneRenderer::SceneRenderer(Mantrax::GFX *gfx)
    : m_gfx(gfx)
{
}

SceneRenderer::~SceneRenderer()
{
    ClearScene();
}

void SceneRenderer::AddObject(RenderableObject *obj)
{
    if (obj)
    {
        m_sceneObjects.push_back(obj);
        m_gfx->AddRenderObject(obj->renderObj);
        std::cout << "Added object to scene: " << obj->name << std::endl;
    }
}

void SceneRenderer::RemoveObject(RenderableObject *obj)
{
    auto it = std::find(m_sceneObjects.begin(), m_sceneObjects.end(), obj);
    if (it != m_sceneObjects.end())
    {
        std::cout << "Removed object from scene: " << obj->name << std::endl;
        m_sceneObjects.erase(it);
    }
}

void SceneRenderer::ClearScene()
{
    std::cout << "Clearing scene..." << std::endl;
    m_sceneObjects.clear();
}

void SceneRenderer::UpdateObjectTransform(
    RenderableObject *obj,
    const glm::vec3 &position,
    const glm::vec3 &rotation,
    const glm::vec3 &scale)
{
    obj->position = position;
    obj->rotation = rotation;
    obj->scale = scale;
}

void SceneRenderer::UpdateUBOs(Mantrax::FPSCamera *camera)
{
    glm::vec3 camPos = camera->GetPosition();

    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 proj = camera->GetProjectionMatrix();

    for (auto *obj : m_sceneObjects)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), obj->position);
        model = model * CreateRotationMatrix(obj->rotation);
        model = glm::scale(model, obj->scale);

        CopyMat4(obj->ubo.model, model);
        CopyMat4(obj->ubo.view, view);
        CopyMat4(obj->ubo.projection, proj);

        obj->ubo.cameraPosition[0] = camPos.x;
        obj->ubo.cameraPosition[1] = camPos.y;
        obj->ubo.cameraPosition[2] = camPos.z;
        obj->ubo.cameraPosition[3] = 0.0f;

        m_gfx->UpdateMaterialUBO(obj->material.get(), obj->ubo);
    }
}

void SceneRenderer::RenderScene(
    std::shared_ptr<Mantrax::OffscreenFramebuffer> framebuffer)
{
    auto renderObjects = GetRenderObjects();
    m_gfx->RenderToOffscreenFramebuffer(framebuffer, renderObjects);
}

std::vector<Mantrax::RenderObject> SceneRenderer::GetRenderObjects()
{
    std::vector<Mantrax::RenderObject> renderObjects;
    for (auto *obj : m_sceneObjects)
    {
        renderObjects.push_back(obj->renderObj);
    }
    return renderObjects;
}

void SceneRenderer::CopyMat4(float *dest, const glm::mat4 &src)
{
    memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
}

glm::mat4 SceneRenderer::CreateRotationMatrix(const glm::vec3 &rotation)
{
    glm::mat4 rotMat = glm::mat4(1.0f);
    rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    return rotMat;
}