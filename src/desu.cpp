#include "desu.h"

VKAPI_ATTR VkBool32 VKAPI_CALL volcanoDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cout << "Validation layer message: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file!");
    const std::streamsize fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkResult createShaderModule(VkDevice device, size_t codeSize, const char* code, VkShaderModule* shaderModule) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = codeSize;
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code);

    return vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, shaderModule);
}

void Desu::init() {
    //Vulkan function loader
    volkInitialize();

    //Create window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    //glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "", nullptr, nullptr);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
    //glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
    //glfwSetWindowAttrib(window, GLFW_MAXIMIZED, GLFW_TRUE);
    //glfwSetWindowSize(window, 1920, 1080);

    //Create Vulkan instance
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    //const char* enabledLayers[] = {"VK_LAYER_KHRONOS_validation"};

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.apiVersion = VK_API_VERSION_1_3;
    applicationInfo.pEngineName = "Volcano";
    applicationInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    applicationInfo.pApplicationName = "Vulkan Bitch";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    //instanceCreateInfo.enabledLayerCount = countof(enabledLayers);
    //instanceCreateInfo.ppEnabledLayerNames = enabledLayers;
    instanceCreateInfo.enabledExtensionCount = instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    volkLoadInstance(instance);

    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = volcanoDebugCallback;
    messengerCreateInfo.pUserData = nullptr;

    if (vkCreateDebugUtilsMessengerEXT(instance, &messengerCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to create debug messenger");

    //Create Vulkan device
    uint32_t ione = 1;
    vkEnumeratePhysicalDevices(instance, &ione, &physicalDevice);
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    auto *queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        const VkQueueFlags flags = queueFamilies[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;
        else if (flags & VK_QUEUE_COMPUTE_BIT) computeFamily = i;
        else if (flags & VK_QUEUE_TRANSFER_BIT) transferFamily = i;
    }

    delete[] queueFamilies;

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    float defaultPriorities[64];
    defaultPriorities[0] = 1.0f;

    VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.pQueuePriorities = defaultPriorities;
    graphicsQueueCreateInfo.queueFamilyIndex = graphicsFamily;

    VkDeviceQueueCreateInfo computeQueueCreateInfo{};
    computeQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    computeQueueCreateInfo.queueCount = 1;
    computeQueueCreateInfo.pQueuePriorities = defaultPriorities;
    computeQueueCreateInfo.queueFamilyIndex = computeFamily;

    VkDeviceQueueCreateInfo transferQueueCreateInfo{};
    transferQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    transferQueueCreateInfo.queueCount = 1;
    transferQueueCreateInfo.pQueuePriorities = defaultPriorities;
    transferQueueCreateInfo.queueFamilyIndex = transferFamily;

    VkDeviceQueueCreateInfo queueCreateInfos[3] = {graphicsQueueCreateInfo, computeQueueCreateInfo, transferQueueCreateInfo};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 3;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.pEnabledFeatures = &features;
    deviceCreateInfo.enabledExtensionCount = countof(deviceExtensions);
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    
    vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

    volkLoadDevice(device);

    //Fetch queues
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
    vkGetDeviceQueue(device, transferFamily, 0, &transferQueue);

    //Pass loaded Vulkan functions to VMA
    VmaVulkanFunctions vmaVulkanFunctions{};
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaVulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vmaVulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vmaVulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vmaVulkanFunctions.vkAllocateMemory = vkAllocateMemory;
    vmaVulkanFunctions.vkFreeMemory = vkFreeMemory;
    vmaVulkanFunctions.vkMapMemory = vkMapMemory;
    vmaVulkanFunctions.vkUnmapMemory = vkUnmapMemory;
    vmaVulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vmaVulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vmaVulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
    vmaVulkanFunctions.vkBindImageMemory = vkBindImageMemory;
    vmaVulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vmaVulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vmaVulkanFunctions.vkCreateBuffer = vkCreateBuffer;
    vmaVulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
    vmaVulkanFunctions.vkCreateImage = vkCreateImage;
    vmaVulkanFunctions.vkDestroyImage = vkDestroyImage;
    vmaVulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vmaVulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vmaVulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vmaVulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vmaVulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
    vmaVulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vmaVulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vmaVulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

    //Create VMA allocator
    VmaAllocatorCreateInfo vmaAllocatorCreateInfo{};
    vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    vmaAllocatorCreateInfo.instance = instance;
    vmaAllocatorCreateInfo.physicalDevice = physicalDevice;
    vmaAllocatorCreateInfo.device = device;
    vmaAllocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;

    vmaCreateAllocator(&vmaAllocatorCreateInfo, &allocator);

    //Create window surface
    glfwCreateWindowSurface(instance, window, nullptr, &surface);

    //Create swapchain
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = 3;
    swapchainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_STORAGE_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &graphicsFamily;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = swapchain;

    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);

    //Get swapchain images
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

    swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

    swapchainImageViews.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
    }

    //Compute pipeline
    auto shaderCode = readFile("../draw.comp.spv");
    createShaderModule(device, shaderCode.size(), shaderCode.data(), &computeModule);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = computeModule;
    shaderStageInfo.pName = "main";

    VkDescriptorSetLayoutBinding renderTextureBinding{};
    renderTextureBinding.binding = 0;
    renderTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    renderTextureBinding.descriptorCount = 1;
    renderTextureBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutBinding spectrumBinding{};
    spectrumBinding.binding = 1;
    spectrumBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    spectrumBinding.descriptorCount = 1;
    spectrumBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding computeBindings[] = {renderTextureBinding, spectrumBinding};
    
    VkDescriptorSetLayoutCreateInfo computeLayoutCreateInfo{};
    computeLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    computeLayoutCreateInfo.bindingCount = countof(computeBindings);
    computeLayoutCreateInfo.pBindings = computeBindings;

    vkCreateDescriptorSetLayout(device, &computeLayoutCreateInfo, nullptr, &computeLayout);

    VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo{};
    computePipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computePipelineLayoutCreateInfo.setLayoutCount = 1;
    computePipelineLayoutCreateInfo.pSetLayouts = &computeLayout;

    vkCreatePipelineLayout(device, &computePipelineLayoutCreateInfo, nullptr, &computePipelineLayout);

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.stage = shaderStageInfo;
    computePipelineCreateInfo.layout = computePipelineLayout;

    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &computePipeline);
    
    //Create a command pool to contain drawing command buffers
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = graphicsFamily;

    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

    //Allocate a command buffer for each swapchain image to asynchronously submit
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = swapchainImageCount;

    commandBuffers.resize(swapchainImageCount);
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data());

    //Setup some synchronization objects
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    swapchainLockSemaphore.resize(swapchainImageCount);
    renderLockSemaphore.resize(swapchainImageCount);
    frameLockFence.resize(swapchainImageCount);

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &swapchainLockSemaphore[i]);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderLockSemaphore[i]);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &frameLockFence[i]);
    }
    
    //Create descriptor pools
    VkDescriptorPoolSize framePoolSizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
    VkDescriptorPoolCreateInfo framePoolCreateInfo{};
    framePoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    framePoolCreateInfo.maxSets = swapchainImageCount;
    framePoolCreateInfo.poolSizeCount = countof(framePoolSizes);
    framePoolCreateInfo.pPoolSizes = framePoolSizes;

    if (vkCreateDescriptorPool(device, &framePoolCreateInfo, nullptr, &framePool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool");

    //Descriptor set allocations & writes
    VkDescriptorSetAllocateInfo frameSetAllocateInfo{};
    frameSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    frameSetAllocateInfo.descriptorPool = framePool;
    frameSetAllocateInfo.descriptorSetCount = 1;
    frameSetAllocateInfo.pSetLayouts = &computeLayout;

    VkDescriptorBufferInfo descriptorBufferInfo{};
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = static_cast<VkDeviceSize>(sizeof(glm::vec4)*(44100/25));

    VkDescriptorImageInfo descriptorImageInfo{};

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorCount = 1;

    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writeDescriptorSet.pImageInfo = &descriptorImageInfo;

    frameSets.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkAllocateDescriptorSets(device, &frameSetAllocateInfo, &frameSets[i]);
        writeDescriptorSet.dstSet = frameSets[i];
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptorImageInfo.imageView = swapchainImageViews[i];
        writeDescriptorSet.dstBinding = 0;
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }
    
    writeDescriptorSet.pImageInfo = nullptr;
    
    //Create spectrum buffer
    VkBufferCreateInfo spectrumCreateInfo{};
    spectrumCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    spectrumCreateInfo.size = static_cast<VkDeviceSize>(sizeof(glm::vec4)*(44100/25));
    spectrumCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo spectrumAllocationCreateInfo{};
    spectrumAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    //spectrumAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    vmaCreateBuffer(allocator, &spectrumCreateInfo, &spectrumAllocationCreateInfo, &spectrum, &spectrumAllocation, nullptr);

    spectrumCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    spectrumAllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    //spectrumAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    //spectrumAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    vmaCreateBuffer(allocator, &spectrumCreateInfo, &spectrumAllocationCreateInfo, &spectrumStage, &spectrumStageAllocation, nullptr);

    //Write spectrum set
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        writeDescriptorSet.dstSet = frameSets[i];
        descriptorBufferInfo.buffer = spectrum;
        writeDescriptorSet.dstBinding = 1;
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
    }

    writeDescriptorSet.pBufferInfo = nullptr;

    //Staging command buffer
    commandBufferAllocateInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &stagingCommandBuffer);

    //Spectrum synchronizations
    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &spectrumAvailableSemaphore);
    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &spectrumLockSemaphore);
    vkCreateFence(device, &fenceCreateInfo, nullptr, &spectrumLockFence);
}

