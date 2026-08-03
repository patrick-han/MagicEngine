
#include "Renderer.h"
#include "Vulkan/Helpers.h"
#include "GPUContext.h"
#include "Swapchain.h"
#include "VertexDescriptors.h"
#include "Camera.h"
#include "Platform/Platform.h"
#include "MemoryManager.h"
#include "DefaultTexture.h"
#include <fstream>
#include <cassert>
#include <limits>
#include <thread>

#include "World.h"

#ifdef NDEBUG
#define MAGIC_DEBUG_VMA 0
#else
#define MAGIC_DEBUG_VMA 1
#endif

#if MAGIC_DEBUG_VMA
#include <mutex>
#if PLATFORM_MACOS
#include <execinfo.h>
#include <unistd.h>
#elif PLATFORM_WINDOWS
#include <stacktrace>
#endif

#endif

#include "GPUStats.h"
#include "CommandEncoder.h"
#include "Timing.h"
#include "../DataLibCode/DataSerialization.h" // TODO: find better organization for this maebbe
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/imgui_impl_vulkan.h"
namespace Magic
{

#if MAGIC_DEBUG_VMA
enum class VMAAllocType
{
    Buffer,
    Image,
    Unknown
};

#define DEBUG_STACKFRAME_SIZE 32

struct AllocationStacktraceDebugInfo {
#if PLATFORM_MACOS
    void* stack[DEBUG_STACKFRAME_SIZE];
#elif PLATFORM_WINDOWS
    std::stacktrace stack;
#endif
    int frameCount;
};

struct VMAAllocInfo
{
    VMAAllocType type = VMAAllocType::Unknown;
    union
    {
        VkBuffer buffer;
        VkImage image;
    };
    std::size_t size = 0;
    AllocationStacktraceDebugInfo info {};
};
std::mutex g_vmaAllocInfoMutex;
static std::unordered_map<VmaAllocation, VMAAllocInfo> g_vmaAllocInfo;
#endif

Renderer* GRenderer = nullptr;

Renderer::Renderer() { }

Renderer::~Renderer() { }

AllocatedBuffer Renderer::UploadBuffer(size_t bufferSize, const void *bufferData, VkBufferUsageFlags usage)
{
    assert(bufferSize > 0);
    VmaAllocator allocator = m_gpuctx->GetVmaAllocator();
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
        , .size = bufferSize
        , .usage = usage
    };

    // Let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaAllocInfo = { .usage = VMA_MEMORY_USAGE_CPU_TO_GPU };
    AllocatedBuffer allocatedBuffer;
    VK_CHECK(vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaAllocInfo,
        &allocatedBuffer.buffer,
        &allocatedBuffer.allocation,
        nullptr
    ));
#if MAGIC_DEBUG_VMA
    VMAAllocInfo debuginfo;
    debuginfo.type = VMAAllocType::Buffer;
    debuginfo.buffer = allocatedBuffer.buffer;
    debuginfo.size = bufferSize;
    AllocationStacktraceDebugInfo allocStack;
#if PLATFORM_MACOS
    allocStack.frameCount = backtrace(allocStack.stack, DEBUG_STACKFRAME_SIZE);
#elif PLATFORM_WINDOWS
    allocStack.stack = std::stacktrace::current();
    allocStack.frameCount = allocStack.stack.size();
#endif
    debuginfo.info = allocStack;
    {
        std::scoped_lock lock(g_vmaAllocInfoMutex);
        g_vmaAllocInfo.insert(std::make_pair(allocatedBuffer.allocation, debuginfo));
    }
#endif

    void* data;
    vmaMapMemory(allocator, allocatedBuffer.allocation, &data);
    std::memcpy(data, bufferData, bufferSize);
    vmaUnmapMemory(allocator, allocatedBuffer.allocation);
#if MAGIC_TRACK_GPU_STATS
    VmaAllocationInfo allocInfo {};
    vmaGetAllocationInfo(m_gpuctx->GetVmaAllocator(), allocatedBuffer.allocation, &allocInfo);
    GGpuStats.IncrementBufferBytes(allocInfo.size);
