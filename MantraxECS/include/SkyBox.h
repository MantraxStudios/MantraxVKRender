#pragma once

#include "../../MantraxRender/include/MantraxGFX_API.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

#include "EngineLoaderDLL.h"
#include "TextureLoader.h"

namespace Mantrax
{
    class MANTRAX_API SkyBox
    {
    public:
        SkyBox(GFX *gfx, float radius = 5.0f, int segments = 64, int rings = 32);
        ~SkyBox();

        bool LoadTexture(const std::string &texturePath, bool flipVertically = false);
        void CreateMaterial(std::shared_ptr<Shader> shader);

        RenderObject GetRenderObject();

        std::shared_ptr<Material> GetMaterial() const;
        std::shared_ptr<Mesh> GetMesh() const;

        void UpdateTransform(const glm::vec3 &position, UniformBufferObject &ubo);

        float GetRadius() const;
        int GetSegments() const;
        int GetRings() const;

    private:
        GFX *m_gfx;
        float m_radius;
        int m_segments;
        int m_rings;

        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;

        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Material> m_material;
        std::shared_ptr<Texture> m_texture;

        void GenerateSphereMesh();
        void CopyMat4(float *dest, const glm::mat4 &src);
    };
}
