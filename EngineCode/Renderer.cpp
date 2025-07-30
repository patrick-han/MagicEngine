
#include "Renderer.h"
#include "Vulkan/Helpers.h"
#include "GPUContext.h"
#include "Swapchain.h"
#include "VertexDescriptors.h"
#include "Camera.h"
#include <fstream>
#include <cassert>
#include "CommandEncoder.h"

#include "../DataLibCode/DataSerialization.h" // TODO: find better organization for this maebbe

namespace Magic
{

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

    // Copy data into mapped memory
    void* data;
    vmaMapMemory(allocator, allocatedBuffer.allocation, &data);
    memcpy(data, bufferData, bufferSize);
    vmaUnmapMemory(allocator, allocatedBuffer.allocation);
    return allocatedBuffer;
}

void Renderer::DestroyBuffer(AllocatedBuffer allocatedBuffer)
{
    vmaDestroyBuffer(m_gpuctx->GetVmaAllocator(), allocatedBuffer.buffer, allocatedBuffer.allocation);
}


[[nodiscard]] static AllocatedImage CreateGPUOnlyImage(VkImageCreateInfo imageCreateInfo, VmaAllocator allocator) {
    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    AllocatedImage allocatedImage;
    VK_CHECK(vmaCreateImage(allocator, &imageCreateInfo, &vmaAllocInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr));
    return allocatedImage;
}


static VkImageSubresourceRange default_image_subresource_range(VkImageAspectFlags aspectMask) // Transition all mipmap levels and layers by default
{
    VkImageSubresourceRange subImage {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}

static void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageMemoryBarrier imageBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = default_image_subresource_range(aspectMask)
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        {},
        0, nullptr,
        0, nullptr,
        1, &imageBarrier
    );
}

AllocatedImage Renderer::UploadImage(const void *imageData, int numChannels, VkImageCreateInfo imageCreateInfo)
{
    AllocatedImage allocatedImage = CreateGPUOnlyImage(imageCreateInfo, m_gpuctx->GetVmaAllocator());
    size_t bytesPerChannel = 1;
    size_t dataSize = imageCreateInfo.extent.width * imageCreateInfo.extent.height * numChannels * bytesPerChannel;
    AllocatedBuffer imageStagingBuffer = UploadBuffer(dataSize, imageData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    ImmediateSubmit([&](VkCommandBuffer cmd) {
        transition_image(cmd, allocatedImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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

        transition_image(cmd, allocatedImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
    DestroyBuffer(imageStagingBuffer);
    return allocatedImage;
}

void Renderer::DestroyImage(AllocatedImage allocatedImage)
{
    vmaDestroyImage(m_gpuctx->GetVmaAllocator(), allocatedImage.image, allocatedImage.allocation);
    vkDestroyImageView(m_gpuctx->GetDevice(), allocatedImage.view, nullptr);
}

void Renderer::Startup(GPUContext* _gpuctx, Swapchain* _swapchain)
{
    m_gpuctx = _gpuctx;
    m_swapchain = _swapchain;
    VkDevice device = m_gpuctx->GetDevice();

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
    }


    // TODO: Immediate resources
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
            , .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
            , .queueFamilyIndex = m_gpuctx->GetGraphicsQueueFamilyIndex()
        };
        vkCreateCommandPool(m_gpuctx->GetDevice(), &commandPoolCreateInfo, nullptr, &m_immediateCommandPool);
        // Immediate command buffer
        VkCommandBufferAllocateInfo immCmdBufferAllocInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
            , .commandPool = m_immediateCommandPool
            , .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
            , .commandBufferCount = 1
        };
        vkAllocateCommandBuffers(m_gpuctx->GetDevice(), &immCmdBufferAllocInfo, &m_immediateCommandBuffer);
        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT};
        vkCreateFence(m_gpuctx->GetDevice(), &fenceCreateInfo, nullptr, &m_immediateFence);
    }
}

static std::vector<char> readFileBytes(const std::string& filename) {
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
};
//