#endif
    return allocatedBuffer;
}

void Renderer::UpdateBuffer(const AllocatedBuffer &buffer, const void *dataSource, size_t bufferSize)
{
    VmaAllocator allocator = m_gpuctx->GetVmaAllocator();
    void* mapped = nullptr;
    VK_CHECK(vmaMapMemory(allocator, buffer.allocation, &mapped));
    std::memcpy(mapped, dataSource, bufferSize);
    // Required for non-coherent memory; harmless when coherent.
    VK_CHECK(vmaFlushAllocation(allocator, buffer.allocation, 0, bufferSize));
    vmaUnmapMemory(allocator, buffer.allocation);
}

void Renderer::DestroyBuffer(AllocatedBuffer allocatedBuffer)
{
#if MAGIC_TRACK_GPU_STATS
    {
        VmaAllocationInfo allocInfo {};
        vmaGetAllocationInfo(m_gpuctx->GetVmaAllocator(), allocatedBuffer.allocation, &allocInfo);
        GGpuStats.DecrementBufferBytes(allocInfo.size);
    }
#endif

    vmaDestroyBuffer(m_gpuctx->GetVmaAllocator(), allocatedBuffer.buffer, allocatedBuffer.allocation);
#if MAGIC_DEBUG_VMA
    {
        std::scoped_lock lock(g_vmaAllocInfoMutex);
        g_vmaAllocInfo.erase(allocatedBuffer.allocation);
    }
#endif
}


[[nodiscard]] AllocatedImage Renderer::CreateGPUOnlyImage(VkImageCreateInfo imageCreateInfo) {
    VmaAllocator allocator = m_gpuctx->GetVmaAllocator();
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    AllocatedImage allocatedImage;
    VK_CHECK(vmaCreateImage(allocator, &imageCreateInfo, &vmaAllocInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr));
#if MAGIC_DEBUG_VMA
    VMAAllocInfo debuginfo;
    debuginfo.type = VMAAllocType::Image;
    debuginfo.image = allocatedImage.image;
    debuginfo.size = 1337;
    AllocationStacktraceDebugInfo allocStack;
#if PLATFORM_MACOS
    allocStack.frameCount = backtrace(allocStack.stack, DEBUG_STACKFRAME_SIZE);
#elif PLATFORM_WINDOWS
    allocStack.stack = std::stacktrace::current();
    allocStack.frameCount = allocStack.stack.size();
#endif
    debuginfo.info = allocStack;
    {
        std::scoped_lock lock(g_vmaAllocInfoMutex);
        g_vmaAllocInfo.insert(std::make_pair(allocatedImage.allocation, debuginfo));
    }
#endif
    return allocatedImage;
}




