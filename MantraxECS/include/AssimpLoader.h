#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../MantraxRender/include/MantraxGFX_API.h"

namespace Mantrax
{
    class AssimpLoader
    {
    public:
        AssimpLoader() = default;
        ~AssimpLoader() = default;

        struct LoadSettings
        {
            bool triangulate = true;
            bool genNormals = true;
            bool flipUVs = true;
            bool calcTangents = true;
            bool preTransform = true;
            bool globalScale = true;

            unsigned int GetFlags() const
            {
                unsigned int flags = 0;
                if (triangulate)
                    flags |= aiProcess_Triangulate;
                if (genNormals)
                    flags |= aiProcess_GenNormals;
                if (flipUVs)
                    flags |= aiProcess_FlipUVs;
                if (calcTangents)
                    flags |= aiProcess_CalcTangentSpace;
                if (preTransform)
                    flags |= aiProcess_PreTransformVertices;
                if (globalScale)
                    flags |= aiProcess_GlobalScale;
                return flags;
            }
        };

        // Información del modelo cargado
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
        };

        /**
         * Carga un modelo 3D desde un archivo usando Assimp (versión simplificada)
         * @param path Ruta al archivo del modelo
         * @param vertices Vector de salida para los vértices
         * @param indices Vector de salida para los índices
         * @param verbose Mostrar información detallada en consola
         * @return true si la carga fue exitosa, false en caso contrario
         */
        bool LoadModel(
            const std::string &path,
            std::vector<Mantrax::Vertex> &vertices,
            std::vector<uint32_t> &indices,
            bool verbose = true)
        {
            LoadSettings defaultSettings;
            return LoadModel(path, vertices, indices, defaultSettings, verbose);
        }

        /**
         * Carga un modelo 3D desde un archivo usando Assimp (versión completa)
         * @param path Ruta al archivo del modelo
         * @param vertices Vector de salida para los vértices
         * @param indices Vector de salida para los índices
         * @param settings Configuración de carga
         * @param verbose Mostrar información detallada en consola
         * @return true si la carga fue exitosa, false en caso contrario
         */
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
                std::cerr << "ERROR: Assimp failed to load model: " << path << "\n";
                std::cerr << "Assimp Error: " << m_lastError << "\n";
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

            // Procesar todas las mallas
            uint32_t indexOffset = 0;
            for (unsigned int m = 0; m < scene->mNumMeshes; m++)
            {
                aiMesh *mesh = scene->mMeshes[m];

                if (verbose)
                {
                    std::cout << "  Mesh " << m << ": "
                              << mesh->mNumVertices << " verts, "
                              << mesh->mNumFaces << " faces\n";
                }

                ProcessMesh(mesh, vertices, indices, indexOffset);
                indexOffset += mesh->mNumVertices;
            }

            // Actualizar información del modelo
            m_modelInfo.totalVertices = static_cast<unsigned int>(vertices.size());
            m_modelInfo.totalIndices = static_cast<unsigned int>(indices.size());

            if (verbose)
            {
                std::cout << "Total vertices: " << m_modelInfo.totalVertices << "\n";
                std::cout << "Total indices: " << m_modelInfo.totalIndices << "\n";
                std::cout << "==========================\n\n";
            }

            return true;
        }

        /**
         * Obtiene el último error ocurrido
         */
        const std::string &GetLastError() const { return m_lastError; }

        /**
         * Obtiene información del último modelo cargado
         */
        const ModelInfo &GetModelInfo() const { return m_modelInfo; }

    private:
        std::string m_lastError;
        ModelInfo m_modelInfo;

        void ProcessMesh(
            aiMesh *mesh,
            std::vector<Mantrax::Vertex> &vertices,
            std::vector<uint32_t> &indices,
            uint32_t indexOffset)
        {
            // Actualizar flags de información
            m_modelInfo.hasNormals = mesh->HasNormals();
            m_modelInfo.hasTexCoords = mesh->HasTextureCoords(0);
            m_modelInfo.hasTangents = mesh->HasTangentsAndBitangents();

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
                    vertex.normal[0] = 0.0f;
                    vertex.normal[1] = 1.0f;
                    vertex.normal[2] = 0.0f;
                }

                // Coordenadas de textura
                if (mesh->HasTextureCoords(0))
                {
                    vertex.texCoord[0] = mesh->mTextureCoords[0][i].x;
                    vertex.texCoord[1] = mesh->mTextureCoords[0][i].y;
                }
                else
                {
                    vertex.texCoord[0] = 0.0f;
                    vertex.texCoord[1] = 0.0f;
                }

                // Color por defecto (blanco)
                vertex.color[0] = 1.0f;
                vertex.color[1] = 1.0f;
                vertex.color[2] = 1.0f;

                vertices.push_back(vertex);
            }

            // Procesar índices
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                {
                    indices.push_back(indexOffset + face.mIndices[j]);
                }
            }
        }

        void PrintModelInfo(const aiScene *scene, const std::string &path)
        {
            std::cout << "\n=== Model Loading Info ===\n";
            std::cout << "File: " << path << "\n";
            std::cout << "Meshes: " << scene->mNumMeshes << "\n";
            std::cout << "Materials: " << scene->mNumMaterials << "\n";
        }
    };
}