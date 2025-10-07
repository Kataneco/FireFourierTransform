#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <cmath>

#define VK_NO_PROTOTYPES
#include "../lib/volk/volk.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "../lib/vk_mem_alloc.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
using glm::countof;

std::vector<char> readFile(const std::string& filename);
VkResult createShaderModule(VkDevice device, size_t codeSize, const char* code, VkShaderModule* shaderModule);

class Desu {
public:
    int width = 1600, height = 912;
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t graphicsFamily, computeFamily, transferFamily;
    VkQueue graphicsQueue, computeQueue, transferQueue;
    VmaAllocator allocator;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    uint32_t swapchainImageCount;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    
    VkShaderModule computeModule;
    VkDescriptorSetLayout computeLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> swapchainLockSemaphore;
    std::vector<VkSemaphore> renderLockSemaphore;
    std::vector<VkFence> frameLockFence;

    VkDescriptorPool framePool;
    std::vector<VkDescriptorSet> frameSets;

    uint32_t frame = 0;
    float deltaTime = 0.0f;
    float currentFrame = 0.0f;
    float lastFrame = 0.0f;
    
    VkBuffer spectrumStage;
    VmaAllocation spectrumStageAllocation;
    VkBuffer spectrum;
    VmaAllocation spectrumAllocation;

    VkCommandBuffer stagingCommandBuffer;
    VkSemaphore spectrumAvailableSemaphore, spectrumLockSemaphore;
    VkFence spectrumLockFence;
    
    void init();
    void start();
    void destroy();
};