AllocatedImage Renderer::UploadImage(const void *imageData, int numChannels, VkImageCreateInfo imageCreateInfo)
{
    AllocatedImage allocatedImage = CreateGPUOnlyImage(imageCreateInfo);
    size_t bytesPerChannel = 1;
    size_t dataSize = imageCreateInfo.extent.width * imageCreateInfo.extent.height * numChannels * bytesPerChannel;
#if MAGIC_TRACK_GPU_STATS
    VmaAllocationInfo allocInfo {};
    vmaGetAllocationInfo(m_gpuctx->GetVmaAllocator(), allocatedImage.allocation, &allocInfo);
    GGpuStats.IncrementImageBytes(allocInfo.size);
#endif
    AllocatedBuffer imageStagingBuffer = UploadBuffer(dataSize, imageData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    ImmediateSubmit([&](VkCommandBuffer cmd) {
        TransitionImage(cmd, allocatedImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageCreateInfo.extent;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, imageStagingBuffer.buffer, allocatedImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        TransitionImage(cmd, allocatedImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
    DestroyBuffer(imageStagingBuffer);
    return allocatedImage;
}

void Renderer::DestroyImage(AllocatedImage allocatedImage)
{
#if MAGIC_TRACK_GPU_STATS
    {
        VmaAllocationInfo allocInfo {};
        vmaGetAllocationInfo(m_gpuctx->GetVmaAllocator(), allocatedImage.allocation, &allocInfo);
        GGpuStats.DecrementImageBytes(allocInfo.size);
    }
#endif

    vmaDestroyImage(m_gpuctx->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
#if MAGIC_DEBUG_VMA
    {
        std::scoped_lock lock(g_vmaAllocInfoMutex);
        g_vmaAllocInfo.erase(allocatedImage.allocation);
    }
#endif
    vkDestroyImageView(m_gpuctx->GetDevice(), allocatedImage.view, nullptr);
}

void Renderer::Startup(GPUContext* _gpuctx, Swapchain* _swapchain)
{
    m_gpuctx = _gpuctx;
    m_swapchain = _swapchain;
    VkDevice device = m_gpuctx->GetDevice();

    // Bindless Manager
    m_bindlessManager.Initialize(_gpuctx);

    // Per frame in flight stuff
    for (auto & f : m_perFrameInFlightData)
    {
        VkCommandPool pool;
        VkCommandBuffer cmdBuffer;


        // Command buffers and pools
        VkCommandPoolCreateInfo commandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
            , .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT // allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer.
            , .queueFamilyIndex = m_gpuctx->GetGraphicsQueueFamilyIndex()
        };
        VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &pool));
        VkCommandBufferAllocateInfo cmdBufferAllocInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
            , .commandPool = pool
            , .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
            , .commandBufferCount = 1
        };
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, &cmdBuffer));

        f.m_commandEncoder = CommandEncoder(cmdBuffer, pool);

        // Image acquire semaphores
        VkSemaphoreCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &f.m_imageReadySemaphore));

        WorldData worldData{}; // Initially populate each frame with nothing
        f.m_worldData = UploadBuffer(sizeof(worldData), &worldData, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        VkBufferDeviceAddressInfoKHR worldDataBufferAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
            .buffer = f.m_worldData.buffer
        };
        f.m_worldDataAddress = vkGetBufferDeviceAddress(device, &worldDataBufferAddressInfo);
    }


    // TODO: Immediate resources
    {
        m_availableImmediateSubmitContexts = std::queue<uint32_t>();
        for (uint32_t i = 0; i < m_immediateCommandBufferCount; i++)
        {
            ImmediateSubmitContext& context = m_immediateSubmitContexts[i];
            VkCommandPoolCreateInfo commandPoolCreateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
                , .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                , .queueFamilyIndex = m_gpuctx->GetGraphicsQueueFamilyIndex()
            };
            VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &context.commandPool));

            VkCommandBufferAllocateInfo immCmdBufferAllocInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
                , .commandPool = context.commandPool
                , .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
                , .commandBufferCount = 1
            };
            VK_CHECK(vkAllocateCommandBuffers(device, &immCmdBufferAllocInfo, &context.commandBuffer));

            VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &context.fence));

            m_availableImmediateSubmitContexts.push(i);
        }
    }

    // Imgui resources
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VK_CHECK(vkCreateDescriptorPool(m_gpuctx->GetDevice(), &pool_info, VK_NULL_HANDLE, &m_imguiDescriptorPool));
    }
}

