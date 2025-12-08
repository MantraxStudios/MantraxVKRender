#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../MantraxRender/include/MantraxGFX_API.h"
#include "EngineLoaderDLL.h"

namespace Mantrax
{
    class MANTRAX_API AssimpLoader
    {
    public:
        AssimpLoader() {}
        ~AssimpLoader() {}

        struct LoadSettings
        {
            bool triangulate = true;
            bool genNormals = true;
            bool flipUVs = false; // ❌ CAMBIADO: false por defecto (OpenGL ya lo hace)
            bool calcTangents = true;
            bool preTransform = true;
            bool globalScale = true;
            bool genSmoothNormals = false; // ✅ NUEVO: opción para normales suaves

            unsigned int GetFlags() const
            {
                unsigned int flags = 0;
                if (triangulate)
                    flags |= aiProcess_Triangulate;
                if (genNormals)
                    flags |= aiProcess_GenNormals;
                if (genSmoothNormals)
                    flags |= aiProcess_GenSmoothNormals; // Alternativa a GenNormals
                if (flipUVs)
                    flags |= aiProcess_FlipUVs;
                if (calcTangents)
                    flags |= aiProcess_CalcTangentSpace;
                if (preTransform)
                    flags |= aiProcess_PreTransformVertices;
                if (globalScale)
                    flags |= aiProcess_GlobalScale;

                // ✅ Flags adicionales recomendados
                flags |= aiProcess_JoinIdenticalVertices; // Optimiza vértices duplicados
                flags |= aiProcess_ImproveCacheLocality;  // Optimiza para cache GPU
                flags |= aiProcess_ValidateDataStructure; // Valida la estructura

                return flags;
            }
        };

        struct ModelInfo
        {
            std::string filePath;
            unsigned int numMeshes = 0;
            unsigned int numMaterials = 0;
            unsigned int totalVertices = 0;
            unsigned int totalIndices = 0;
            bool hasNormals = false;
            bool hasTexCoords = false;
            bool hasTangents = false;
            bool hasColors = false;
        };

        bool LoadModel(
            const std::string &path,
            std::vector<Mantrax::Vertex> &vertices,
            std::vector<uint32_t> &indices,
            bool verbose = true)
        {
            LoadSettings defaultSettings;
            return LoadModel(path, vertices, indices, defaultSettings, verbose);
        }

        bool LoadModel(
            const std::string &path,
            std::vector<Mantrax::Vertex> &vertices,
            std::vector<uint32_t> &indices,
            const LoadSettings &settings,
            bool verbose = true)
        {
            vertices.clear();
            indices.clear();

            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(path, settings.GetFlags());

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                m_lastError = importer.GetErrorString();
                std::cerr << "❌ ERROR: Assimp failed to load model: " << path << "\n";
                std::cerr << "   Assimp Error: " << m_lastError << "\n";
                return false;
            }

            // Guardar información del modelo
            m_modelInfo.filePath = path;
            m_modelInfo.numMeshes = scene->mNumMeshes;
            m_modelInfo.numMaterials = scene->mNumMaterials;

            if (verbose)
            {
                PrintModelInfo(scene, path);
            }

            // ✅ FIX CRÍTICO: No usar indexOffset acumulativo
            // Procesar todas las mallas
            for (unsigned int m = 0; m < scene->mNumMeshes; m++)
            {
                aiMesh *mesh = scene->mMeshes[m];

                if (verbose)
                {
                    std::cout << "  📦 Mesh " << m << ": "
                              << mesh->mNumVertices << " verts, "
                              << mesh->mNumFaces << " faces";

                    if (mesh->HasTextureCoords(0))
                        std::cout << " [has UVs]";
                    if (mesh->HasNormals())
                        std::cout << " [has normals]";
                    if (mesh->HasVertexColors(0))
                        std::cout << " [has colors]";

                    std::cout << "\n";
                }

                // ✅ Cada mesh agrega sus vértices/índices independientemente
                uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());
                ProcessMesh(mesh, vertices, indices, vertexOffset);
            }

            // Actualizar información del modelo
            m_modelInfo.totalVertices = static_cast<unsigned int>(vertices.size());
            m_modelInfo.totalIndices = static_cast<unsigned int>(indices.size());