void Renderer::BuildResources()
{
    // Can be deferred later TODO
    VkDevice device = m_gpuctx->GetDevice();
    std::vector<char> vspv = readFileBytes("../Shaders/triangleVertex.vertex.spv");
    std::vector<char> pspv = readFileBytes("../Shaders/trianglePixel.pixel.spv");
    VkShaderModule vs_m = m_gpuctx->CreateShaderModule(vspv);
    VkShaderModule ps_m = m_gpuctx->CreateShaderModule(pspv);

    VkFormat outRTFormats[] = { m_swapchain->GetFormat() };
    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = outRTFormats, // match swapchain format for now
        // .depthAttachmentFormat = m_GfxDevice.m_depthImage.imageFormat,
    };

    auto pipelineBuilder = GraphicsPipeline::CreateBuilder();
    pipelineBuilder.SetRenderingInfo(&pipelineRenderingInfo);
    pipelineBuilder.SetExtent(outputWidth, outputHeight);
    auto vd = SimpleVertexDescription();
    pipelineBuilder.SetVertexDescription(vd);

    pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT);
    pipelineBuilder.SetRasterizerPolygonMode(VK_POLYGON_MODE_LINE);

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

    // Test triangle vertex buffer
    {
        // SimpleVertex v1 = {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
        // SimpleVertex v2 = {{0.0f,  1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        // SimpleVertex v3 = {{-1.0f,  -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
        // m_vertices.push_back(v1);
        // m_vertices.push_back(v2);
        // m_vertices.push_back(v3);
        // m_indices = {0, 2, 1}; // CCW winding order is frontfacing for all graphics pipelines
        // m_triBuffer = UploadBuffer(sizeof(SimpleVertex) * vertices.size(), vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        // m_triBufferIndices = UploadBuffer(sizeof(uint32_t) * indices.size(), indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }

    // Custom model
    {
        // auto testModel = Data::DeserializeModelData("../DataLibCode/helmet.bin");
        //
        // size_t meshIndex = 0;
        // for (const MeshData& meshData : testModel.m_meshes)
        // {
        //     RenderableMesh renderable;
        //     renderable.vertexBuffer = UploadBuffer(sizeof(SimpleVertex) * meshData.m_vertices.size(), meshData.m_vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        //     renderable.indexBuffer = UploadBuffer(sizeof(uint32_t) * meshData.m_indices.size(), meshData.m_indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        //     renderable.indexCount = meshData.m_indices.size();
        //     renderable.transform = testModel.m_transforms[meshIndex];
        //     m_renderables.push_back(renderable);
        //     meshIndex++;
        // }
        // Logger::Info("Loaded test model");
    }


    // Test color attachment
    {
        VkExtent3D extent = {.width = static_cast<uint32_t>(outputWidth), .height = static_cast<uint32_t>(outputHeight), .depth = 1 };
        VkImageCreateInfo colorImageInfo = TEMP_image_create_info(VK_FORMAT_B8G8R8A8_UNORM, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TYPE_2D);
        VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        VK_CHECK(vmaCreateImage(m_gpuctx->GetVmaAllocator(), &colorImageInfo, &vmaAllocInfo, &m_colorImage.image, &m_colorImage.allocation, nullptr));

        VkImageViewCreateInfo colorImageViewInfo = DefaultImageViewCreateInfo(m_colorImage.image, VK_FORMAT_B8G8R8A8_UNORM, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(device, &colorImageViewInfo, nullptr, &m_colorImage.view));
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
    vkDeviceWaitIdle(device);
    m_simplePipeline.Destroy();
    vkDestroySemaphore(device, m_timelineSemaphore, nullptr);
    vkDestroyImageView(device, m_colorImage.view, nullptr);
    vmaDestroyImage(m_gpuctx->GetVmaAllocator(), m_colorImage.image, m_colorImage.allocation);
}

void Renderer::DoWork(int frameNumber, RenderingInfo& renderingInfo)
{
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

    cmdEncoder.ImageBarrier(m_colorImage
    , VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    {
        VkClearValue clearValue = {{{0.5f, 0.5f, 0.7f, 1.0f}}};
        auto rai = TEMP_rendering_attachment_info(m_colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &clearValue);
        auto ri = TEMP_rendering_info_fullscreen(1, &rai, nullptr, outputWidth, outputHeight);

        cmdEncoder.BeginRendering(ri);
        cmdEncoder.SetViewport(outputWidth, outputHeight);
        cmdEncoder.SetScissor(outputWidth, outputHeight);
        cmdEncoder.BindGraphicsPipeline(m_simplePipeline);

        // TODO: render all renderables
        for (RenderableMesh& renderable : renderingInfo.meshesToRender)
        {
            cmdEncoder.BindVertexBufferSimple(renderable.vertexBuffer);
            cmdEncoder.BindIndexBufferSimple(renderable.indexBuffer);
            {
                DefaultPushConstants pushConstants;
                pushConstants.model = renderable.transform * Matrix4f::MakeScale(100.0f);
                pushConstants.viewProjection = renderingInfo.pCamera->GetProjectionMatrix(outputWidth, outputHeight, 0.1f, 2000.0f, 70.0f) * renderingInfo.pCamera->GetViewMatrix();
                vkCmdPushConstants(cmdEncoder.Handle(), m_simplePipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
            }
            cmdEncoder.DrawIndexedSimple(renderable.indexCount, 0);
        }
        cmdEncoder.EndRendering();
    }
    cmdEncoder.ImageBarrier(m_colorImage
        , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    cmdEncoder.ImageBarrier(swapchainImageData.image
        , VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT
        , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    cmdEncoder.CopyImageToImage(m_colorImage, swapchainImageData.image
        , VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_COLOR_BIT
        , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , outputWidth, outputHeight);

    cmdEncoder.ImageBarrier(swapchainImageData.image
        , VK_ACCESS_TRANSFER_WRITE_BIT, {}
        , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
            .waitSemaphoreValueCount = 0,         // No waits
            .pWaitSemaphoreValues = nullptr,
            .signalSemaphoreValueCount = 2,
            .pSignalSemaphoreValues = values
        };
        VkSemaphore semaphores[] = { m_timelineSemaphore, swapchainImageData.presentSemaphore };
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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
        vkQueueSubmit(m_gpuctx->GetGraphicsQueue(), 1, &submitInfo, nullptr);
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
    vkQueuePresentKHR(m_gpuctx->GetGraphicsQueue(), &presentInfo);
}

void Renderer::WaitIdle()
{
    VkDevice device = m_gpuctx->GetDevice();
    vkDeviceWaitIdle(device);
}

void Renderer::Shutdown()
{
    VkDevice device = m_gpuctx->GetDevice();

    vkDestroyCommandPool(m_gpuctx->GetDevice(), m_immediateCommandPool, nullptr);
    vkDestroyFence(m_gpuctx->GetDevice(), m_immediateFence, nullptr);

    for (auto & p : m_perFrameInFlightData)
    {
        vkDestroySemaphore(device, p.m_imageReadySemaphore, nullptr);
        vkDestroyCommandPool(device, p.m_commandEncoder.Pool(), nullptr);
    }
}

void Renderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function)
{
    VkDevice device = m_gpuctx->GetDevice();
    vkResetFences(device, 1, &m_immediateFence);
    vkResetCommandBuffer(m_immediateCommandBuffer, {});

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = {}
    };
    vkBeginCommandBuffer(m_immediateCommandBuffer, &beginInfo);

    function(m_immediateCommandBuffer);

    vkEndCommandBuffer(m_immediateCommandBuffer);

    // Submit immediate workload
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_immediateCommandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    vkQueueSubmit(m_gpuctx->GetGraphicsQueue(), 1, &submitInfo, m_immediateFence);

    vkWaitForFences(device, 1, &m_immediateFence, true, (std::numeric_limits<uint64_t>::max)());
}


}