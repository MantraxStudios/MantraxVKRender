#pragma once
#include <vector>
#include <cmath>
#include <random>
#include <glm/glm.hpp>
#include "../render/include/MantraxGFX_API.h"

namespace Mantrax
{
    // ========================================================================
    // TIPOS DE BLOQUES
    // ========================================================================
    enum class BlockType : uint8_t
    {
        Air = 0,
        Grass,
        Dirt,
        Stone,
        Sand,
        Water,
        Wood,
        Leaves
    };

    // ========================================================================
    // CONFIGURACIÓN DE CHUNK
    // ========================================================================
    struct ChunkConfig
    {
        int chunkSizeX = 16;
        int chunkSizeY = 64;
        int chunkSizeZ = 16;
        int seed = 12345;
        float noiseScale = 0.05f;
        float terrainHeight = 32.0f;
        float terrainAmplitude = 16.0f;
    };

    // ========================================================================
    // CHUNK - Generación de terreno estilo Minecraft
    // ========================================================================
    class MinecraftChunk
    {
    public:
        MinecraftChunk(const ChunkConfig &config = ChunkConfig{})
            : m_Config(config)
        {
            int totalBlocks = m_Config.chunkSizeX * m_Config.chunkSizeY * m_Config.chunkSizeZ;
            m_Blocks.resize(totalBlocks, BlockType::Air);
        }

        // Generar terreno usando Perlin-like noise
        void GenerateTerrain()
        {
            std::mt19937 rng(m_Config.seed);
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            for (int x = 0; x < m_Config.chunkSizeX; ++x)
            {
                for (int z = 0; z < m_Config.chunkSizeZ; ++z)
                {
                    // Generar altura usando noise simple
                    float height = GenerateHeight(x, z);
                    int blockHeight = static_cast<int>(height);

                    // Limitar altura al chunk
                    blockHeight = std::min(blockHeight, m_Config.chunkSizeY - 1);

                    for (int y = 0; y < m_Config.chunkSizeY; ++y)
                    {
                        BlockType blockType = BlockType::Air;

                        if (y < blockHeight - 4)
                        {
                            blockType = BlockType::Stone;
                        }
                        else if (y < blockHeight - 1)
                        {
                            blockType = BlockType::Dirt;
                        }
                        else if (y == blockHeight - 1)
                        {
                            if (y < 28)
                                blockType = BlockType::Sand;
                            else
                                blockType = BlockType::Grass;
                        }
                        else if (y < 25) // Nivel del agua
                        {
                            blockType = BlockType::Water;
                        }

                        SetBlock(x, y, z, blockType);
                    }

                    // Agregar árboles ocasionalmente
                    if (blockHeight > 28 && dist(rng) > 0.95f)
                    {
                        GenerateTree(x, blockHeight, z);
                    }
                }
            }
        }

        // Construir la malla combinada (solo caras visibles)
        void BuildMesh(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices)
        {
            vertices.clear();
            indices.clear();

            for (int x = 0; x < m_Config.chunkSizeX; ++x)
            {
                for (int y = 0; y < m_Config.chunkSizeY; ++y)
                {
                    for (int z = 0; z < m_Config.chunkSizeZ; ++z)
                    {
                        BlockType block = GetBlock(x, y, z);
                        if (block == BlockType::Air)
                            continue;

                        // Solo agregar caras que están expuestas al aire
                        glm::vec3 blockColor = GetBlockColor(block);

                        // Cara frontal (+Z)
                        if (IsAir(x, y, z + 1))
                            AddFace(vertices, indices, x, y, z, 0, blockColor);

                        // Cara trasera (-Z)
                        if (IsAir(x, y, z - 1))
                            AddFace(vertices, indices, x, y, z, 1, blockColor);

                        // Cara derecha (+X)
                        if (IsAir(x + 1, y, z))
                            AddFace(vertices, indices, x, y, z, 2, blockColor);

                        // Cara izquierda (-X)
                        if (IsAir(x - 1, y, z))
                            AddFace(vertices, indices, x, y, z, 3, blockColor);

                        // Cara superior (+Y)
                        if (IsAir(x, y + 1, z))
                            AddFace(vertices, indices, x, y, z, 4, blockColor);

                        // Cara inferior (-Y)
                        if (IsAir(x, y - 1, z))
                            AddFace(vertices, indices, x, y, z, 5, blockColor);
                    }
                }
            }
        }