            if (verbose)
            {
                std::cout << "\n✅ Loading complete:\n";
                std::cout << "   Total vertices: " << m_modelInfo.totalVertices << "\n";
                std::cout << "   Total indices: " << m_modelInfo.totalIndices << "\n";
                std::cout << "   Triangles: " << (m_modelInfo.totalIndices / 3) << "\n";

                // ✅ Mostrar rango de UVs para debug
                if (m_modelInfo.hasTexCoords && !vertices.empty())
                {
                    float minU = 999.0f, maxU = -999.0f;
                    float minV = 999.0f, maxV = -999.0f;

                    for (const auto &v : vertices)
                    {
                        minU = std::min(minU, v.texCoord[0]);
                        maxU = std::max(maxU, v.texCoord[0]);
                        minV = std::min(minV, v.texCoord[1]);
                        maxV = std::max(maxV, v.texCoord[1]);
                    }

                    std::cout << "   UV range: U[" << minU << " to " << maxU
                              << "] V[" << minV << " to " << maxV << "]\n";

                    if (minU < 0.0f || maxU > 1.0f || minV < 0.0f || maxV > 1.0f)
                    {
                        std::cout << "   ⚠️ UVs outside [0,1] range detected\n";
                    }
                }

                std::cout << "==========================\n\n";
            }

            return true;
        }

        const std::string &GetLastError() const { return m_lastError; }
        const ModelInfo &GetModelInfo() const { return m_modelInfo; }

    private:
        std::string m_lastError;
        ModelInfo m_modelInfo;

        void ProcessMesh(
            aiMesh *mesh,
            std::vector<Mantrax::Vertex> &vertices,
            std::vector<uint32_t> &indices,
            uint32_t vertexOffset)
        {
            // Actualizar flags de información (acumulativo para múltiples meshes)
            m_modelInfo.hasNormals |= mesh->HasNormals();
            m_modelInfo.hasTexCoords |= mesh->HasTextureCoords(0);
            m_modelInfo.hasTangents |= mesh->HasTangentsAndBitangents();
            m_modelInfo.hasColors |= mesh->HasVertexColors(0);

            // Procesar vértices
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Mantrax::Vertex vertex;

                // Posición
                vertex.position[0] = mesh->mVertices[i].x;
                vertex.position[1] = mesh->mVertices[i].y;
                vertex.position[2] = mesh->mVertices[i].z;

                // Normales
                if (mesh->HasNormals())
                {
                    vertex.normal[0] = mesh->mNormals[i].x;
                    vertex.normal[1] = mesh->mNormals[i].y;
                    vertex.normal[2] = mesh->mNormals[i].z;
                }
                else
                {
                    // Default: normal apuntando hacia arriba
                    vertex.normal[0] = 0.0f;
                    vertex.normal[1] = 1.0f;
                    vertex.normal[2] = 0.0f;
                }

                // ✅ Coordenadas de textura (SIN FLIP manual)
                if (mesh->HasTextureCoords(0))
                {
                    vertex.texCoord[0] = mesh->mTextureCoords[0][i].x;
                    vertex.texCoord[1] = mesh->mTextureCoords[0][i].y;

                    // ✅ OPCIONAL: Si aún se ve mal, descomentar esto:
                    // vertex.texCoord[1] = 1.0f - mesh->mTextureCoords[0][i].y;
                }
                else
                {
                    vertex.texCoord[0] = 0.0f;
                    vertex.texCoord[1] = 0.0f;
                }

                // ✅ Color: usar color del mesh si existe, sino blanco
                if (mesh->HasVertexColors(0))
                {
                    vertex.color[0] = mesh->mColors[0][i].r;
                    vertex.color[1] = mesh->mColors[0][i].g;
                    vertex.color[2] = mesh->mColors[0][i].b;
                }
                else
                {
                    vertex.color[0] = 1.0f;
                    vertex.color[1] = 1.0f;
                    vertex.color[2] = 1.0f;
                }

                vertices.push_back(vertex);
            }

            // ✅ Procesar índices con offset correcto
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];

                // ✅ Verificar que la cara es un triángulo
                if (face.mNumIndices != 3)
                {
                    std::cerr << "⚠️ Warning: Face " << i << " has "
                              << face.mNumIndices << " indices (expected 3)\n";
                    continue;
                }

                for (unsigned int j = 0; j < face.mNumIndices; j++)
                {
                    indices.push_back(vertexOffset + face.mIndices[j]);
                }
            }
        }

        void PrintModelInfo(const aiScene *scene, const std::string &path)
        {
            std::cout << "\n=== 📁 Model Loading Info ===\n";
            std::cout << "File: " << path << "\n";
            std::cout << "Meshes: " << scene->mNumMeshes << "\n";
            std::cout << "Materials: " << scene->mNumMaterials << "\n";

            // ✅ Info adicional útil
            if (scene->HasAnimations())
            {
                std::cout << "Animations: " << scene->mNumAnimations << "\n";
            }
            if (scene->HasLights())
            {
                std::cout << "Lights: " << scene->mNumLights << "\n";
            }
            if (scene->HasCameras())
            {
                std::cout << "Cameras: " << scene->mNumCameras << "\n";
            }

            std::cout << "\n";
        }
    };
}