static std::vector<char> readFileBytes(const std::string& filename) {
    // Logger::Info(std::format("cwd = {}", std::filesystem::current_path().string()));
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
     if (!file.is_open()) { throw std::runtime_error("Failed to open file!"); }
    const std::size_t fileSize = (std::size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// TODO: temp
struct DefaultPushConstants {
    Matrix4f model;
    Matrix4f viewProjection;
    uint32_t diffuseTextureBindlessTextureArraySlot = 0;
    uint32_t normalTextureBindlessTextureArraySlot = 0;
    uint32_t metallicRoughnessTextureBindlessTextureArraySlot = 0;
    VkDeviceAddress worldDataBufferAddress = 0;
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.5f;
    float normalYSign = 1.0f;
};
static_assert(sizeof(WorldData) == 64);
static_assert(offsetof(DefaultPushConstants, worldDataBufferAddress) == 144);
static_assert(sizeof(DefaultPushConstants) == 168);

struct BoundingBoxPushConstants {
    Vector3f min;
    float pad0;
    Vector3f max;
    float pad1;
    Matrix4f viewProjection;
};
//

void Renderer::BuildResources() {
    // Can be deferred later TODO
    VkDevice device = m_gpuctx->GetDevice();

    {
        // Sampler(s)
        VkSamplerCreateInfo linearCI = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .anisotropyEnable = VK_FALSE,
            // .maxAnisotropy = maxAnisotropy,
        };
        VK_CHECK(vkCreateSampler(device, &linearCI, nullptr, &m_linearSampler));

        // Sampler(s)
        VkSamplerCreateInfo pointCI = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .anisotropyEnable = VK_FALSE,
            // .maxAnisotropy = maxAnisotropy,
        };
        VK_CHECK(vkCreateSampler(device, &pointCI, nullptr, &m_pointSampler));
        m_bindlessManager.UpdateBindlessSamplers(m_linearSampler, m_pointSampler);
    }

    {
        std::vector<char> vspv = readFileBytes("Shaders/triangleVertex.vertex.spv");
        std::vector<char> pspv = readFileBytes("Shaders/trianglePixel.pixel.spv");
        VkShaderModule vs_m = m_gpuctx->CreateShaderModule(vspv);
        VkShaderModule ps_m = m_gpuctx->CreateShaderModule(pspv);

        VkFormat outRTFormats[] = { m_colorFormat };
        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR
            , .colorAttachmentCount = 1
            , .pColorAttachmentFormats = outRTFormats
            , .depthAttachmentFormat = m_depthFormat
        };

        auto pipelineBuilder = GraphicsPipeline::CreateBuilder();
        pipelineBuilder.SetRenderingInfo(&pipelineRenderingInfo);
        pipelineBuilder.SetExtent(outputWidth, outputHeight);
        auto vd = SimpleVertexDescription();
        pipelineBuilder.SetVertexDescription(vd);

        pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT);
        pipelineBuilder.SetDescriptorSetLayouts(m_bindlessManager.m_descriptorSetLayout);
        pipelineBuilder.SetDepthTestEnable(true);
        pipelineBuilder.SetDepthCompareOp(VK_COMPARE_OP_LESS);

        {
            VkPushConstantRange defaultPushConstantRange = {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(DefaultPushConstants)
            };
            m_pushConstantRanges.push_back(defaultPushConstantRange);
            pipelineBuilder.SetPushConstantRanges(m_pushConstantRanges);
        }

        m_simplePipeline = pipelineBuilder.Build(device, vs_m, ps_m);

        vkDestroyShaderModule(device, vs_m, nullptr);
        vkDestroyShaderModule(device, ps_m, nullptr);
    }

    // Bounding box 
    {
        std::vector<char> vspv = readFileBytes("Shaders/aabbVertex.vertex.spv");
        std::vector<char> pspv = readFileBytes("Shaders/aabbPixel.pixel.spv");
        VkShaderModule vs_m = m_gpuctx->CreateShaderModule(vspv);
        VkShaderModule ps_m = m_gpuctx->CreateShaderModule(pspv);

        VkFormat outRTFormats[] = { m_colorFormat };
        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR
            , .colorAttachmentCount = 1
            , .pColorAttachmentFormats = outRTFormats
            , .depthAttachmentFormat = m_depthFormat
        };
        auto pipelineBuilder = GraphicsPipeline::CreateBuilder();
        pipelineBuilder.SetRenderingInfo(&pipelineRenderingInfo);
        pipelineBuilder.SetExtent(outputWidth, outputHeight);
        pipelineBuilder.SetDepthTestEnable(true);
        pipelineBuilder.SetDepthCompareOp(VK_COMPARE_OP_LESS);
        // pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT);
        // pipelineBuilder.SetRasterizerPolygonMode(VK_POLYGON_MODE_LINE); // Seemingly irrelevant for line topologies
        pipelineBuilder.SetInputAssemblyPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        {
            VkPushConstantRange boundingBoxPushConstantRange = {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(BoundingBoxPushConstants)
            };
            m_boundingBoxPushConstantRanges.push_back(boundingBoxPushConstantRange);
            pipelineBuilder.SetPushConstantRanges(m_boundingBoxPushConstantRanges);
        }
        m_debugDrawPipeline = pipelineBuilder.Build(device, vs_m, ps_m);
        vkDestroyShaderModule(device, vs_m, nullptr);
        vkDestroyShaderModule(device, ps_m, nullptr);
    }



    VkExtent3D rtExtent = {.width = static_cast<uint32_t>(outputWidth), .height = static_cast<uint32_t>(outputHeight), .depth = 1 };
    // Test color attachment
    {
        VkImageCreateInfo colorImageInfo = DefaultImageCreateInfo(m_colorFormat, rtExtent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TYPE_2D);
        VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        VK_CHECK(vmaCreateImage(m_gpuctx->GetVmaAllocator(), &colorImageInfo, &vmaAllocInfo, &m_rtColorImage.image, &m_rtColorImage.allocation, nullptr));

        VkImageViewCreateInfo colorImageViewInfo = DefaultImageViewCreateInfo(m_rtColorImage.image, m_colorFormat, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(device, &colorImageViewInfo, nullptr, &m_rtColorImage.view));
    }

    // Depth target
    {
        VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
        VkImageCreateInfo depthImageInfo = DefaultImageCreateInfo(m_depthFormat, rtExtent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TYPE_2D);
        VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        VK_CHECK(vmaCreateImage(m_gpuctx->GetVmaAllocator(), &depthImageInfo, &vmaAllocInfo, &m_rtDepthImage.image, &m_rtDepthImage.allocation, nullptr));

        VkImageViewCreateInfo depthImageViewInfo = DefaultImageViewCreateInfo(m_rtDepthImage.image, m_depthFormat, {}, VK_IMAGE_ASPECT_DEPTH_BIT);
        VK_CHECK(vkCreateImageView(device, &depthImageViewInfo, nullptr, &m_rtDepthImage.view));
    }

    // Single timeline semaphore, shared between frames in flight
    {
        VkSemaphoreCreateInfo        createInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphoreTypeCreateInfoKHR type_create_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR, .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE, .initialValue = 0 };
        createInfo.pNext              = &type_create_info;

        VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &m_timelineSemaphore));
    }
}

