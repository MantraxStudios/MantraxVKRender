#pragma once

#include "MantraxGFX_API.h"
#include <vector>

namespace MantraxGFX
{
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 texcoord;
    };

    class Mesh
    {
    public:
        void CreateCube(ID3D12Device *device)
        {
            Vertex vertices[] = {
                // Front face
                {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

                // Back face
                {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
                {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
                {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
                {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

                // Top face
                {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

                // Bottom face
                {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
                {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
                {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},

                // Right face
                {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

                // Left face
                {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}};

            UINT indices[] = {
                0, 1, 2, 0, 2, 3,       // Front
                4, 5, 6, 4, 6, 7,       // Back
                8, 9, 10, 8, 10, 11,    // Top
                12, 13, 14, 12, 14, 15, // Bottom
                16, 17, 18, 16, 18, 19, // Right
                20, 21, 22, 20, 22, 23  // Left
            };

            CreateFromData(device, vertices, _countof(vertices), indices, _countof(indices));
        }

        void CreateSphere(ID3D12Device *device, float radius = 1.0f, UINT segments = 32, UINT rings = 16)
        {
            std::vector<Vertex> vertices;
            std::vector<UINT> indices;

            for (UINT ring = 0; ring <= rings; ring++)
            {
                float phi = XM_PI * ring / rings;
                for (UINT seg = 0; seg <= segments; seg++)
                {
                    float theta = 2.0f * XM_PI * seg / segments;

                    Vertex v;
                    v.position.x = radius * sinf(phi) * cosf(theta);
                    v.position.y = radius * cosf(phi);
                    v.position.z = radius * sinf(phi) * sinf(theta);

                    v.normal.x = v.position.x / radius;
                    v.normal.y = v.position.y / radius;
                    v.normal.z = v.position.z / radius;

                    v.texcoord.x = (float)seg / segments;
                    v.texcoord.y = (float)ring / rings;

                    vertices.push_back(v);
                }
            }

            for (UINT ring = 0; ring < rings; ring++)
            {
                for (UINT seg = 0; seg < segments; seg++)
                {
                    UINT current = ring * (segments + 1) + seg;
                    UINT next = current + segments + 1;

                    indices.push_back(current);
                    indices.push_back(next);
                    indices.push_back(current + 1);

                    indices.push_back(current + 1);
                    indices.push_back(next);
                    indices.push_back(next + 1);
                }
            }

            CreateFromData(device, vertices.data(), (UINT)vertices.size(), indices.data(), (UINT)indices.size());
        }

        void CreatePlane(ID3D12Device *device, float width = 10.0f, float depth = 10.0f)
        {
            Vertex vertices[] = {
                {{-width / 2, 0.0f, -depth / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                {{width / 2, 0.0f, -depth / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                {{width / 2, 0.0f, depth / 2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{-width / 2, 0.0f, depth / 2}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}};

            UINT indices[] = {0, 1, 2, 0, 2, 3};

            CreateFromData(device, vertices, _countof(vertices), indices, _countof(indices));
        }

        void Draw(ID3D12GraphicsCommandList *cmdList) const
        {
            cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
            cmdList->IASetIndexBuffer(&indexBufferView);
            cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
        }

        UINT GetIndexCount() const { return indexCount; }

    private:
        void CreateFromData(ID3D12Device *device, const Vertex *vertices, UINT vertexCount,
                            const UINT *indices, UINT indexCountParam)
        {
            indexCount = indexCountParam;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            // Vertex buffer
            D3D12_RESOURCE_DESC bufferDesc = {};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Width = sizeof(Vertex) * vertexCount;
            bufferDesc.Height = 1;
            bufferDesc.DepthOrArraySize = 1;
            bufferDesc.MipLevels = 1;
            bufferDesc.SampleDesc.Count = 1;
            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&vertexBuffer));

            UINT8 *pVertexDataBegin;
            vertexBuffer->Map(0, nullptr, reinterpret_cast<void **>(&pVertexDataBegin));
            memcpy(pVertexDataBegin, vertices, sizeof(Vertex) * vertexCount);
            vertexBuffer->Unmap(0, nullptr);

            vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
            vertexBufferView.StrideInBytes = sizeof(Vertex);
            vertexBufferView.SizeInBytes = sizeof(Vertex) * vertexCount;

            // Index buffer
            bufferDesc.Width = sizeof(UINT) * indexCount;
            device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&indexBuffer));

            UINT8 *pIndexDataBegin;
            indexBuffer->Map(0, nullptr, reinterpret_cast<void **>(&pIndexDataBegin));
            memcpy(pIndexDataBegin, indices, sizeof(UINT) * indexCount);
            indexBuffer->Unmap(0, nullptr);

            indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = sizeof(UINT) * indexCount;
        }

        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        UINT indexCount = 0;
    };
}