#include "VulkanContext.h"
#include "VulkanImage.h"
#include "resources/RenderTarget.h"
#include "RenderFrame.h"
#include "Device.h"
#include "Core/ServiceLocator.h"
#include <cassert>

VulkanContext::VulkanContext(Device& device, VkSurfaceKHR surface, uint32_t window_width, uint32_t window_height):
    m_DeviceRef(device), 
    m_Surface_extent{window_width,window_height},
    m_Queue(m_DeviceRef.getGraphicsQueue())
{
    if (surface != VK_NULL_HANDLE)
    {
        m_SwapChain = std::make_unique<SwapChain>(m_DeviceRef, surface, window_width, window_height);
    }
}


void VulkanContext::prepare(size_t nThreads, RenderTarget::CreateFunc createRenderTargetfunc)
{
    m_CreateRenderTargetFunction = createRenderTargetfunc;
    m_DeviceRef.wait_idle();//We are creating important stuff here we need idleing 

    if (m_SwapChain)
    {
        VkExtent3D extent{ m_Surface_extent.width, m_Surface_extent.height, 1 };
        for (const VkImage& imageHandle : m_SwapChain->get_images())
        {
            VulkanImage swapChainImage(m_DeviceRef, imageHandle, extent, m_SwapChain->get_format(), m_SwapChain->get_image_usage());
            
            auto renderTarget = m_CreateRenderTargetFunction(std::move(swapChainImage));
           
            m_Frames.emplace_back(std::make_unique<RenderFrame>(m_DeviceRef, std::move(renderTarget), nThreads));

        }
    }
    else//if swapchain is null means is not available hence we create a single frame
    {

    }
}

void VulkanContext::recreate(uint32_t window_width, uint32_t window_height)
{
    m_DeviceRef.wait_idle();
    VkSurfaceKHR surface = m_SwapChain->get_surface();
    m_SwapChain = std::make_unique<SwapChain>(*m_SwapChain,m_DeviceRef, surface, window_width, window_height);

    VkExtent3D extent{ m_Surface_extent.width, m_Surface_extent.height, 1 };

    auto frame_it = m_Frames.begin();

    for (const VkImage& imageHandle : m_SwapChain->get_images())
    {
        VulkanImage swapChainImage(m_DeviceRef, imageHandle, extent, m_SwapChain->get_format(), m_SwapChain->get_image_usage());

        auto renderTarget = m_CreateRenderTargetFunction(std::move(swapChainImage));
       

        if (frame_it != m_Frames.end())
        {
            (*frame_it)->updateRenderTarget(std::move(renderTarget));
        }
        else
        {
            // Create a new frame if the new swapchain has more images than current frames
            m_Frames.emplace_back(std::make_unique<RenderFrame>(m_DeviceRef, std::move(renderTarget), 1));
        }
        ++frame_it;

    }


}

void VulkanContext::checkForSurfaceChanges()
{
    
    VkSurfaceCapabilitiesKHR surface_properties;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_DeviceRef.get_physical_device(),
        m_SwapChain->get_surface(),
        &surface_properties);

    if (surface_properties.currentExtent.width != m_Surface_extent.width ||
        surface_properties.currentExtent.height != m_Surface_extent.height)
    {
        m_Surface_extent = surface_properties.currentExtent;
        recreate(surface_properties.currentExtent.width, surface_properties.currentExtent.height);
    }

}

CommandBuffer& VulkanContext::begin(CommandBuffer::ResetMode reset_mode)
{
    //TODO: Here we need to grab a commandbuffer from the current Frame --> Create the Frame class which has a vector of command pools (one per thread? )
    //Then we need to grab the right pool for this thread (threadId compare, but I might skip the multithread thingy for now..) and request a commandbuffer from the chosen
    //pool
    if(m_SwapChain)
        checkForSurfaceChanges();//We regenerate here the swapchainimages in case of window resizing

    assert(!m_FrameActive && "Frame is still active, please call end_frame");
    auto& previousFrame = *(m_Frames.at(m_FrameIndex));

    m_FrameSemaphore = previousFrame.requestSemaphore();

    if (m_SwapChain)
    {
        auto fence = previousFrame.requestFence(); 
        auto result = m_SwapChain->acquire_next_image(m_FrameIndex, m_FrameSemaphore, fence); //m_FrameIndex can be and will be increased here, we are passing it by reference!! 


        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            checkForSurfaceChanges();
            result = m_SwapChain->acquire_next_image(m_FrameIndex, m_FrameSemaphore, fence); //m_FrameIndex can be and will be increased here, we are passing it by reference!! 
        }

        if (result != VK_SUCCESS)
        {
            previousFrame.reset();
        }

    }
    m_FrameActive = true;
    waitFrame();

    if (m_FrameSemaphore == VK_NULL_HANDLE)
    {
        LOGERROR("Error can't begin frame!");
    }

    const auto& queue = m_DeviceRef.getQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0);
    return getActiveFrame().requestCommandBuffer(queue, reset_mode);
    
}

void VulkanContext::submit(const CommandBuffer& command_buffer)
{

    assert(m_FrameActive && "RenderContext is inactive, cannot submit command buffer. Please call begin()");

    VkSemaphore render_semaphore = VK_NULL_HANDLE;
    if(m_SwapChain)
        render_semaphore = submit(m_Queue, command_buffer, m_FrameSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    /*
    else
        submit(queue, command_buffer);
    */
    end(render_semaphore);

    m_FrameSemaphore = VK_NULL_HANDLE;
}

VkSemaphore VulkanContext::submit(const Queue& queue, const CommandBuffer& command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage)
{
    RenderFrame& frame = getActiveFrame();

    VkSemaphore signal_semaphore = frame.requestSemaphore();

    VkCommandBuffer cmd_buf = command_buffer.getHandle();

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &wait_semaphore;
    submit_info.pWaitDstStageMask = &wait_pipeline_stage;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &signal_semaphore;

    VkFence fence = frame.requestFence();

    queue.submit({ submit_info }, fence);

    return signal_semaphore;

}

RenderFrame& VulkanContext::getActiveFrame()const
{
    assert(m_FrameActive && "Frame is not active, please call begin_frame");
    return *(m_Frames.at(m_FrameIndex));
}

void VulkanContext::waitFrame()
{
    RenderFrame& frame = getActiveFrame();
    frame.reset();
}


void VulkanContext::end(VkSemaphore semaphore)
{
    assert(m_FrameActive && "Frame is not active, can't call end");

    if (m_SwapChain)
    {
        VkSwapchainKHR vk_swapchain = m_SwapChain->getHandle();

        VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &vk_swapchain;
        present_info.pImageIndices = &m_FrameIndex;

        VkResult result = m_Queue.present(present_info);

        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            checkForSurfaceChanges();
        }
    }


    m_FrameActive = false;


}