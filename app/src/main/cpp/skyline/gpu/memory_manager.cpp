// SPDX-License-Identifier: MPL-2.0
// Copyright © 2021 Skyline Team and Contributors (https://github.com/skyline-emu/)

#include <gpu.h>
#include "memory_manager.h"

namespace skyline::gpu::memory {
    StagingBuffer::~StagingBuffer() {
        vmaDestroyBuffer(vmaAllocator, vkBuffer, vmaAllocation);
    }

    void MemoryManager::ThrowOnFail(VkResult result, const char *function) {
        if (result != VK_SUCCESS)
            vk::throwResultException(vk::Result(result), function);
    }

    MemoryManager::MemoryManager(const GPU &pGpu) : gpu(pGpu) {
        auto dispatcher{gpu.vkDevice.getDispatcher()};
        VmaVulkanFunctions vulkanFunctions{
            .vkGetPhysicalDeviceProperties = dispatcher->vkGetPhysicalDeviceProperties,
            .vkGetPhysicalDeviceMemoryProperties = dispatcher->vkGetPhysicalDeviceMemoryProperties,
            .vkAllocateMemory = dispatcher->vkAllocateMemory,
            .vkFreeMemory = dispatcher->vkFreeMemory,
            .vkMapMemory = dispatcher->vkMapMemory,
            .vkUnmapMemory = dispatcher->vkUnmapMemory,
            .vkFlushMappedMemoryRanges = dispatcher->vkFlushMappedMemoryRanges,
            .vkInvalidateMappedMemoryRanges = dispatcher->vkInvalidateMappedMemoryRanges,
            .vkBindBufferMemory = dispatcher->vkBindBufferMemory,
            .vkBindImageMemory = dispatcher->vkBindImageMemory,
            .vkGetBufferMemoryRequirements = dispatcher->vkGetBufferMemoryRequirements,
            .vkGetImageMemoryRequirements = dispatcher->vkGetImageMemoryRequirements,
            .vkCreateBuffer = dispatcher->vkCreateBuffer,
            .vkDestroyBuffer = dispatcher->vkDestroyBuffer,
            .vkCreateImage = dispatcher->vkCreateImage,
            .vkDestroyImage = dispatcher->vkDestroyImage,
            .vkCmdCopyBuffer = dispatcher->vkCmdCopyBuffer,
            .vkGetBufferMemoryRequirements2KHR = dispatcher->vkGetBufferMemoryRequirements2,
            .vkGetImageMemoryRequirements2KHR = dispatcher->vkGetImageMemoryRequirements2,
            .vkBindBufferMemory2KHR = dispatcher->vkBindBufferMemory2,
            .vkBindImageMemory2KHR = dispatcher->vkBindImageMemory2,
            .vkGetPhysicalDeviceMemoryProperties2KHR = dispatcher->vkGetPhysicalDeviceMemoryProperties2,
        };
        VmaAllocatorCreateInfo allocatorCreateInfo{
            .physicalDevice = *gpu.vkPhysicalDevice,
            .device = *gpu.vkDevice,
            .instance = *gpu.vkInstance,
            .pVulkanFunctions = &vulkanFunctions,
            .vulkanApiVersion = GPU::VkApiVersion,
        };
        ThrowOnFail(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator));
        // TODO: Use VK_KHR_dedicated_allocation when available (Should be on Adreno GPUs)
    }

    MemoryManager::~MemoryManager() {
        vmaDestroyAllocator(vmaAllocator);
    }

    std::shared_ptr<StagingBuffer> MemoryManager::AllocateStagingBuffer(vk::DeviceSize size) {
        vk::BufferCreateInfo bufferCreateInfo{
            .size = size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &gpu.vkQueueFamilyIndex,
        };
        VmaAllocationCreateInfo allocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_ONLY,
        };

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        ThrowOnFail(vmaCreateBuffer(vmaAllocator, &static_cast<const VkBufferCreateInfo &>(bufferCreateInfo), &allocationCreateInfo, &buffer, &allocation, &allocationInfo));

        return std::make_shared<memory::StagingBuffer>(reinterpret_cast<u8 *>(allocationInfo.pMappedData), allocationInfo.size, vmaAllocator, buffer, allocation);
    }
}