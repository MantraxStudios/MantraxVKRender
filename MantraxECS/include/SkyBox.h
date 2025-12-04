#pragma once

#include "../MantraxRender/include/MantraxGFX_API.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <memory>
#include <string>
#include <iostream>

#include "../MantraxECS/include/TextureLoader.h" // Usar la clase TextureLoader

namespace Mantrax
{
    class SkyBox
    {
    public:
        // Radio grande por defecto (500 = skybox real)
        SkyBox(GFX *gfx, float radius = 5.0f, int segments = 64, int rings = 32)
            : m_gfx(gfx), m_radius(radius), m_segments(segments), m_rings(rings)
        {
            GenerateSphereMesh();
        }

        ~SkyBox() = default;

        bool LoadTexture(const std::string &texturePath, bool flipVertically = false)
        {
            try
            {
                // Usar TextureLoader para cargar la imagen
                auto textureData = TextureLoader::LoadFromFile(texturePath, flipVertically);

                if (!textureData || !textureData->pixels)
                {
                    std::cerr << "Failed to load skybox texture: " << texturePath << std::endl;
                    return false;
                }

                // Crear textura en el GFX
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

        void CreateMaterial(std::shared_ptr<Shader> shader)
        {
            m_material = m_gfx->CreateMaterial(shader);

            // Color blanco para no alterar la textura (o más brillante si quieres)
            m_material->SetBaseColor(1.0f, 1.0f, 1.0f);
            m_material->SetMetallicFactor(0.0f);
            m_material->SetRoughnessFactor(1.0f);

            // Configurar textura si existe
            if (m_texture)
            {
                m_gfx->SetMaterialPBRTextures(
                    m_material,
                    m_texture, // Albedo (skybox texture)
                    nullptr,   // Normal
                    nullptr,   // Metallic
                    nullptr,   // Roughness
                    nullptr    // AO
                );
            }
        }

        RenderObject GetRenderObject()
        {
            return RenderObject(m_mesh, m_material);
        }

        std::shared_ptr<Material> GetMaterial() const
        {
            return m_material;
        }

        std::shared_ptr<Mesh> GetMesh() const
        {
            return m_mesh;
        }

        // CAMBIADO: No seguir la cámara, posición fija para debug
        void UpdateTransform(const glm::vec3 &position, UniformBufferObject &ubo)
        {
            // Posición fija en el origen para debug
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
            CopyMat4(ubo.model, model);
        }

        float GetRadius() const { return m_radius; }
        int GetSegments() const { return m_segments; }
        int GetRings() const { return m_rings; }

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

        void GenerateSphereMesh()
        {
            m_vertices.clear();
            m_indices.clear();

            // Generar vértices de la esfera
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

                    // Posición en la esfera
                    float x = sinPhi * cosTheta;
                    float y = cosPhi;
                    float z = sinPhi * sinTheta;

                    vertex.position[0] = m_radius * x;
                    vertex.position[1] = m_radius * y;
                    vertex.position[2] = m_radius * z;

                    // Normal apuntando hacia DENTRO (invertida)
                    vertex.normal[0] = -x;
                    vertex.normal[1] = -y;
                    vertex.normal[2] = -z;

                    // UVs para textura equirectangular (invertir Y)
                    vertex.texCoord[0] = float(seg) / float(m_segments);
                    vertex.texCoord[1] = 1.0f - (float(ring) / float(m_rings));

                    m_vertices.push_back(vertex);
                }
            }

            // Winding normal (counter-clockwise desde fuera = visible desde dentro con back-face culling disabled)
            for (int ring = 0; ring < m_rings; ++ring)
            {
                for (int seg = 0; seg < m_segments; ++seg)
                {
                    int current = ring * (m_segments + 1) + seg;
                    int next = current + m_segments + 1;

                    // Winding normal
                    // Primer triángulo
                    m_indices.push_back(current);
                    m_indices.push_back(current + 1);
                    m_indices.push_back(next);

                    // Segundo triángulo
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

        void CopyMat4(float *dest, const glm::mat4 &src)
        {
            memcpy(dest, glm::value_ptr(src), 16 * sizeof(float));
        }
    };

} // namespace Mantrax