    private:
        ChunkConfig m_Config;
        std::vector<BlockType> m_Blocks;

        // Obtener índice en el array 1D
        int GetIndex(int x, int y, int z) const
        {
            if (x < 0 || x >= m_Config.chunkSizeX ||
                y < 0 || y >= m_Config.chunkSizeY ||
                z < 0 || z >= m_Config.chunkSizeZ)
                return -1;

            return x + z * m_Config.chunkSizeX + y * m_Config.chunkSizeX * m_Config.chunkSizeZ;
        }

        BlockType GetBlock(int x, int y, int z) const
        {
            int idx = GetIndex(x, y, z);
            if (idx < 0 || idx >= static_cast<int>(m_Blocks.size()))
                return BlockType::Air;
            return m_Blocks[idx];
        }

        void SetBlock(int x, int y, int z, BlockType type)
        {
            int idx = GetIndex(x, y, z);
            if (idx >= 0 && idx < static_cast<int>(m_Blocks.size()))
                m_Blocks[idx] = type;
        }

        bool IsAir(int x, int y, int z) const
        {
            BlockType block = GetBlock(x, y, z);
            return block == BlockType::Air || block == BlockType::Water;
        }

        // Generar altura usando múltiples octavas de noise
        float GenerateHeight(int x, int z) const
        {
            float height = m_Config.terrainHeight;

            // Octava 1: Terreno principal
            float n1 = SimplexNoise(x * m_Config.noiseScale, z * m_Config.noiseScale, m_Config.seed);
            height += n1 * m_Config.terrainAmplitude;

            // Octava 2: Detalles
            float n2 = SimplexNoise(x * m_Config.noiseScale * 2.0f, z * m_Config.noiseScale * 2.0f, m_Config.seed + 1);
            height += n2 * m_Config.terrainAmplitude * 0.5f;

            // Octava 3: Detalles finos
            float n3 = SimplexNoise(x * m_Config.noiseScale * 4.0f, z * m_Config.noiseScale * 4.0f, m_Config.seed + 2);
            height += n3 * m_Config.terrainAmplitude * 0.25f;

            return height;
        }

        // Simplex Noise simplificado (aproximación)
        float SimplexNoise(float x, float y, int seed) const
        {
            // Hash simple para generar pseudo-random
            auto hash = [](int x, int y, int seed) -> float
            {
                int n = x + y * 57 + seed * 131;
                n = (n << 13) ^ n;
                return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
            };

            int xi = static_cast<int>(std::floor(x));
            int yi = static_cast<int>(std::floor(y));

            float xf = x - xi;
            float yf = y - yi;

            // Interpolación suave
            float u = xf * xf * (3.0f - 2.0f * xf);
            float v = yf * yf * (3.0f - 2.0f * yf);

            float a = hash(xi, yi, seed);
            float b = hash(xi + 1, yi, seed);
            float c = hash(xi, yi + 1, seed);
            float d = hash(xi + 1, yi + 1, seed);

            float x1 = a + (b - a) * u;
            float x2 = c + (d - c) * u;

            return (x1 + (x2 - x1) * v) * 0.5f + 0.5f;
        }

        void GenerateTree(int x, int baseY, int z)
        {
            int trunkHeight = 4;
            int leavesRadius = 2;

            // Tronco
            for (int y = 0; y < trunkHeight; ++y)
            {
                SetBlock(x, baseY + y, z, BlockType::Wood);
            }

            // Hojas (esfera aproximada)
            int topY = baseY + trunkHeight;
            for (int dx = -leavesRadius; dx <= leavesRadius; ++dx)
            {
                for (int dy = -leavesRadius; dy <= leavesRadius; ++dy)
                {
                    for (int dz = -leavesRadius; dz <= leavesRadius; ++dz)
                    {
                        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                        if (dist <= leavesRadius)
                        {
                            int leafX = x + dx;
                            int leafY = topY + dy;
                            int leafZ = z + dz;

                            if (GetBlock(leafX, leafY, leafZ) == BlockType::Air)
                            {
                                SetBlock(leafX, leafY, leafZ, BlockType::Leaves);
                            }
                        }
                    }
                }
            }
        }