void Renderer::DestroyResources()
{
    VkDevice device = m_gpuctx->GetDevice();
    WaitIdle();
    vkDestroySampler(device, m_linearSampler, NULL);
    vkDestroySampler(device, m_pointSampler, NULL);
    m_debugDrawPipeline.Destroy();
    m_simplePipeline.Destroy();
    vkDestroySemaphore(device, m_timelineSemaphore, nullptr);
    { // Destroy rendertargetse
        vkDestroyImageView(device, m_rtDepthImage.view, nullptr);
        vmaDestroyImage(m_gpuctx->GetVmaAllocator(), m_rtDepthImage.image, m_rtDepthImage.allocation);
        vkDestroyImageView(device, m_rtColorImage.view, nullptr);
        vmaDestroyImage(m_gpuctx->GetVmaAllocator(), m_rtColorImage.image, m_rtColorImage.allocation);
    }
    m_bindlessManager.Shutdown();
}

void Renderer::DoWork(int frameNumber, RenderingInfo& renderingInfo)
{
    DoUIWork(frameNumber, renderingInfo);
    PerFrameInFlightData frameData = GetFrameInFlightData(frameNumber);
    VkDevice device = m_gpuctx->GetDevice();

    { // Wait for previous GPU work to finish FOR THIS FRAME IN FLIGHT before...
        uint64_t valueToWaitFor = frameData.signalValue;
        VkSemaphoreWaitInfo semaphore_wait_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &m_timelineSemaphore,
            .pValues = &valueToWaitFor //m_timelineValue  // same value that was signaled for queue submission FOR THIS FRAME IN FLIGHT
        };
        vkWaitSemaphores(device, &semaphore_wait_info, UINT64_MAX); // block CPU until finish queue work
    }



    CommandEncoder cmdEncoder = frameData.m_commandEncoder;

    const Swapchain::SwapchainImageData swapchainImageData = m_swapchain->GetNextSwapchainImageData(frameData.m_imageReadySemaphore);


    cmdEncoder.Reset();
    cmdEncoder.Begin();

    cmdEncoder.ImageBarrier(m_rtColorImage
    , VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    cmdEncoder.ImageBarrier(m_rtDepthImage
    , VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    , VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
    , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    {

        VkClearValue clearValue = {{{0.5f, 0.5f, 0.7f, 1.0f}}};
        auto rai_color = TEMP_rendering_attachment_info(m_rtColorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &clearValue);
        VkClearValue depthClearValue = {.depthStencil = {1.0f}};
        auto rai_depth = TEMP_rendering_attachment_info(m_rtDepthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, &depthClearValue);
        auto ri = TEMP_rendering_info_fullscreen(1, &rai_color, &rai_depth, outputWidth, outputHeight);
        cmdEncoder.BeginRendering(ri);

        cmdEncoder.SetViewport(outputWidth, outputHeight);
        cmdEncoder.SetScissor(outputWidth, outputHeight);

        // Bindless set

        vkCmdBindDescriptorSets(
            cmdEncoder.Handle(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_simplePipeline.GetPipelineLayout(),
            0, // firstSet
            1,
            &m_bindlessManager.m_descriptorSet, // your bindless descriptor set
            0,
            nullptr
        );

        Matrix4f viewProjection = renderingInfo.pCamera->GetProjectionMatrix(outputWidth, outputHeight, 0.1f, 2000.0f, 70.0f) * renderingInfo.pCamera->GetViewMatrix();

        std::span<Matrix4f const> transforms = GMemoryManager->GetFrameTransforms();

        {
            int subMeshIndex = 0;
            DefaultPushConstants pushConstants;
            pushConstants.viewProjection = viewProjection;

            { // Update per frame in flight world data
                pushConstants.worldDataBufferAddress = frameData.m_worldDataAddress;
                WorldData worldData{};
                if (renderingInfo.pWorld->m_pDirLight)
                {
                    worldData.dirLight.m_direction = renderingInfo.pWorld->m_pDirLight->m_direction;
                    worldData.dirLight.m_color = renderingInfo.pWorld->m_pDirLight->m_color;
                    worldData.dirLight.m_angle = renderingInfo.pWorld->m_pDirLight->m_angle;
                    worldData.dirLight.m_intensity = renderingInfo.pWorld->m_pDirLight->m_intensity;
                    worldData.dirLight.m_exposure = renderingInfo.pWorld->m_pDirLight->m_exposure;
                }

                worldData.cameraPos = Vector4f(renderingInfo.pCamera->GetPosition(), 1.0f);

                UpdateBuffer(frameData.m_worldData, &worldData, sizeof(worldData));
            }

            for (SubMesh* pSubMesh : renderingInfo.meshesToRender)
            {
                cmdEncoder.BindVertexBufferSimple(pSubMesh->vertexBuffer);
                cmdEncoder.BindIndexBufferSimple(pSubMesh->indexBuffer);
                {
                    pushConstants.model = transforms[subMeshIndex];

  
                    cmdEncoder.BindGraphicsPipeline(m_simplePipeline);
                    pushConstants.diffuseTextureBindlessTextureArraySlot = pSubMesh->diffuseTextureBindlessArraySlot;
                    pushConstants.normalTextureBindlessTextureArraySlot = pSubMesh->normalTextureBindlessArraySlot;
                    pushConstants.metallicRoughnessTextureBindlessTextureArraySlot = pSubMesh->metallicRoughnessTextureBindlessArraySlot;
                    pushConstants.metallicFactor = pSubMesh->metallicFactor;
                    pushConstants.roughnessFactor = pSubMesh->roughnessFactor;
                    pushConstants.normalYSign = pSubMesh->normalYSign;

                    vkCmdPushConstants(cmdEncoder.Handle(), m_simplePipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
                }
                cmdEncoder.DrawIndexedSimple(pSubMesh->indexCount, 0);
                subMeshIndex++;
            }
        }

        if (m_renderBoundingBoxes)
        {
            // Render all bounding boxes
            int subMeshIndex = 0;
            BoundingBoxPushConstants boundingBoxPushConstants;
            boundingBoxPushConstants.viewProjection = viewProjection;
            cmdEncoder.BindGraphicsPipeline(m_debugDrawPipeline);
            for (SubMesh* pSubMesh : renderingInfo.meshesToRender)
            {
                AABB3f worldSpaceAABB;
                worldSpaceAABB.Transform(pSubMesh->aabb, transforms[subMeshIndex]);
                boundingBoxPushConstants.min = worldSpaceAABB.GetMin();
                boundingBoxPushConstants.max = worldSpaceAABB.GetMax();
                vkCmdPushConstants(cmdEncoder.Handle(), m_debugDrawPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(boundingBoxPushConstants), &boundingBoxPushConstants);
                cmdEncoder.Draw(24, 1, 0, 0);
                subMeshIndex++;
            }
        }

        cmdEncoder.EndRendering();
    }
    cmdEncoder.ImageBarrier(m_rtColorImage
        , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    cmdEncoder.ImageBarrier(swapchainImageData.image
        , VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT
        , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    cmdEncoder.CopyImageToImage(m_rtColorImage, swapchainImageData.image
        , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_COLOR_BIT
        , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , outputWidth, outputHeight);

    // cmdEncoder.ImageBarrier(swapchainImageData.image
    //     , VK_ACCESS_TRANSFER_WRITE_BIT, {}
    //     , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
    //     , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


    {
        cmdEncoder.ImageBarrier(swapchainImageData.image
            , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        ImGui::Render();

        VkRenderingAttachmentInfo uiColor{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchainImageData.image.view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,              // keep the copied scene
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE
        };
        VkRenderingInfoKHR uiRI = {};
        uiRI.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        VkRect2D renderArea = {};
        renderArea.extent.width = outputWidth;
        renderArea.extent.height = outputHeight;
        uiRI.renderArea = renderArea;
        uiRI.layerCount = 1;
        uiRI.colorAttachmentCount = 1;
        uiRI.pColorAttachments = &uiColor;
        cmdEncoder.BeginRendering(uiRI);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdEncoder.Handle());
        cmdEncoder.EndRendering();

        cmdEncoder.ImageBarrier(swapchainImageData.image
            , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, {}
            , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
            , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }


    cmdEncoder.End();

    // Submit work
    {
        m_timelineValue++;
        SignalFrameInFlight(frameNumber, m_timelineValue);
        uint64_t value_to_signal = m_timelineValue;
        uint64_t values[] = { value_to_signal, 0 }; // 0 is dummy value for binary semaphore
        // Submit the command buffer with a timeline semaphore
        VkTimelineSemaphoreSubmitInfo timelineInfo{
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .waitSemaphoreValueCount = 0,
            .pWaitSemaphoreValues = nullptr,
            .signalSemaphoreValueCount = 2,
            .pSignalSemaphoreValues = values
        };
        VkSemaphore semaphores[] = { m_timelineSemaphore, swapchainImageData.presentSemaphore };
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkCommandBuffer handle = cmdEncoder.Handle();
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = &timelineInfo,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frameData.m_imageReadySemaphore,
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &handle,
            .signalSemaphoreCount = 2,
            .pSignalSemaphores = semaphores
        };
        {
            std::scoped_lock lock(m_graphicsQueueMutex);
            vkQueueSubmit(m_gpuctx->GetGraphicsQueue(), 1, &submitInfo, nullptr);
        }
    }

    VkSwapchainKHR s = m_swapchain->GetSwapchainHandle();
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchainImageData.presentSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &s,
        .pImageIndices = &swapchainImageData.imageIndex,
        .pResults = nullptr
    };
    {
        std::scoped_lock lock(m_graphicsQueueMutex);
        vkQueuePresentKHR(m_gpuctx->GetGraphicsQueue(), &presentInfo);
    }
}

void Renderer::WaitIdle()
{
    VkDevice device = m_gpuctx->GetDevice();
    vkDeviceWaitIdle(device);
}

void Renderer::Shutdown()
{
    VkDevice device = m_gpuctx->GetDevice();
    vkDestroyDescriptorPool(device, m_imguiDescriptorPool, nullptr);

    for (ImmediateSubmitContext& context : m_immediateSubmitContexts)
    {
        vkDestroyFence(device, context.fence, nullptr);
        context.fence = VK_NULL_HANDLE;
        vkDestroyCommandPool(device, context.commandPool, nullptr);
        context.commandPool = VK_NULL_HANDLE;
        context.commandBuffer = VK_NULL_HANDLE;
    }
    m_availableImmediateSubmitContexts = std::queue<uint32_t>();

    for (auto & p : m_perFrameInFlightData)
    {
        vkDestroySemaphore(device, p.m_imageReadySemaphore, nullptr);
        vkDestroyCommandPool(device, p.m_commandEncoder.Pool(), nullptr);
        DestroyBuffer(p.m_worldData);
    }

    // Check for gpu memory leaks
#if MAGIC_DEBUG_VMA
    {
        std::scoped_lock lock(g_vmaAllocInfoMutex);
        for (auto& debug : g_vmaAllocInfo)
        {
#if PLATFORM_MACOS
            backtrace_symbols_fd(debug.second.info.stack, debug.second.info.frameCount, STDERR_FILENO);
#elif PLATFORM_WINDOWS
            Logger::Err(std::to_string(debug.second.info.stack));
#endif
        }
    }
#endif
}

void Renderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function)
{
    VkDevice device = m_gpuctx->GetDevice();
    uint32_t contextIndex = 0;
    {
        std::unique_lock lock(m_immediateMutex);
        m_immediateCondition.wait(lock, [this]() { // m_immediateCondition.wait(lock, predicate) : Sleep until predicate is true
            return !m_availableImmediateSubmitContexts.empty();
        });
        contextIndex = m_availableImmediateSubmitContexts.front();
        m_availableImmediateSubmitContexts.pop();
    }

    ImmediateSubmitContext& context = m_immediateSubmitContexts[contextIndex];
    VK_CHECK(vkResetFences(device, 1, &context.fence));
    VK_CHECK(vkResetCommandBuffer(context.commandBuffer, {}));

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = {}
    };
    VK_CHECK(vkBeginCommandBuffer(context.commandBuffer, &beginInfo));

    function(context.commandBuffer);

    VK_CHECK(vkEndCommandBuffer(context.commandBuffer));

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &context.commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    {
        std::scoped_lock lock(m_graphicsQueueMutex);
        VK_CHECK(vkQueueSubmit(m_gpuctx->GetGraphicsQueue(), 1, &submitInfo, context.fence));
    }

    VK_CHECK(vkWaitForFences(device, 1, &context.fence, VK_TRUE, (std::numeric_limits<uint64_t>::max)()));

    {
        std::scoped_lock lock(m_immediateMutex);
        m_availableImmediateSubmitContexts.push(contextIndex);
    }
    m_immediateCondition.notify_one();
}


}
