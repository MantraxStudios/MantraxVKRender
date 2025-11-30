#pragma once

#include "MantraxGFX_API.h"

namespace MantraxGFX
{
    class RenderTarget
    {
    public:
        void Create(ID3D12Device *device, IDXGISwapChain3 *swapChain, UINT width, UINT height)
        {
            this->width = width;
            this->height = height;

            // Crear RTV heap
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = 2;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

            UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

            for (UINT i = 0; i < 2; i++)
            {
                swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
                device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
                rtvHandle.ptr += rtvDescriptorSize;
            }

            // Crear depth stencil
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

            D3D12_RESOURCE_DESC depthDesc = {};
            depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            depthDesc.Width = width;
            depthDesc.Height = height;
            depthDesc.DepthOrArraySize = 1;
            depthDesc.MipLevels = 1;
            depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthDesc.SampleDesc.Count = 1;
            depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;

            device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
                IID_PPV_ARGS(&depthStencil));

            device->CreateDepthStencilView(
                depthStencil.Get(), nullptr,
                dsvHeap->GetCPUDescriptorHandleForHeapStart());
        }

        void BeginRender(ID3D12GraphicsCommandList *cmdList, UINT frameIndex,
                         ID3D12Device *device, const float *clearColor = nullptr)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = renderTargets[frameIndex].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            cmdList->ResourceBarrier(1, &barrier);

            UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += frameIndex * rtvDescriptorSize;
            D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

            cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

            float defaultClearColor[] = {0.1f, 0.1f, 0.15f, 1.0f};
            const float *color = clearColor ? clearColor : defaultClearColor;
            cmdList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
            cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            D3D12_VIEWPORT viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
            D3D12_RECT scissorRect = {0, 0, (LONG)width, (LONG)height};
            cmdList->RSSetViewports(1, &viewport);
            cmdList->RSSetScissorRects(1, &scissorRect);
        }

        void EndRender(ID3D12GraphicsCommandList *cmdList, UINT frameIndex)
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = renderTargets[frameIndex].Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
            cmdList->ResourceBarrier(1, &barrier);
        }

    private:
        ComPtr<ID3D12DescriptorHeap> rtvHeap;
        ComPtr<ID3D12DescriptorHeap> dsvHeap;
        ComPtr<ID3D12Resource> renderTargets[2];
        ComPtr<ID3D12Resource> depthStencil;
        UINT width = 0;
        UINT height = 0;
    };
}