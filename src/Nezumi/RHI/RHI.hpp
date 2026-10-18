// =============================================================================
// RHI.hpp
//
// Umbrella header.
// =============================================================================

#pragma once

// Core
#include "Nezumi/RHI/BackendType.hpp"
#include "Nezumi/RHI/CommandEncoder.hpp"
#include "Nezumi/RHI/Device.hpp"
#include "Nezumi/RHI/DeviceConfig.hpp"
#include "Nezumi/RHI/DeviceInfo.hpp"

// BindGroup
#include "Nezumi/RHI/BindGroup/BindGroupConfig.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupEntry.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupHandle.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutConfig.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutEntry.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include "Nezumi/RHI/BindGroup/BindType.hpp"

// Buffers
#include "Nezumi/RHI/Buffer/BufferConfig.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Buffer/BufferUsageFlags.hpp"
#include "Nezumi/RHI/Buffer/IndexFormat.hpp"
#include "Nezumi/RHI/Buffer/MemoryUsageFlags.hpp"

// Common
#include "Nezumi/RHI/Common/Color.hpp"
#include "Nezumi/RHI/Common/Extent2D.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include "Nezumi/RHI/Common/Offset2D.hpp"
#include "Nezumi/RHI/Common/Rect2D.hpp"
#include "Nezumi/RHI/Common/Viewport.hpp"

// Pipeline
#include "Nezumi/RHI/Pipeline/BlendComponent.hpp"
#include "Nezumi/RHI/Pipeline/BlendFactor.hpp"
#include "Nezumi/RHI/Pipeline/BlendOp.hpp"
#include "Nezumi/RHI/Pipeline/BlendState.hpp"
#include "Nezumi/RHI/Pipeline/ColorTargetState.hpp"
#include "Nezumi/RHI/Pipeline/ColorWriteFlags.hpp"
#include "Nezumi/RHI/Pipeline/CompareOp.hpp"
#include "Nezumi/RHI/Pipeline/CullMode.hpp"
#include "Nezumi/RHI/Pipeline/DepthStencilState.hpp"
#include "Nezumi/RHI/Pipeline/FrontFace.hpp"
#include "Nezumi/RHI/Pipeline/PrimitiveState.hpp"
#include "Nezumi/RHI/Pipeline/PrimitiveTopology.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineConfig.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include "Nezumi/RHI/Pipeline/StencilOp.hpp"
#include "Nezumi/RHI/Pipeline/StencilOpState.hpp"
#include "Nezumi/RHI/Pipeline/VertexAttribute.hpp"
#include "Nezumi/RHI/Pipeline/VertexBufferLayout.hpp"
#include "Nezumi/RHI/Pipeline/VertexFormat.hpp"
#include "Nezumi/RHI/Pipeline/VertexStepMode.hpp"

// RenderPass
#include "Nezumi/RHI/RenderPass/ColorAttachment.hpp"
#include "Nezumi/RHI/RenderPass/DepthStencilAttachment.hpp"
#include "Nezumi/RHI/RenderPass/LoadOp.hpp"
#include "Nezumi/RHI/RenderPass/RenderPassConfig.hpp"
#include "Nezumi/RHI/RenderPass/StoreOp.hpp"

// Sampler
#include "Nezumi/RHI/Sampler/SamplerAddressMode.hpp"
#include "Nezumi/RHI/Sampler/SamplerConfig.hpp"
#include "Nezumi/RHI/Sampler/SamplerHandle.hpp"
#include "Nezumi/RHI/Sampler/SamplerFilterMode.hpp"

// Shader
#include "Nezumi/RHI/Shader/ShaderConfig.hpp"
#include "Nezumi/RHI/Shader/ShaderHandle.hpp"
#include "Nezumi/RHI/Shader/ShaderFormat.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"

// Texture
#include "Nezumi/RHI/Texture/TextureConfig.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"
#include "Nezumi/RHI/Texture/TextureRegion.hpp"
#include "Nezumi/RHI/Texture/TextureType.hpp"
#include "Nezumi/RHI/Texture/TextureUsageFlags.hpp"
