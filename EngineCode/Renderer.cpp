
#include "Renderer.h"
#include "Vulkan/Helpers.h"
#include "GPUContext.h"
#include "Image.h"
#include "Swapchain.h"
#include <fstream>
namespace Magic
{

Renderer::Renderer() { }

Renderer::~Renderer() { }

void Renderer::Startup(GPUContext* _gpuctx, Swapchain* _swapchain)
{
    m_gpuctx = _gpuctx;
    m_swapchain = _swapchain;
    VkDevice device = m_gpuctx->GetDevice();

    // Per frame in flight stuff
    for (auto & f : m_perFrameInFlightData)
    {
        // Command buffers and pools
        VkCommandPoolCreateInfo commandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
            , .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT // allows any command buffer allocated from a pool to be individually reset to the initial state; either by calling vkResetCommandBuffer, or via the implicit reset when calling vkBeginCommandBuffer.
            , .queueFamilyIndex = m_gpuctx->GetGraphicsQueueFamilyIndex()
        };
        VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &f.m_commandPool));
        VkCommandBufferAllocateInfo cmdBufferAllocInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
            , .commandPool = f.m_commandPool
            , .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
            , .commandBufferCount = 1
        };
        VK_CHECK(vkAllocateCommandBuffers(device, &cmdBufferAllocInfo, &f.m_commandBuffer));

        // Image acquire semaphores
        VkSemaphoreCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &f.m_imageReadySemaphore));
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
    m_pipeline = pipelineBuilder.Build(device, vs_m, ps_m);

    vkDestroyShaderModule(device, vs_m, nullptr);
    vkDestroyShaderModule(device, ps_m, nullptr);

    {
        VkExtent3D extent = {.width = static_cast<uint32_t>(outputWidth), .height = static_cast<uint32_t>(outputHeight), .depth = 1 };
        VkImageCreateInfo colorImageInfo = TEMP_image_create_info(VK_FORMAT_B8G8R8A8_UNORM, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_TYPE_2D);
        VmaAllocationCreateInfo vmaAllocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        VK_CHECK(vmaCreateImage(m_gpuctx->GetVmaAllocator(), &colorImageInfo, &vmaAllocInfo, &m_colorImage, &m_colorImageAllocation, nullptr));

        VkImageViewCreateInfo colorImageViewInfo = DefaultImageViewCreateInfo(m_colorImage, VK_FORMAT_B8G8R8A8_UNORM, VkComponentMapping{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }, VK_IMAGE_ASPECT_COLOR_BIT);
        VK_CHECK(vkCreateImageView(device, &colorImageViewInfo, nullptr, &m_colorImageView));
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
    m_pipeline.Destroy();
    vkDestroySemaphore(device, m_timelineSemaphore, nullptr);
    vkDestroyImageView(device, m_colorImageView, nullptr);
    vmaDestroyImage(m_gpuctx->GetVmaAllocator(), m_colorImage, m_colorImageAllocation);
}

void Renderer::DoWork(int frameNumber)
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




    VkCommandBuffer cmdBuf = frameData.m_commandBuffer;

    // TODO
    PFN_vkCmdBeginRenderingKHR vkCmdBeginRendering = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
    PFN_vkCmdEndRenderingKHR vkCmdEndRendering = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));

    const Swapchain::SwapchainImageData swapchainImageData = m_swapchain->GetNextSwapchainImageData(frameData.m_imageReadySemaphore);

    {
        vkResetCommandBuffer(cmdBuf, {});
        VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(cmdBuf, &beginInfo);
    }
    {
        auto imb = TEMP_create_image_memory_barrier(m_colorImage, VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            {},
            0, nullptr,
            0, nullptr,
            1, &imb
        );
    }
    {
        VkClearValue clearValue = {{{0.5f, 0.5f, 0.7f, 1.0f}}};
        auto rai = TEMP_rendering_attachment_info(m_colorImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &clearValue);
        auto ri = TEMP_rendering_info_fullscreen(1, &rai, nullptr, outputWidth, outputHeight);
        vkCmdBeginRendering(cmdBuf, &ri);
        const VkViewport DEFAULT_VIEWPORT_FULLSCREEN = { 0.0f, 0.0f, static_cast<float>(outputWidth), static_cast<float>(outputHeight), 0.0f, 1.0f };
        const VkRect2D DEFAULT_SCISSOR_FULLSCREEN = { {0, 0}, {static_cast<uint32_t>(outputWidth), static_cast<uint32_t>(outputHeight)}};
        vkCmdSetViewport(cmdBuf, 0, 1, &DEFAULT_VIEWPORT_FULLSCREEN);
        vkCmdSetScissor(cmdBuf, 0, 1, &DEFAULT_SCISSOR_FULLSCREEN);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetPipelineHandle());
        vkCmdDraw(cmdBuf, 3, 1, 0, 0);
        vkCmdEndRendering(cmdBuf);
    }
    {
        auto imb = TEMP_create_image_memory_barrier(m_colorImage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            {},
            0, nullptr,
            0, nullptr,
            1, &imb
        );

        auto imb2 = TEMP_create_image_memory_barrier(
                    swapchainImageData.image,
                    VK_ACCESS_NONE,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                );
        vkCmdPipelineBarrier(
            cmdBuf,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            {},
            0, nullptr,
            0, nullptr,
            1, &imb2
        );
    }
    {
        const VkImageCopy imageCopy= {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
            },
            .srcOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1
            },
            .dstOffset = {
                .x = 0,
                .y = 0,
                .z = 0
            },
            .extent = {
                .width = static_cast<uint32_t>(outputWidth),
                .height = static_cast<uint32_t>(outputHeight),
                .depth = 1
            }
        };

        vkCmdCopyImage(
            cmdBuf,
            m_colorImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            swapchainImageData.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageCopy
        );
    }
    {
        auto imb = TEMP_create_image_memory_barrier(
                    swapchainImageData.image,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    {},
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                );
        vkCmdPipelineBarrier(
            cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            {},
            0, nullptr,
            0, nullptr,
            1, &imb
        );
    }
    {
        vkEndCommandBuffer(cmdBuf);
    }

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
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = &timelineInfo,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frameData.m_imageReadySemaphore,
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmdBuf,
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


void Renderer::Shutdown()
{
    VkDevice device = m_gpuctx->GetDevice();




    for (auto & p : m_perFrameInFlightData)
    {
        vkDestroySemaphore(device, p.m_imageReadySemaphore, nullptr);
        vkDestroyCommandPool(device, p.m_commandPool, nullptr);
    }
}

}