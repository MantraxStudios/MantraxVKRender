#include "../include/SkyBox.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cstring>

namespace Mantrax
{
    SkyBox::SkyBox(GFX *gfx, float radius, int segments, int rings)
        : m_gfx(gfx), m_radius(radius), m_segments(segments), m_rings(rings)
    {
        GenerateSphereMesh();
    }

    SkyBox::~SkyBox() {}

    bool SkyBox::LoadTexture(const std::string &texturePath, bool flipVertically)
    {
        try
        {
            auto textureData = TextureLoader::LoadFromFile(texturePath, flipVertically);

            if (!textureData || !textureData->pixels)
            {
                std::cerr << "Failed to load skybox texture: " << texturePath << std::endl;
                return false;
            }

            m_texture = m_gfx->CreateTexture(
                textureData->pixels,
                textureData->width,
                textureData->height);

            std::cout << "✓ Loaded Skybox Texture: " << texturePath
                      << " (" << textureData->width << "x" << textureData->height
                      << ", " << textureData->channels << " channels)\n";

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception loading skybox texture: " << e.what() << std::endl;
            return false;
        }
    }

    void SkyBox::CreateMaterial(std::shared_ptr<Shader> shader)
    {
        m_material = m_gfx->CreateMaterial(shader);

        m_material->SetBaseColor(1.0f, 1.0f, 1.0f);
        m_material->SetMetallicFactor(0.0f);
        m_material->SetRoughnessFactor(1.0f);

        if (m_texture)
        {
            m_gfx->SetMaterialPBRTextures(
                m_material,
                m_texture,
                nullptr,
                nullptr,
                nullptr,
                nullptr);
        }
    }

    RenderObject SkyBox::GetRenderObject()
    {
        return RenderObject(m_mesh, m_material);
    }

    std::shared_ptr<Material> SkyBox::GetMaterial() const
    {
        return m_material;
    }

    std::shared_ptr<Mesh> SkyBox::GetMesh() const
    {
        return m_mesh;
    }

    void SkyBox::UpdateTransform(const glm::vec3 &position, UniformBufferObject &ubo)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(0.0f, 0.0f, 0.0f));
        CopyMat4(ubo.model, model);
    }

    float SkyBox::GetRadius() const { return m_radius; }
    int SkyBox::GetSegments() const { return m_segments; }
    int SkyBox::GetRings() const { return m_rings; }

    void SkyBox::GenerateSphereMesh()
    {
        m_vertices.clear();
        m_indices.clear();

        for (int ring = 0; ring <= m_rings; ++ring)
        {
            float phi = glm::pi<float>() * float(ring) / float(m_rings);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            for (int seg = 0; seg <= m_segments; ++seg)
            {
                float theta = 2.0f * glm::pi<float>() * float(seg) / float(m_segments);
                float sinTheta = sin(theta);
                float cosTheta = cos(theta);

                Vertex vertex;

                float x = sinPhi * cosTheta;
                float y = cosPhi;
                float z = sinPhi * sinTheta;

                vertex.position[0] = m_radius * x;
                vertex.position[1] = m_radius * y;
                vertex.position[2] = m_radius * z;

                vertex.normal[0] = -x;
                vertex.normal[1] = -y;
                vertex.normal[2] = -z;

                vertex.texCoord[0] = float(seg) / float(m_segments);
                vertex.texCoord[1] = 1.0f - (float(ring) / float(m_rings));

                m_vertices.push_back(vertex);
            }
        }

        for (int ring = 0; ring < m_rings; ++ring)
        {
            for (int seg = 0; seg < m_segments; ++seg)
            {
                int current = ring * (m_segments + 1) + seg;
                int next = current + m_segments + 1;

                m_indices.push_back(current);
                m_indices.push_back(current + 1);
                m_indices.push_back(next);

                m_indices.push_back(current + 1);
                m_indices.push_back(next + 1);
                m_indices.push_back(next);
            }
        }

        m_mesh = m_gfx->CreateMesh(m_vertices, m_indices);

        std::cout << "✓ Skybox Sphere Generated (INVERTED FACES): "
                  << m_vertices.size() << " vertices, "
                  << m_indices.size() << " indices, "
                  << "radius: " << m_radius << "\n";
    }

    void SkyBox::CopyMat4(float *dest, const glm::mat4 &src)
    {
        memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
    }
}