void Desu::start() {
    attach:
    //Start drawing
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //glfwShowWindow(window);
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) == GLFW_TRUE) continue; //Pause rendering if minimized
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) exit(1); //Exit button
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) goto reload; //Exit button

        // Delta time
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        vkWaitForFences(device, 1, &frameLockFence[frame], VK_TRUE, UINT64_MAX); //Wait for this frame to be unlocked (CPU)
        vkResetFences(device, 1, &frameLockFence[frame]); //Re-lock this frame (CPU)

        //Get the index of the next available image in the swapchain and assign that image to this frame, not guaranteed to be in order
        //The acquire function does not block this thread, so instead we make it signal a semaphore when the function returns an available image, so we can synchronize our queues for this frame
        uint32_t swapchainIndex = 0;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, swapchainLockSemaphore[frame], VK_NULL_HANDLE, &swapchainIndex);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        //Implicit command buffer reset
        if (vkBeginCommandBuffer(commandBuffers[frame], &commandBufferBeginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer");
            
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.image = swapchainImages[swapchainIndex];
        imageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VkBufferCopy spectrumCopy{};
        spectrumCopy.size = static_cast<VkDeviceSize>(sizeof(glm::vec4)*(44100/25));
        vkCmdCopyBuffer(commandBuffers[frame], spectrumStage, spectrum, 1, &spectrumCopy);

        vkCmdPipelineBarrier(commandBuffers[frame], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        vkCmdBindPipeline(commandBuffers[frame], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(commandBuffers[frame], VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &frameSets[swapchainIndex], 0, nullptr);
        vkCmdDispatch(commandBuffers[frame], width/16, height/16, 1);

        VkImageMemoryBarrier presentImageMemoryBarrier{};
        presentImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentImageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        presentImageMemoryBarrier.dstAccessMask = VK_ACCESS_NONE;
        presentImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        presentImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentImageMemoryBarrier.image = swapchainImages[swapchainIndex];
        presentImageMemoryBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(commandBuffers[frame], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentImageMemoryBarrier);

        if (vkEndCommandBuffer(commandBuffers[frame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer");
        
        VkSemaphore waitSemaphores[] = {swapchainLockSemaphore[frame]/*, spectrumAvailableSemaphore*/};
        VkSemaphore signalSemaphores[] = {renderLockSemaphore[frame]/*, spectrumLockSemaphore*/};
        
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[frame];
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.waitSemaphoreCount = countof(waitSemaphores);
        submitInfo.pWaitSemaphores = waitSemaphores; //Don't start until new image available (GPU)
        submitInfo.signalSemaphoreCount = countof(signalSemaphores);
        submitInfo.pSignalSemaphores = signalSemaphores; //Unlock render lock when rendering finished (GPU)

        //Unlock frame lock after rendering on that frame is finished (CPU)
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameLockFence[frame]) != VK_SUCCESS)
            throw std::runtime_error("Couldn't render ;(");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &swapchainIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderLockSemaphore[frame]; //Wait until image is rendered (GPU)

        if (vkQueuePresentKHR(graphicsQueue, &presentInfo) != VK_SUCCESS)
            throw std::runtime_error("Couldn't present rendered image, how did you fuck up?");

        frame = (frame + 1) % swapchainImageCount; //Jump to next frame
    }
    return;
    reload:
    vkDeviceWaitIdle(device);

    vkDestroyPipeline(device, computePipeline, nullptr);
    vkDestroyShaderModule(device, computeModule, nullptr);

    //Compute pipeline
    auto shaderCode = readFile("../draw.comp.spv");
    createShaderModule(device, shaderCode.size(), shaderCode.data(), &computeModule);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = computeModule;
    shaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.stage = shaderStageInfo;
    computePipelineCreateInfo.layout = computePipelineLayout;

    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &computePipeline);

    goto attach;
}

void Desu::destroy() {
    vkDeviceWaitIdle(device);

    vkDestroySemaphore(device, spectrumAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, spectrumLockSemaphore, nullptr);
    vkDestroyFence(device, spectrumLockFence, nullptr);

    vmaDestroyBuffer(allocator, spectrum, spectrumAllocation);
    vmaDestroyBuffer(allocator, spectrumStage, spectrumStageAllocation);
    
    vkDestroyDescriptorPool(device, framePool, nullptr);

    for (uint32_t i = 0; i < swapchainImageCount; ++i) {
        vkDestroySemaphore(device, swapchainLockSemaphore[i], nullptr);
        vkDestroySemaphore(device, renderLockSemaphore[i], nullptr);
        vkDestroyFence(device, frameLockFence[i], nullptr);
    }
    vkFreeCommandBuffers(device, commandPool, swapchainImageCount, commandBuffers.data());
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyPipeline(device, computePipeline, nullptr);
    vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, computeLayout, nullptr);
    vkDestroyShaderModule(device, computeModule, nullptr);

    for (uint32_t i = 0; i < swapchainImageCount; ++i)
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
}