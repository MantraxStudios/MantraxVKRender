#pragma once

#include "MantraxGFX_API.h"

namespace MantraxGFX
{
    class LightSystem
    {
    public:
        void Initialize(ID3D12Device *device, UINT maxLights = 8)
        {
            this->maxLights = maxLights;
            lights.reserve(maxLights);

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC bufferDesc = {};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Width = sizeof(Light) * maxLights;
            bufferDesc.Height = 1;
            bufferDesc.DepthOrArraySize = 1;
            bufferDesc.MipLevels = 1;
            bufferDesc.SampleDesc.Count = 1;
            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&lightsBuffer));
        }

        void AddLight(const XMFLOAT3 &position, const XMFLOAT3 &color, float intensity = 100.0f, float radius = 10.0f)
        {
            if (lights.size() < maxLights)
            {
                Light light;
                light.position = position;
                light.color = color;
                light.intensity = intensity;
                light.radius = radius;
                lights.push_back(light);
                needsUpdate = true;
            }
        }

        void ClearLights()
        {
            lights.clear();
            needsUpdate = true;
        }

        void UpdateBuffer()
        {
            if (needsUpdate && !lights.empty())
            {
                UINT8 *pData;
                lightsBuffer->Map(0, nullptr, reinterpret_cast<void **>(&pData));
                memcpy(pData, lights.data(), sizeof(Light) * lights.size());
                lightsBuffer->Unmap(0, nullptr);
                needsUpdate = false;
            }
        }

        ID3D12Resource *GetBuffer() const { return lightsBuffer.Get(); }
        UINT GetLightCount() const { return static_cast<UINT>(lights.size()); }

    private:
        ComPtr<ID3D12Resource> lightsBuffer;
        std::vector<Light> lights;
        UINT maxLights = 8;
        bool needsUpdate = false;
    };
}