        glm::vec3 GetBlockColor(BlockType type) const
        {
            switch (type)
            {
            case BlockType::Grass:
                return glm::vec3(0.2f, 0.8f, 0.2f);
            case BlockType::Dirt:
                return glm::vec3(0.6f, 0.4f, 0.2f);
            case BlockType::Stone:
                return glm::vec3(0.5f, 0.5f, 0.5f);
            case BlockType::Sand:
                return glm::vec3(0.9f, 0.9f, 0.6f);
            case BlockType::Water:
                return glm::vec3(0.2f, 0.4f, 0.8f);
            case BlockType::Wood:
                return glm::vec3(0.4f, 0.25f, 0.1f);
            case BlockType::Leaves:
                return glm::vec3(0.1f, 0.6f, 0.1f);
            default:
                return glm::vec3(1.0f, 0.0f, 1.0f); // Magenta para debug
            }
        }

        // Agregar una cara del cubo - MODIFICADO para incluir coordenadas baricéntricas
        void AddFace(std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
                     int x, int y, int z, int face, const glm::vec3 &color)
        {
            uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

            float x0 = static_cast<float>(x);
            float y0 = static_cast<float>(y);
            float z0 = static_cast<float>(z);
            float x1 = x0 + 1.0f;
            float y1 = y0 + 1.0f;
            float z1 = z0 + 1.0f;

            // Centrar el chunk
            float offsetX = -m_Config.chunkSizeX * 0.5f;
            float offsetZ = -m_Config.chunkSizeZ * 0.5f;

            x0 += offsetX;
            x1 += offsetX;
            z0 += offsetZ;
            z1 += offsetZ;

            Vertex v0, v1, v2, v3;
            v0.color[0] = v1.color[0] = v2.color[0] = v3.color[0] = color.r;
            v0.color[1] = v1.color[1] = v2.color[1] = v3.color[1] = color.g;
            v0.color[2] = v1.color[2] = v2.color[2] = v3.color[2] = color.b;

            switch (face)
            {
            case 0: // Frontal (+Z)
                v0.position[0] = x0;
                v0.position[1] = y0;
                v0.position[2] = z1;
                v1.position[0] = x1;
                v1.position[1] = y0;
                v1.position[2] = z1;
                v2.position[0] = x1;
                v2.position[1] = y1;
                v2.position[2] = z1;
                v3.position[0] = x0;
                v3.position[1] = y1;
                v3.position[2] = z1;
                v0.normal[0] = v1.normal[0] = v2.normal[0] = v3.normal[0] = 0.0f;
                v0.normal[1] = v1.normal[1] = v2.normal[1] = v3.normal[1] = 0.0f;
                v0.normal[2] = v1.normal[2] = v2.normal[2] = v3.normal[2] = 1.0f;
                break;

            case 1: // Trasera (-Z)
                v0.position[0] = x1;
                v0.position[1] = y0;
                v0.position[2] = z0;
                v1.position[0] = x0;
                v1.position[1] = y0;
                v1.position[2] = z0;
                v2.position[0] = x0;
                v2.position[1] = y1;
                v2.position[2] = z0;
                v3.position[0] = x1;
                v3.position[1] = y1;
                v3.position[2] = z0;
                v0.normal[0] = v1.normal[0] = v2.normal[0] = v3.normal[0] = 0.0f;
                v0.normal[1] = v1.normal[1] = v2.normal[1] = v3.normal[1] = 0.0f;
                v0.normal[2] = v1.normal[2] = v2.normal[2] = v3.normal[2] = -1.0f;
                break;

            case 2: // Derecha (+X)
                v0.position[0] = x1;
                v0.position[1] = y0;
                v0.position[2] = z1;
                v1.position[0] = x1;
                v1.position[1] = y0;
                v1.position[2] = z0;
                v2.position[0] = x1;
                v2.position[1] = y1;
                v2.position[2] = z0;
                v3.position[0] = x1;
                v3.position[1] = y1;
                v3.position[2] = z1;
                v0.normal[0] = v1.normal[0] = v2.normal[0] = v3.normal[0] = 1.0f;
                v0.normal[1] = v1.normal[1] = v2.normal[1] = v3.normal[1] = 0.0f;
                v0.normal[2] = v1.normal[2] = v2.normal[2] = v3.normal[2] = 0.0f;
                break;

            case 3: // Izquierda (-X)
                v0.position[0] = x0;
                v0.position[1] = y0;
                v0.position[2] = z0;
                v1.position[0] = x0;
                v1.position[1] = y0;
                v1.position[2] = z1;
                v2.position[0] = x0;
                v2.position[1] = y1;
                v2.position[2] = z1;
                v3.position[0] = x0;
                v3.position[1] = y1;
                v3.position[2] = z0;
                v0.normal[0] = v1.normal[0] = v2.normal[0] = v3.normal[0] = -1.0f;
                v0.normal[1] = v1.normal[1] = v2.normal[1] = v3.normal[1] = 0.0f;
                v0.normal[2] = v1.normal[2] = v2.normal[2] = v3.normal[2] = 0.0f;
                break;

            case 4: // Superior (+Y)
                v0.position[0] = x0;
                v0.position[1] = y1;
                v0.position[2] = z1;
                v1.position[0] = x1;
                v1.position[1] = y1;
                v1.position[2] = z1;
                v2.position[0] = x1;
                v2.position[1] = y1;
                v2.position[2] = z0;
                v3.position[0] = x0;
                v3.position[1] = y1;
                v3.position[2] = z0;
                v0.normal[0] = v1.normal[0] = v2.normal[0] = v3.normal[0] = 0.0f;
                v0.normal[1] = v1.normal[1] = v2.normal[1] = v3.normal[1] = 1.0f;
                v0.normal[2] = v1.normal[2] = v2.normal[2] = v3.normal[2] = 0.0f;
                break;

            case 5: // Inferior (-Y)
                v0.position[0] = x0;
                v0.position[1] = y0;
                v0.position[2] = z0;
                v1.position[0] = x1;
                v1.position[1] = y0;
                v1.position[2] = z0;
                v2.position[0] = x1;
                v2.position[1] = y0;
                v2.position[2] = z1;
                v3.position[0] = x0;
                v3.position[1] = y0;
                v3.position[2] = z1;
                v0.normal[0] = v1.normal[0] = v2.normal[0] = v3.normal[0] = 0.0f;
                v0.normal[1] = v1.normal[1] = v2.normal[1] = v3.normal[1] = -1.0f;
                v0.normal[2] = v1.normal[2] = v2.normal[2] = v3.normal[2] = 0.0f;
                break;
            }

            // UVs simples
            v0.texCoord[0] = 0.0f;
            v0.texCoord[1] = 0.0f;
            v1.texCoord[0] = 1.0f;
            v1.texCoord[1] = 0.0f;
            v2.texCoord[0] = 1.0f;
            v2.texCoord[1] = 1.0f;
            v3.texCoord[0] = 0.0f;
            v3.texCoord[1] = 1.0f;

            // ============================================
            // NUEVO: Asignar coordenadas baricéntricas
            // ============================================
            // Para cada quad (4 vértices formando 2 triángulos):
            // - Triángulo 1: v0, v1, v2 -> barycentric (1,0,0), (0,1,0), (0,0,1)
            // - Triángulo 2: v2, v3, v0 -> barycentric (0,0,1), (1,0,0), (0,1,0)
            //
            // Asignamos a cada vértice su coordenada baricéntrica correspondiente:
            v0.barycentric[0] = 1.0f;
            v0.barycentric[1] = 0.0f;
            v0.barycentric[2] = 0.0f;
            v1.barycentric[0] = 0.0f;
            v1.barycentric[1] = 1.0f;
            v1.barycentric[2] = 0.0f;
            v2.barycentric[0] = 0.0f;
            v2.barycentric[1] = 0.0f;
            v2.barycentric[2] = 1.0f;
            v3.barycentric[0] = 1.0f;
            v3.barycentric[1] = 0.0f;
            v3.barycentric[2] = 0.0f;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);

            // Dos triángulos por cara
            // Triángulo 1: v0 -> v1 -> v2
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);

            // Triángulo 2: v2 -> v3 -> v0
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 0);
        }
    };

} // namespace Mantrax