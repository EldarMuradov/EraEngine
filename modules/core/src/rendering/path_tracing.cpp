// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/path_tracing.h"

#include "core/color.h"

#include "raytracing.hlsli"

#define PATH_TRACING_RS_RESOURCES   0
#define PATH_TRACING_RS_CAMERA      1
#define PATH_TRACING_RS_CB          2

namespace era_engine
{
    void path_tracer::initialize()
    {
        const wchar* shaderPath = L"shaders/raytracing/path_tracing_rts.hlsl";

        const uint32 numInputResources = sizeof(input_resources) / sizeof(dx_cpu_descriptor_handle);
        const uint32 numOutputResources = sizeof(output_resources) / sizeof(dx_cpu_descriptor_handle);

        CD3DX12_DESCRIPTOR_RANGE resourceRanges[] =
        {
            // Must be input first, then output
            CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numInputResources, 0),
            CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numOutputResources, 0),
        };

        CD3DX12_ROOT_PARAMETER globalRootParameters[] =
        {
            root_descriptor_table(arraysize(resourceRanges), resourceRanges),
            root_cbv(0), // Camera
            root_constants<path_tracing_cb>(1),
        };

        CD3DX12_STATIC_SAMPLER_DESC globalStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

        D3D12_ROOT_SIGNATURE_DESC globalDesc =
        {
            arraysize(globalRootParameters), globalRootParameters,
            1, &globalStaticSampler
        };

        pbr_raytracer::initialize(shaderPath, settings.maxPayloadSize, settings.maxRecursionDepth, globalDesc);

        allocateDescriptorHeapSpaceForGlobalResources<input_resources, output_resources>(descriptorHeap);

        oldSettings = settings;
    }

    void path_tracer::render(dx_command_list* cl, const raytracing_tlas& tlas,
        const ref<dx_texture>& output,
        const common_render_data& common)
    {
        if (memcmp(&settings, &oldSettings, sizeof(path_tracer_settings)) != 0)
        {
            resetRendering();
            oldSettings = settings;
        }

        input_resources in;
        in.tlas = tlas.tlas->raytracingSRV;
        in.sky = common.sky->defaultSRV;

        output_resources out;
        out.output = output->defaultUAV;

        dx_gpu_descriptor_handle gpuHandle = copyGlobalResourcesToDescriptorHeap(in, out);

        // Fill out description
        D3D12_DISPATCH_RAYS_DESC raytraceDesc;
        fillOutRayTracingRenderDesc(bindingTable.getBuffer(), raytraceDesc,
            output->width, output->height, 1,
            numRayTypes, bindingTable.getNumberOfHitGroups());

        // Set up pipeline
        cl->setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap.descriptorHeap);

        cl->setPipelineState(pipeline.pipeline);
        cl->setComputeRootSignature(pipeline.rootSignature);

        uint32 depth = min(settings.recursionDepth, settings.maxRecursionDepth - 1);

        cl->setComputeDescriptorTable(PATH_TRACING_RS_RESOURCES, gpuHandle);
        cl->setComputeDynamicConstantBuffer(PATH_TRACING_RS_CAMERA, common.cameraCBV);
        cl->setCompute32BitConstants(PATH_TRACING_RS_CB,
            path_tracing_cb
            {
                (uint32)dxContext.frameID,
                numAveragedFrames,
                depth,
                clamp(settings.startRussianRouletteAfter, 0u, depth),
                (uint32)settings.useThinLensCamera,
                settings.focalLength,
                settings.focalLength / (2.f * settings.fNumber),
                (uint32)settings.useRealMaterials,
                (uint32)settings.enableDirectLighting,
                settings.lightIntensityScale,
                settings.pointLightRadius,
                (uint32)settings.multipleImportanceSampling,
            });

        cl->raytrace(raytraceDesc);

        ++numAveragedFrames;
    }

    void path_tracer::resetRendering()
    {
        numAveragedFrames = 0;
    }
}